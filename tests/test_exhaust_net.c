/*
 * test_exhaust_net.c - stage 138: the exhaust as a network (engine.h,
 * exhaust_net.c). The legacy fields' networks are valid and route as the
 * legacy exhaust did; a network given as the description builds the same
 * engine, to the last bit; side branches, closed ends, crossovers and extra
 * outlets build, run and keep the books; bad networks are refused; the
 * listener hears every outlet where it is.
 */
#include "audio.h"
#include "engine.h"
#include "noise.h"
#include "presets.h"
#include "rb.h"
#include "rb_test.h"
#include "scene.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define DT (1.0 / 192000.0)

static rb_engine a, b;

static void preset(const char *key, rb_engine_desc *d)
{
    rb_preset_desc(rb_preset_find(key), d);
}

/* Held at rpm and throttle for `steps` physics steps, from its build. */
static int run(rb_engine *e, const rb_engine_desc *d, double rpm, double throttle, long steps)
{
    if (rb_engine_init(e, d, DT) != 0) return -1;
    e->rpm = rpm;
    e->throttle_pos = throttle;
    e->ignition = 1;
    for (long s = 0; s < steps; s++) rb_engine_step(e);
    return 0;
}

void test_net_legacy_networks_are_valid(void)
{
    /* Every preset's fields compile into a network the network checks pass,
     * and each cylinder reaches the outlets its legacy exhaust did: its own
     * system's tailpipe, or both through an X or Y junction. */
    for (int p = 0; p < rb_preset_count(); p++) {
        rb_engine_desc d;
        rb_preset_desc(p, &d);
        rb_exhaust_net n;
        rb_exhaust_net_legacy(&d, &n);
        CHECK_EQ_INT(rb_exhaust_net_check(&n, &d), 0);
        CHECK_EQ_INT(n.outlets, d.exhaust_systems);
        int wrong = 0;
        for (int i = 0; i < d.cylinders; i++) {
            double length[RB_NET_OUTLETS];
            const int reached = rb_exhaust_net_paths(&n, &d, i, length);
            const int want = (d.junction_volume > 0.0) ? d.exhaust_systems : 1;
            wrong += reached != want;
            if (want == 1 && d.exhaust_systems == 2) wrong += !(length[d.collector[i]] > 0.0);
            /* the way is at least the header, the exhaust pipe and the tailpipe */
            for (int o = 0; o < n.outlets; o++)
                if (length[o] >= 0.0)
                    wrong += !(length[o] >= d.header_length[i] + d.exhaust_pipe_length
                                            + d.tailpipe_length - 1e-12);
        }
        printf("    %-9s %d pipes, %d volumes, %d outlets\n", rb_preset_key(p), n.pipes, n.nodes, n.outlets);
        CHECK_EQ_INT(wrong, 0);
    }
}

void test_net_as_description_builds_the_same_engine(void)
{
    /* The legacy network handed over as the description (net.pipes > 0)
     * builds the engine the fields do: every pipe's cells and every volume
     * the same after 0.1 s at 4000 rpm wide open, to the last bit, for every
     * preset; and each narrowed face as its area says (a catalyst or baffle
     * wider than its pipe narrows nothing: the boxer's). */
    int narrowed_in_all = 0;
    for (int p = 0; p < rb_preset_count(); p++) {
        rb_engine_desc d, n;
        rb_preset_desc(p, &d);
        n = d;
        rb_exhaust_net_legacy(&d, &n.net);
        CHECK(run(&a, &d, 4000.0, 1.0, 19200) == 0);
        CHECK(run(&b, &n, 4000.0, 1.0, 19200) == 0);
        int differ = a.pipe_total != b.pipe_total;
        for (int k = 0; k < a.pipe_total && !differ; k++) {
            const rb_pipe *x = rb_engine_pipe(&a, k), *y = rb_engine_pipe(&b, k);
            differ += x->cells != y->cells;
            for (int j = 0; j < x->cells && !differ; j++)
                differ += x->cell[j].p != y->cell[j].p || x->cell[j].T != y->cell[j].T;
        }
        for (int k = 0; k < a.net.nodes; k++) differ += a.xnode[k].p != b.xnode[k].p;
        for (int i = 0; i < d.cylinders; i++) differ += a.cyl[i].gas.p != b.cyl[i].gas.p;
        for (int o = 0; o < a.systems; o++) differ += a.tail_flow[o] != b.tail_flow[o];
        /* each narrowed face as its area says: an exit loss of (A / a)^2 - 1 */
        int narrowed = 0;
        for (int k = 0; k < b.net.pipes; k++)
            for (int end = 0; end < 2; end++) {
                const double area = b.net.pipe[k].end[end].area, r = b.xpipe[k].area / area;
                if (!(area > 0.0) || !(r > 1.0)) continue;
                narrowed++;
                differ += fabs(b.xpipe[k].end_face[end].exit - (r * r - 1.0)) > 1e-12 * r * r;
            }
        narrowed_in_all += narrowed;
        printf("    %-9s %s (%d narrowed faces)\n", rb_preset_key(p), differ ? "differs" : "identical", narrowed);
        CHECK_EQ_INT(differ, 0);
    }
    CHECK(narrowed_in_all >= 6);
}

