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
        if (d.net.pipes > 0) continue;   /* a preset with its own network (stage 141) */
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

/* The walls' viscothermal loss of the power down a pipe (noise.c's wall_pass,
 * Kirchhoff at 3 kHz in the hot gas). */
static double walls(double diameter, double length)
{
    const double alpha = sqrt(0.5 * 2.0 * 3.14159265358979323846 * 3000.0 * 9e-5) / (0.5 * diameter * 580.0)
                       * (1.0 + 0.35 / sqrt(0.7));
    return exp(-2.0 * alpha * length);
}

void test_net_noise_routes_as_the_legacy_exhaust(void)
{
    /* A network's noise routing against the legacy one, on the legacy
     * networks: the same delays and the same baffle jets; and where the
     * parts on its ways pass what the legacy constants do (the flat V8: a
     * catalyst, and a "none" muffler a pipe the tailpipe's size, which passes
     * all), every share is the legacy's times the walls' loss down that way
     * (Kirchhoff, header and pipes), to 1e-12. The four's network passes its
     * muffler and centre can from their geometry (stage 139): its shares
     * differ from the legacy's by more than the walls. */
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
                differ += na.hole[h].delay[s] != nb.hole[h].delay[s];
        double worst = 0.0;
        for (int i = 0; i < d.cylinders; i++) {
            double length[RB_NET_OUTLETS];
            int route[RB_NET_OUTLETS][RB_NET_PIPES], count[RB_NET_OUTLETS];
            rb_exhaust_net_route(&b.net, &d, i, 0, 0, length, route, count);
            for (int s = 0; s < a.systems; s++) {
                const double legacy = na.blow_share[i][s] * sqrt(na.blow_pass), net = nb.blow_share[i][s];
                differ += (legacy > 0.0) != (net > 0.0);
                if (!(legacy > 0.0)) continue;
                differ += na.blow_delay[i][s] != nb.blow_delay[i][s];
                double wall = walls(d.header_diameter, d.header_length[i]);
                for (int q = 0; q < count[s]; q++)
                    wall *= walls(b.net.pipe[route[s][q]].diameter, b.net.pipe[route[s][q]].length);
                worst = fmax(worst, fabs(net / (legacy * sqrt(wall)) - 1.0));
            }
        }
        printf("    %-8s %d baffle jets; the network's shares the legacy's times the walls' within %.1e\n",
               keys[p], nb.holes, worst);
        CHECK_EQ_INT(differ, 0);
        if (p == 0) CHECK(worst < 1e-12);
        else CHECK(worst > 0.1);   /* its cans from their geometry */
    }
}

/* The noise layer's share of cylinder 0's noise at outlet 0 for an engine
 * built from network n on description d. */
static double share_of(const rb_engine_desc *d)
{
    CHECK(rb_engine_init(&a, d, DT) == 0);
    rb_noise_init(&na, &a, 7);
    return na.blow_share[0][0];
}

