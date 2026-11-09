/*
 * trace.c - scripted command traces for the offline mode (stage 123): the
 * text format of include/rb_host_trace.h, its parser and the player that
 * renders one through a host with no device.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host_internal.h"
#include "rb_host_trace.h"

#define MAX_BUILDS 8
#define NAME_BYTES 32

/* What a command in a trace is. */
enum { TC_SET, TC_LIVE, TC_DYNO_ATTACH, TC_DYNO_RELEASE, TC_DYNO_SWEEP, TC_DYNO_CLEAR, TC_SCENE_TIME, TC_SWAP };

typedef struct trace_command {
    long long frame;
    int       kind;
    int       i0, i1;
    double    d0, d1;
} trace_command;

typedef struct trace_build {
    char          name[NAME_BYTES];
    rb_host_build spec;
} trace_build;

struct rb_trace {
    int            rate;
    long long      skip, frames;
    trace_build    builds[MAX_BUILDS];   /* [0] is the initial one */
    int            n_builds;
    trace_command *commands;
    int            n_commands, capacity;
};

static void say(char *error, size_t size, int line, const char *what, const char *detail)
{
    if (error && size) {
        if (line > 0) snprintf(error, size, "line %d: %s%s%s", line, what, detail ? " " : "", detail ? detail : "");
        else snprintf(error, size, "%s%s%s", what, detail ? " " : "", detail ? detail : "");
    }
}

void rb_trace_free(rb_trace *t)
{
    if (!t) return;
    free(t->commands);
    free(t);
}

int rb_trace_rate(const rb_trace *t) { return t ? t->rate : 0; }
long long rb_trace_skip_frames(const rb_trace *t) { return t ? t->skip : 0; }
long long rb_trace_frames(const rb_trace *t) { return t ? t->frames : 0; }
int rb_trace_command_count(const rb_trace *t) { return t ? t->n_commands : 0; }
int rb_trace_build_count(const rb_trace *t) { return t ? t->n_builds : 0; }

int rb_trace_channels(const rb_trace *t)
{
    if (!t) return 1;
    for (int i = 0; i < t->n_builds; i++)
        if (t->builds[i].spec.scene >= 0) return 2;
    return 1;
}

/* A finite number from a token. */
static int number(const char *token, double *out)
{
    if (!token) return 0;
    char *end;
    *out = strtod(token, &end);
    return *end == '\0' && end != token && isfinite(*out);
}

static int integer(const char *token, long long *out)
{
    double v;
    if (!number(token, &v) || v != floor(v) || fabs(v) > 9e15) return 0;
    *out = (long long)v;
    return 1;
}

static int quality_from(const char *token, int *out)
{
    static const char *const names[] = { "low", "medium", "high", "ultra" };
    for (int i = 0; i < 4; i++)
        if (!strcmp(token, names[i])) {
            *out = i;
            return 1;
        }
    long long v;
    if (integer(token, &v) && v >= -1 && v < RB_QUALITY_COUNT) {
        *out = (int)v;
        return 1;
    }
    return 0;
}

static int add_command(rb_trace *t, const trace_command *c)
{
    if (t->n_commands == t->capacity) {
        const int capacity = t->capacity ? 2 * t->capacity : 64;
        trace_command *grown = (trace_command *)realloc(t->commands, (size_t)capacity * sizeof *grown);
        if (!grown) return 0;
        t->commands = grown;
        t->capacity = capacity;
    }
    t->commands[t->n_commands++] = *c;
    return 1;
}

/* One build line (preset, edit, param, scene, scene_time, settle, quality, oversample) into `b`;
 * 1 if it was one and fine, 0 if it wasn't one, -1 if it was and wrong (`why` says what). */