/* The engine's books over a run, motored wide open at 3000 rpm with the
 * cylinders' walls adiabatic (as test_exhaust_engine_books: the cylinders'
 * wall heat and the fuel aren't in these books), and its repairs and guards
 * (none), and each outlet's flow (some). */
static void books(rb_engine *e, const rb_engine_desc *given, const char *what)
{
    rb_engine_desc d0 = *given;
    const rb_engine_desc *d = &d0;
    d0.heat_transfer = 0;
    CHECK(rb_engine_init(e, d, DT) == 0);
    e->rpm = 3000.0;
    e->throttle_pos = 1.0;
    for (long s = 0; s < 9600; s++) rb_engine_step(e);
    double m0, u0, y0[3], m1, u1, y1[3];
    const double in0 = e->mass_in, out0 = e->mass_out, ein0 = e->energy_in, eout0 = e->energy_out,
                 w0 = e->work_on_gas, q0 = rb_engine_pipe_heat(e);
    double sin0[3], sout0[3];
    for (int j = 0; j < 3; j++) sin0[j] = e->species_in[j], sout0[j] = e->species_out[j];
    rb_engine_contents(e, &m0, &u0, y0);
    double flow[RB_NET_OUTLETS] = { 0.0 };
    for (long s = 0; s < 3 * 7680; s++) {
        rb_engine_step(e);
        for (int o = 0; o < e->systems; o++) flow[o] = fmax(flow[o], fabs(e->tail_flow[o]));
    }
    rb_engine_contents(e, &m1, &u1, y1);
    const double dm = (e->mass_in - in0) - (e->mass_out - out0);
    const double du = (e->energy_in - ein0) - (e->energy_out - eout0) + (e->work_on_gas - w0)
                    + (rb_engine_pipe_heat(e) - q0);
    printf("    %-34s mass %+.1e, energy %+.1e of the flow; outlets' peak flow", what,
           (m1 - m0 - dm) / m0, (u1 - u0 - du) / fabs(e->energy_out - eout0));
    for (int o = 0; o < e->systems; o++) printf(" %.4f", flow[o]);
    printf(" m^3/s\n");
    CHECK_NEAR(m1 - m0, dm, m0 * 1e-11);
    CHECK_NEAR(u1 - u0, du, fabs(e->energy_out - eout0) * 1e-9);
    for (int j = 0; j < 3; j++)
        CHECK_NEAR(y1[j] - y0[j], (e->species_in[j] - sin0[j]) - (e->species_out[j] - sout0[j]), m0 * 1e-11);
    CHECK(rb_engine_guards(e) == 0);
    for (int o = 0; o < e->systems; o++) CHECK(flow[o] > 1e-4);
    CHECK(isfinite(e->omega) && e->omega > 0.0);
}