void test_net_passes_from_geometry(void)
{
    /* Stage 139: a network's parts pass the valves' noise as their geometry
     * says. The four's network with its centre can a plain pipe: its muffler
     * an expansion chamber, which passes 1 / sqrt(1 + (m - 1/m)^2 / 4) of
     * the power (m its area over the pipes'), so two bores' shares (the
     * square root of the power) are in that ratio; a packed core is a lined
     * duct (Piening: 1.05 a^1.4 4 / d dB a metre, a = 0.9), so twice its
     * length takes that again. */
    rb_engine_desc d;
    preset("i4", &d);
    rb_exhaust_net_legacy(&d, &d.net);
    rb_net_pipe *z = NULL, *m = NULL;
    for (int k = 0; k < d.net.pipes; k++) {
        if (d.net.pipe[k].kind == RB_PIPE_CENTER) z = &d.net.pipe[k];
        if (d.net.pipe[k].kind == RB_PIPE_MUFFLER) m = &d.net.pipe[k];
    }
    CHECK(z != NULL && m != NULL);
    if (!z || !m) return;
    z->role = RB_NET_ROLE_PIPE;
    z->diameter = d.exhaust_pipe_diameter;
    z->packing = 0.0;
    z->muffler = RB_MUFFLER_NONE;
    const double pipe = 0.25 * 3.14159265358979323846 * d.exhaust_pipe_diameter * d.exhaust_pipe_diameter;
    const double tail = 0.25 * 3.14159265358979323846 * d.tailpipe_diameter * d.tailpipe_diameter;
    double share[2], tau[2];
    const double bore[2] = { 0.12, 0.18 };
    for (int j = 0; j < 2; j++) {
        m->diameter = bore[j];
        const double ratio = 0.25 * 3.14159265358979323846 * bore[j] * bore[j] / sqrt(pipe * tail);
        tau[j] = 1.0 / sqrt(1.0 + 0.25 * (ratio - 1.0 / ratio) * (ratio - 1.0 / ratio))
               * walls(bore[j], m->length);   /* and its walls' loss, a wider can's less */
        share[j] = share_of(&d);
    }
    printf("    a %.0f cm can's share over a %.0f cm's: %.4f (theory %.4f)\n", 100 * bore[1], 100 * bore[0],
           share[1] / share[0], sqrt(tau[1] / tau[0]));
    CHECK_NEAR(share[1] / share[0], sqrt(tau[1] / tau[0]), 1e-12);
    /* the muffler packed: a straight-through, as long and then twice */
    m->diameter = d.tailpipe_diameter;
    m->packing = 1500.0;
    const double one = share_of(&d);
    m->length *= 2.0;
    const double two = share_of(&d);
    const double per = 1.05 * pow(0.9, 1.4) * 4.0 / m->diameter * 0.5 * m->length;   /* dB */
    const double want = sqrt(pow(10.0, -0.1 * per) * walls(m->diameter, 0.5 * m->length));
    printf("    its straight-through twice as long: the share %.4f of what it was (Piening %.4f, %.1f dB more)\n",
           two / one, want, per);
    CHECK_NEAR(two / one, want, 1e-12);
}

void test_net_polo_turbine(void)
{
    /* Stage 145: the Polo 1.0 TSI's turbine and wastegate (exhaust_models.c,
     * ea211_polo), two narrowed ways out of the manifold's volume into the
     * turbine's outlet. Its books balance (motored wide open); both ways
     * carry the gas, in the share of their areas within a quarter (the same
     * pressure drop across both, each flowing as its effective area); and the
     * turbine's nozzle holds the manifold above its outlet. Off boost the
 * wastegate is open: it carries at least 0.4 of the turbine's flow. */
    rb_engine_desc d;
    preset("polo-tsi", &d);
    CHECK_EQ_INT(rb_exhaust_net_check(&d.net, &d), 0);
    books(&a, &d, "Polo 1.0 TSI");
    double turbine = 0.0, gate = 0.0, drop = 0.0;
    for (long s = 0; s < 7680; s++) {
        rb_engine_step(&a);
        turbine += a.xpipe[0].end_face[0].mdot;
        gate += a.xpipe[1].end_face[0].mdot;
        drop += a.xnode[0].p - a.xnode[1].p;
    }
    const double share = gate / turbine, want = a.net.pipe[1].end[0].area / a.net.pipe[0].end[0].area;
    printf("    the wastegate carries %.2f of the turbine's flow (their areas %.2f); "
           "the manifold %.1f kPa over the turbine's outlet\n", share, want, 1e-3 * drop / 7680);
    CHECK(turbine > 0.0 && gate > 0.0);
    CHECK_NEAR(share, want, 0.25 * want);
    CHECK(share > 0.4);   /* the wastegate open off boost (VW's strategy, ea211_polo) */
    CHECK(drop / 7680 > 500.0);
}