static int build_line(rb_host_build *b, const char *const *tok, int n, const char **why)
{
    long long i;
    double v, w;
    if (!strcmp(tok[0], "preset")) {
        if (n != 2 || (b->preset = rb_preset_find(tok[1])) < 0) {
            *why = "needs the key of a preset";
            return -1;
        }
        return 1;
    }
    if (!strcmp(tok[0], "quality")) {
        int q;
        if (n != 2 || !quality_from(tok[1], &q)) {
            *why = "needs low, medium, high, ultra or -1";
            return -1;
        }
        b->quality = q;
        return 1;
    }
    if (!strcmp(tok[0], "oversample")) {
        if (n != 2 || !integer(tok[1], &i) || i < 0 || i > RB_MAX_OVERSAMPLE) {
            *why = "needs a whole number";
            return -1;
        }
        b->oversample = (int)i;
        return 1;
    }
    if (!strcmp(tok[0], "edit")) {
        const int field = n == 4 ? rb_engine_field_find(tok[1]) : -1;
        if (field < 0 || !integer(tok[2], &i) || !number(tok[3], &v) || rb_host_build_edit(b, field, (int)i, v) != RB_HOST_OK) {
            *why = "needs an engine field's name, an index and a value";
            return -1;
        }
        return 1;
    }
    if (!strcmp(tok[0], "param")) {
        const int id = n == 3 ? rb_param_find(tok[1]) : -1;
        if (id < 0 || !number(tok[2], &v) || rb_host_build_param(b, id, v) != RB_HOST_OK) {
            *why = "needs a parameter's name and a value";
            return -1;
        }
        return 1;
    }
    if (!strcmp(tok[0], "scene")) {
        if (n != 2 || !integer(tok[1], &i) || i < -1 || i >= RB_SCENE_COUNT) {
            *why = "needs a scene number";
            return -1;
        }
        b->scene = (int)i;
        return 1;
    }
    if (!strcmp(tok[0], "scene_time")) {
        if (n != 2 || !number(tok[1], &w)) {
            *why = "needs seconds";
            return -1;
        }
        b->scene_time = w;
        return 1;
    }
    if (!strcmp(tok[0], "settle")) {
        if (n != 2 || !integer(tok[1], &i) || i < 0 || i > 100000) {
            *why = "needs a number of blocks";
            return -1;
        }
        b->settle_blocks = (int)i;
        return 1;
    }
    return 0;
}

/* The command after "at FRAME" in tok[2..]. */
static int command_line(rb_trace *t, long long frame, const char *const *tok, int n, const char **why)
{
    trace_command c;
    memset(&c, 0, sizeof c);
    c.frame = frame;
    long long i;
    double v, w;
    const char *what = tok[2];
    if (!strcmp(what, "set")) {
        c.kind = TC_SET;
        c.i0 = n == 5 ? rb_param_find(tok[3]) : -1;
        if (c.i0 < 0 || !number(tok[4], &c.d0)) {
            *why = "set needs a parameter's name and a value";
            return 0;
        }
    } else if (!strcmp(what, "live")) {
        c.kind = TC_LIVE;
        c.i0 = n == 6 ? rb_engine_field_find(tok[3]) : -1;
        if (c.i0 < 0 || !integer(tok[4], &i) || !number(tok[5], &c.d0)) {
            *why = "live needs an engine field's name, an index and a value";
            return 0;
        }
        c.i1 = (int)i;
    } else if (!strcmp(what, "dyno_attach")) {
        c.kind = TC_DYNO_ATTACH;
        if (n != 4 || !number(tok[3], &c.d0)) {
            *why = "dyno_attach needs rpm";
            return 0;
        }
    } else if (!strcmp(what, "dyno_sweep")) {
        c.kind = TC_DYNO_SWEEP;
        if (n != 5 || !number(tok[3], &v) || !number(tok[4], &w)) {
            *why = "dyno_sweep needs a target rpm and a rate";
            return 0;
        }
        c.d0 = v;
        c.d1 = w;
    } else if (!strcmp(what, "dyno_release") || !strcmp(what, "dyno_clear")) {
        c.kind = !strcmp(what, "dyno_release") ? TC_DYNO_RELEASE : TC_DYNO_CLEAR;
        if (n != 3) {
            *why = "takes no value";
            return 0;
        }
    } else if (!strcmp(what, "scene_time")) {
        c.kind = TC_SCENE_TIME;
        if (n != 4 || !number(tok[3], &c.d0)) {
            *why = "scene_time needs seconds";
            return 0;
        }
    } else if (!strcmp(what, "swap")) {
        c.kind = TC_SWAP;
        c.i0 = -1;
        for (int k = 1; n == 4 && k < t->n_builds; k++)
            if (!strcmp(t->builds[k].name, tok[3])) c.i0 = k;
        if (c.i0 < 0) {
            *why = "swap needs the name of a build defined earlier";
            return 0;
        }
    } else {
        *why = "is not a command";
        return 0;
    }
    if (t->n_commands > 0 && frame < t->commands[t->n_commands - 1].frame) {
        *why = "comes before the command above it";
        return 0;
    }
    if (!add_command(t, &c)) {
        *why = "ran out of memory";
        return 0;
    }
    return 1;
}