void test_net_new_parts_run_and_keep_the_books(void)
{
    rb_engine_desc d;
    /* a Helmholtz resonator on the four's exhaust pipe: a 1.5 L volume on a
     * 3 cm neck 8 cm long, half-way along */
    preset("i4", &d);
    rb_exhaust_net_legacy(&d, &d.net);
    {
        rb_exhaust_net *n = &d.net;
        const int v = n->nodes++, k = n->pipes++;
        n->node[v] = (rb_net_node){ 1.5e-3, RB_PLACE_JUNCTION, 0 };
        n->pipe[k] = (rb_net_pipe){ .role = RB_NET_ROLE_PIPE, .kind = RB_PIPE_EXHAUST, .number = 9,
                                    .length = 0.08, .diameter = 0.03 };
        n->pipe[k].end[0] = (rb_net_end){ .kind = RB_NET_TAP, .index = 0, .at = 0.5 };
        n->pipe[k].end[1] = (rb_net_end){ .kind = RB_NET_NODE, .index = v };
        CHECK_EQ_INT(rb_exhaust_net_check(n, &d), 0);
        books(&a, &d, "i4 + Helmholtz side branch");
        /* its neck opens into the exhaust pipe's middle cell */
        const rb_pipe *x = &a.xpipe[0];
        CHECK(a.xpipe[k].end[0].chamber == &x->cell[(int)lround(0.5 * (x->cells - 1))]);
        double lo = INFINITY, hi = -INFINITY;
        for (long s = 0; s < 3840; s++) {
            rb_engine_step(&a);
            lo = fmin(lo, a.xnode[v].p), hi = fmax(hi, a.xnode[v].p);
        }
        printf("    the resonator's volume swings %.0f Pa\n", hi - lo);
        CHECK(hi - lo > 100.0);   /* it breathes */
    }
    /* a quarter-wave tube: 50 cm closed at its far end, off the exhaust pipe */
    preset("i4", &d);
    rb_exhaust_net_legacy(&d, &d.net);
    {
        rb_exhaust_net *n = &d.net;
        const int k = n->pipes++;
        n->pipe[k] = (rb_net_pipe){ .role = RB_NET_ROLE_PIPE, .kind = RB_PIPE_EXHAUST, .number = 9,
                                    .length = 0.5, .diameter = 0.04 };
        n->pipe[k].end[0] = (rb_net_end){ .kind = RB_NET_TAP, .index = 0, .at = 0.3 };
        CHECK_EQ_INT(rb_exhaust_net_check(n, &d), 0);
        books(&a, &d, "i4 + quarter-wave tube");
    }
    /* three outlets off the four's muffler */
    preset("i4", &d);
    rb_exhaust_net_legacy(&d, &d.net);
    {
        rb_exhaust_net *n = &d.net;
        for (int o = 1; o < 3; o++) {
            const int k = n->pipes++;
            n->pipe[k] = n->pipe[1];   /* the tailpipe's twin */
            n->pipe[k].number = o;
            n->pipe[k].end[1].index = o;
            n->outlet[o] = n->outlet[0];
            n->outlet[o].at[0] = 0.5 - 0.15 * o;
        }
        n->outlets = 3;
        CHECK_EQ_INT(rb_exhaust_net_check(n, &d), 0);
        books(&a, &d, "i4 with three outlets");
        CHECK_EQ_INT(a.systems, 3);
    }
    /* an H-pipe between the flat V8's two banks */
    preset("v8-flat", &d);
    rb_exhaust_net_legacy(&d, &d.net);
    {
        rb_exhaust_net *n = &d.net;
        const int k = n->pipes++;
        n->pipe[k] = (rb_net_pipe){ .role = RB_NET_ROLE_PIPE, .kind = RB_PIPE_EXHAUST, .number = 9,
                                    .length = 0.25, .diameter = 0.05 };
        n->pipe[k].end[0] = (rb_net_end){ .kind = RB_NET_TAP, .index = 0, .at = 0.6 };
        n->pipe[k].end[1] = (rb_net_end){ .kind = RB_NET_TAP, .index = 1, .at = 0.6 };
        CHECK_EQ_INT(rb_exhaust_net_check(n, &d), 0);
        /* each bank now reaches both outlets */
        double length[RB_NET_OUTLETS];
        CHECK_EQ_INT(rb_exhaust_net_paths(n, &d, 0, length), 2);
        books(&b, &d, "v8-flat + H-pipe");
    }
}