rb_trace *rb_trace_parse(const char *text, char *error, size_t error_size)
{
    rb_trace *t = (rb_trace *)calloc(1, sizeof *t);
    if (!t) {
        say(error, error_size, 0, "out of memory", NULL);
        return NULL;
    }
    t->rate = 48000;
    t->n_builds = 1;
    strcpy(t->builds[0].name, "initial");
    rb_host_build_default(&t->builds[0].spec);
    rb_host_build *build = &t->builds[0].spec;
    int line_no = 0, seen_header = 0;
    const char *p = text;
    while (*p) {
        const char *end = strchr(p, '\n');
        const size_t len = end ? (size_t)(end - p) : strlen(p);
        char line[512];
        line_no++;
        if (len >= sizeof line) {
            say(error, error_size, line_no, "is too long", NULL);
            rb_trace_free(t);
            return NULL;
        }
        memcpy(line, p, len);
        line[len] = '\0';
        p += len + (end != NULL);
        char *hash = strchr(line, '#');
        if (hash) *hash = '\0';
        const char *tok[8];
        int n = 0;
        for (char *s = strtok(line, " \t\r"); s && n < 8; s = strtok(NULL, " \t\r")) tok[n++] = s;
        if (n == 0) continue;
        const char *why = NULL;
        long long i;
        double v;
        int ok = 1;
        if (!seen_header) {
            if (n != 2 || strcmp(tok[0], "rbtrace") || strcmp(tok[1], "1")) {
                say(error, error_size, line_no, "a trace starts with `rbtrace 1`", NULL);
                rb_trace_free(t);
                return NULL;
            }
            seen_header = 1;
            continue;
        }
        if (!strcmp(tok[0], "rate")) {
            ok = n == 2 && integer(tok[1], &i) && (i == 44100 || i == 48000);
            if (ok) t->rate = (int)i;
            else why = "needs 44100 or 48000";
        } else if (!strcmp(tok[0], "skip") || !strcmp(tok[0], "frames")) {
            ok = n == 2 && integer(tok[1], &i) && i >= 0;
            if (ok) {
                if (!strcmp(tok[0], "skip")) t->skip = i;
                else t->frames = i;
            } else {
                why = "needs a number of frames";
            }
        } else if (!strcmp(tok[0], "at")) {
            ok = n >= 3 && integer(tok[1], &i) && i >= 0 && command_line(t, i, tok, n, &why);
            if (!ok && !why) why = "needs a frame and a command";
        } else if (!strcmp(tok[0], "build")) {
            if (n != 2 || t->n_builds >= MAX_BUILDS || strlen(tok[1]) >= NAME_BYTES) {
                ok = 0;
                why = "needs a name (and at most 7 builds)";
            } else {
                snprintf(t->builds[t->n_builds].name, NAME_BYTES, "%s", tok[1]);
                rb_host_build_default(&t->builds[t->n_builds].spec);
                build = &t->builds[t->n_builds].spec;
                t->n_builds++;
            }
        } else if (!strcmp(tok[0], "end")) {
            build = &t->builds[0].spec;
        } else {
            const int r = build_line(build, tok, n, &why);
            if (r == 0) {
                ok = 0;
                why = "is not something a trace says";
            } else {
                ok = r > 0;
            }
        }
        (void)v;
        if (!ok) {
            say(error, error_size, line_no, tok[0], why ? why : "is wrong");
            rb_trace_free(t);
            return NULL;
        }
    }
    if (!seen_header) {
        say(error, error_size, 0, "empty: a trace starts with `rbtrace 1`", NULL);
        rb_trace_free(t);
        return NULL;
    }
    return t;
}