void test_net_check_refuses_bad_networks(void)
{
    rb_engine_desc d;
    preset("v8-flat", &d);
    rb_exhaust_net good;
    rb_exhaust_net_legacy(&d, &good);
    CHECK_EQ_INT(rb_exhaust_net_check(&good, &d), 0);
    rb_exhaust_net n;
    int refused = 0, cases = 0;
#define BAD(...) do { n = good; __VA_ARGS__; cases++; refused += rb_exhaust_net_check(&n, &d) != 0; } while (0)
    BAD(n.pipes = 0);
    BAD(n.pipes = RB_NET_PIPES + 1);
    BAD(n.outlets = 0);
    BAD(n.node[0].volume = 0.0);
    BAD(n.pipe[0].length = 0.0);
    BAD(n.pipe[0].diameter = 0.9);
    BAD(n.pipe[0].end[0].index = 99);                          /* no such volume  */
    BAD(n.pipe[0].end[1] = (rb_net_end){ .kind = RB_NET_INTO, .index = 0 });   /* into itself */
    BAD(n.pipe[0].end[0] = (rb_net_end){ .kind = RB_NET_ONWARD, .index = 2 }); /* onward from end 0 */
    BAD(n.pipe[2].end[1] = n.pipe[3].end[1]);                  /* outlet 0 lost to outlet 1 */
    BAD(n.pipe[n.pipes] = n.pipe[3]; n.pipes++);               /* two pipes into outlet 1 */
    BAD(n.pipe[2].end[0] = (rb_net_end){ .kind = RB_NET_OUTLET, .index = 0 });  /* out of end 0 */
    BAD(n.pipe[0].end[1] = (rb_net_end){ .kind = RB_NET_TAP, .index = 2, .at = 1.5 });
    BAD(n.pipe[0].end[1].area = 0.2);
    BAD(n.header_to[0] = (rb_net_end){ .kind = RB_NET_CLOSED });
    BAD(n.pipe[4].end[1] = (rb_net_end){ .kind = RB_NET_CLOSED };   /* bank 0 loses its way out */
        n.pipe[2].end[1] = (rb_net_end){ .kind = RB_NET_CLOSED });
#undef BAD
    printf("    %d of %d bad networks refused\n", refused, cases);
    CHECK_EQ_INT(refused, cases);
    /* and an engine won't build from one */
    rb_engine_desc bad = d;
    bad.net = good;
    bad.net.outlets = 0;
    CHECK(rb_engine_init(&a, &bad, DT) != 0);
}

void test_net_listener_hears_every_outlet(void)
{
    /* Three outlets, each heard from behind at its own distance: its direct
     * path's gain 1/r of where it is, and a fourth (absent) silent. */
    static rb_listener l;
    const double at[9] = { 0.5, -0.05, 0.35, 0.0, -0.10, 0.40, -0.5, -0.05, 0.35 };
    CHECK_EQ_INT(rb_listener_place_at(&l, RB_HEAR_BEHIND, at, 3, 343.0, 1.0 / 48000.0), 0);
    const double ear[3] = { 0.6, -3.0, 1.6 };   /* audio.c: RB_HEAR_BEHIND */
    int wrong = 0;
    for (int o = 0; o < 3; o++) {
        const double dx = at[3 * o] - ear[0], dy = at[3 * o + 1] - ear[1], dz = at[3 * o + 2] - ear[2];
        wrong += fabs(l.gain[o][0] - 1.0 / sqrt(dx * dx + dy * dy + dz * dz)) > 1e-12;
    }
    wrong += l.gain[3][0] != 0.0;
    CHECK(l.gain[RB_LISTEN_MOUTH][0] > 0.0);
    CHECK_EQ_INT(wrong, 0);
    CHECK(rb_listener_place_at(&l, RB_HEAR_BEHIND, at, RB_LISTEN_OUTLETS + 1, 343.0, 1.0 / 48000.0) != 0);
    /* and the stereo scene: each of the three outlets heard by both ears, a
     * fourth not at all */
    static rb_scene sc;
    const double diameter[3] = { 0.06, 0.08, 0.06 };
    CHECK_EQ_INT(rb_scene_init_at(&sc, RB_SCENE_ASPHALT, 48000.0, 343.0, at, diameter, 3), 0);
    int heard = 0;
    for (int e = 0; e < 2; e++)
        for (int o = 0; o < RB_SCENE_OUTLETS; o++) heard += (sc.path[e][o][0].gain > 0.0) == (o < 3);
    CHECK_EQ_INT(heard, 2 * RB_SCENE_OUTLETS);
    CHECK(fabs(sc.at[1][1] - at[4]) < 1e-15 && sc.diameter[1] == 0.08);
}