rb_trace *rb_trace_load(const char *path, char *error, size_t error_size)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        say(error, error_size, 0, "cannot read", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    const long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *text = size >= 0 ? (char *)malloc((size_t)size + 1) : NULL;
    if (!text || fread(text, 1, (size_t)size, f) != (size_t)size) {
        fclose(f);
        free(text);
        say(error, error_size, 0, "cannot read", path);
        return NULL;
    }
    fclose(f);
    text[size] = '\0';
    rb_trace *t = rb_trace_parse(text, error, error_size);
    free(text);
    return t;
}

/* ---- the player ---------------------------------------------------------------- */

int rb_trace_play(const rb_trace *t, rb_trace_sink sink, void *user, char *error, size_t error_size)
{
    if (!t || !sink) return RB_HOST_ERR_ARG;
    rb_host_config config;
    rb_host_config_default(&config);
    config.backend = RB_HOST_BACKEND_OFFLINE;
    config.sample_rate = t->rate;
    config.initial = t->builds[0].spec;
    rb_host *h = NULL;
    int rc = rb_host_create(&config, &h);
    if (rc != RB_HOST_OK) {
        say(error, error_size, 0, "the first engine could not be built", NULL);
        return rc;
    }
    rb_host_start(h);
    const int channels = rb_trace_channels(t);
    const long long total = t->skip + t->frames;
    float block[2 * RB_HOST_BLOCK_FRAMES], mono[RB_HOST_BLOCK_FRAMES];
    int next = 0;
    rc = RB_HOST_OK;
    for (long long frame = 0; frame < total && rc == RB_HOST_OK; frame += RB_HOST_BLOCK_FRAMES) {
        /* the driver's commands due, before the block: as rb_cli sets parameters between blocks */
        while (next < t->n_commands && t->commands[next].frame <= frame) {
            const trace_command *c = &t->commands[next++];
            int sent = RB_HOST_OK;
            uint64_t pid = 0;
            switch (c->kind) {
            case TC_SET:          sent = rb_host_set_param(h, c->i0, c->d0, 0, NULL); break;
            case TC_LIVE:         sent = rb_host_set_engine_live(h, c->i0, c->i1, c->d0, 0, NULL); break;
            case TC_DYNO_ATTACH:  sent = rb_host_dyno_attach(h, c->d0, 0, NULL); break;
            case TC_DYNO_RELEASE: sent = rb_host_dyno_release(h, 0, NULL); break;
            case TC_DYNO_SWEEP:   sent = rb_host_dyno_sweep(h, c->d0, c->d1, 0, NULL); break;
            case TC_DYNO_CLEAR:   sent = rb_host_dyno_clear(h, 0, NULL); break;
            case TC_SCENE_TIME:   sent = rb_host_set_scene_time(h, c->d0, 0, NULL); break;
            case TC_SWAP:
                sent = rb_host_prepare_begin(h, &t->builds[c->i0].spec, &pid);
                if (sent == RB_HOST_OK && rb_host_prepare_wait(h, pid, 120000) != RB_HOST_PREPARE_READY) sent = RB_HOST_ERR_BUILD;
                if (sent == RB_HOST_OK) sent = rb_host_swap(h, pid, RB_HOST_ANY_GENERATION, NULL);
                break;
            default: break;
            }
            if (sent != RB_HOST_OK) {
                say(error, error_size, 0, "a command at the frame could not be sent:", "the build or a queue refused it");
                rc = sent;
                break;
            }
        }
        if (rc != RB_HOST_OK) break;
        const int n = (int)(total - frame < RB_HOST_BLOCK_FRAMES ? total - frame : RB_HOST_BLOCK_FRAMES);
        rb_host_offline_render(h, block, n);
        rb_host_ack acks[16];
        rb_host_poll(h, acks, 16);
        /* what's recorded: from the skip on, in the output's own channels */
        const long long first = t->skip > frame ? t->skip - frame : 0;
        if (first < n) {
            const int out_frames = (int)(n - first);
            const float *src = block + 2 * first;
            if (channels == 2) {
                if (sink(user, src, out_frames, 2)) break;
            } else {
                for (int i = 0; i < out_frames; i++) mono[i] = src[2 * i];
                if (sink(user, mono, out_frames, 1)) break;
            }
        }
    }
    rb_host_destroy(h);
    return rc;
}