static rb_noise na, nb;

void test_net_noise_routes_as_the_legacy_exhaust(void)
{
    /* Where the parts on its ways are the legacy muffler and catalyst (the
     * flat V8's: a catalyst and a "none" muffler; no centre can), the noise
     * layer's network routing is the legacy routing: the same shares, delays
     * and baffle jets, and the same sound, to the last bit, over 0.1 s at
     * 4000 rpm wide open. The four has a centre can as well, which the legacy
     * routing (older than its centre cans) doesn't count and the network's
     * does: the same delays and baffle jets, and every cylinder's share less
     * by the same factor, the can's. */
    const char *keys[2] = { "v8-flat", "i4" };
    for (int p = 0; p < 2; p++) {
        rb_engine_desc d, n;
        preset(keys[p], &d);
        n = d;
        rb_exhaust_net_legacy(&d, &n.net);
        CHECK(run(&a, &d, 4000.0, 1.0, 9600) == 0);
        CHECK(run(&b, &n, 4000.0, 1.0, 9600) == 0);
        rb_noise_init(&na, &a, 7);
        rb_noise_init(&nb, &b, 7);
        int differ = na.holes != nb.holes;
        for (int h = 0; h < na.holes && h < nb.holes; h++)
            for (int s = 0; s < a.systems; s++)
                differ += na.hole[h].delay[s] != nb.hole[h].delay[s] || na.hole[h].share[s] != nb.hole[h].share[s];
        double ratio = -1.0;
        for (int i = 0; i < d.cylinders; i++)
            for (int s = 0; s < a.systems; s++) {
                const double legacy = na.blow_share[i][s] * sqrt(na.blow_pass), net = nb.blow_share[i][s];
                differ += (legacy > 0.0) != (net > 0.0);
                if (!(legacy > 0.0)) continue;
                differ += na.blow_delay[i][s] != nb.blow_delay[i][s];
                if (ratio < 0.0) ratio = net / legacy;
                differ += fabs(net / legacy - ratio) > 1e-12;
            }
        double most = 0.0;
        for (long s = 0; s < 19200; s++) {
            rb_engine_step(&a);
            rb_engine_step(&b);
            rb_noise_step(&na, &a, RB_NOISE_FLOW);
            rb_noise_step(&nb, &b, RB_NOISE_FLOW);
            for (int k = 0; k < RB_NOISE_OUTPUTS; k++)
                most = fmax(most, fabs(na.out[k] - nb.out[k]) / fmax(1e-30, fabs(na.out[k])));
        }
        printf("    %-8s %d baffle jets; the network's shares %.4f of the legacy's; the sound's largest "
               "difference %.1e of itself\n", keys[p], nb.holes, ratio, most);
        CHECK_EQ_INT(differ, 0);
        if (p == 0) {
            CHECK(ratio == 1.0);
            CHECK(most == 0.0);
        } else {
            CHECK(ratio > 0.01 && ratio < 0.2);   /* sqrt of a chambered can's pass */
        }
    }
}
