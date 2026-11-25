/*
 * exhaust_net.c - the exhaust as a network (stage 138, engine.h): the
 * legacy fields compiled into one, a network's checks, and the paths through
 * it from each cylinder to each outlet (the noise layer's routing).
 */
#include "engine.h"
#include "rb.h"

#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846

/* An end: nothing given is closed, so these only set what matters. */
static rb_net_end node_end(int k, double entry)
{
    return (rb_net_end){ .kind = RB_NET_NODE, .index = k, .entry_loss = entry };
}

static rb_net_end into_end(int m, int end)
{
    return (rb_net_end){ .kind = RB_NET_INTO, .index = m, .end = end };
}

static rb_net_end onward_end(int m, int back)
{
    return (rb_net_end){ .kind = RB_NET_ONWARD, .index = m, .back = back };
}

/* The legacy muffler's width (engine_build.c's muffler_width, the same). */
static double legacy_muffler_width(const rb_engine_desc *d)
{
    if (d->muffler_type == RB_MUFFLER_CHAMBERED)
        return sqrt(4.0 * d->muffler_volume / (PI * d->muffler_length));
    return d->tailpipe_diameter;
}

/* The audio frame's two tailpipes (audio.c's TAILPIPE): x right of the
 * middle, y forwards from the bumper, z up from the ground. */
static const double LEGACY_OUTLET[2][3] = { { 0.5, -0.05, 0.35 }, { -0.5, -0.05, 0.35 } };

void rb_exhaust_net_legacy(const rb_engine_desc *d, rb_exhaust_net *n)
{
    memset(n, 0, sizeof *n);
    int collectors = 0;
    for (int i = 0; i < d->cylinders; i++)
        if (d->collector[i] + 1 > collectors) collectors = d->collector[i] + 1;
    const int systems = d->exhaust_systems;
    const int extended = d->center_length > 0.0;
    const int straight = d->muffler_type != RB_MUFFLER_CHAMBERED;
    const int alone = systems == collectors;   /* each muffler has one exhaust pipe */
    const int junction = d->junction_volume > 0.0;
    const int x_first = junction && d->junction_before_center;

    /* the volumes: each collector (a cone about two pipe diameters long),
     * then the junction */
    const double header_area = 0.25 * PI * d->header_diameter * d->header_diameter;
    const double pipe_area = 0.25 * PI * d->exhaust_pipe_diameter * d->exhaust_pipe_diameter;
    for (int c = 0; c < collectors; c++) {
        int headers = 0;
        for (int i = 0; i < d->cylinders; i++) headers += (d->collector[i] == c);
        n->node[c] = (rb_net_node){ d->exhaust_pipe_diameter * (headers * header_area + pipe_area),
                                    RB_PLACE_COLLECTOR, c };
    }
    n->nodes = collectors;
    const int J = n->nodes;
    if (junction) n->node[n->nodes++] = (rb_net_node){ d->junction_volume, RB_PLACE_JUNCTION, 0 };
    for (int i = 0; i < d->cylinders; i++) n->header_to[i] = node_end(d->collector[i], RB_HEADER_ENTRY);

    /* the pipes, in the engine's old order: exhaust pipes, tailpipes,
     * mufflers, (the snorkel), centre pipes, links */
    const int X = 0, T = collectors, M = collectors + systems, Z = collectors + 2 * systems,
              L = Z + collectors;
    n->pipes = Z + (extended ? 2 * collectors : 0);
    n->snorkel_at = Z;
    for (int c = 0; c < collectors; c++) {
        rb_net_pipe *x = &n->pipe[X + c];
        const int m = M + ((systems == 2) ? c : 0);
        *x = (rb_net_pipe){ .role = RB_NET_ROLE_PIPE, .kind = RB_PIPE_EXHAUST, .number = c,
                            .from = RB_PLACE_COLLECTOR, .from_number = c,
                            .to = RB_PLACE_MUFFLER, .to_number = (systems == 2) ? c : 0,
                            .length = d->exhaust_pipe_length, .diameter = d->exhaust_pipe_diameter,
                            .catalyst = 1.0 };
        x->end[0] = node_end(c, RB_COLLECTOR_ENTRY);
        if (extended) {
            rb_net_pipe *z = &n->pipe[Z + c], *l = &n->pipe[L + c];
            const double width = d->center_volume > 0.0
                ? sqrt(4.0 * d->center_volume / (PI * d->center_length)) : d->center_diameter;
            x->to = x_first ? RB_PLACE_JUNCTION : RB_PLACE_CENTER;
            x->to_number = x_first ? 0 : c;
            *z = (rb_net_pipe){ .role = RB_NET_ROLE_CAN, .kind = RB_PIPE_CENTER, .number = c,
                                .from = x->to, .from_number = x->to_number,
                                .to = RB_PLACE_CENTER_OUTLET, .to_number = c,
                                .length = d->center_length, .diameter = width,
                                .packing = d->center_packing, .wall = RB_EXHAUST_WALL,
                                .muffler = (d->center_packing > 0.0) ? RB_MUFFLER_STRAIGHT
                                         : (d->center_volume > 0.0) ? RB_MUFFLER_CHAMBERED
                                                                    : RB_MUFFLER_NONE };
            *l = (rb_net_pipe){ .role = RB_NET_ROLE_PIPE, .kind = RB_PIPE_LINK, .number = c,
                                .from = RB_PLACE_CENTER_OUTLET, .from_number = c,
                                .to = (junction && !x_first) ? RB_PLACE_JUNCTION : RB_PLACE_MUFFLER,
                                .length = d->link_length, .diameter = d->exhaust_pipe_diameter };
            l->to_number = (l->to == RB_PLACE_JUNCTION || systems == 1) ? 0 : c;
            if (x_first) {
                x->end[1] = node_end(J, 0.0);
                z->end[0] = node_end(J, 0.0);
            } else {
                x->end[1] = (d->center_volume > 0.0) ? into_end(Z + c, 0) : onward_end(Z + c, 0);
            }
            if (d->center_volume > 0.0) l->end[0] = into_end(Z + c, 1);
            else z->end[1] = onward_end(L + c, 1);
            if (junction && !x_first) l->end[1] = node_end(J, 0.0);
            else l->end[1] = (straight && alone) ? onward_end(m, 0) : into_end(m, 0);
        } else {
            x->end[1] = (straight && alone) ? onward_end(m, 0) : into_end(m, 0);
        }
        x->end[1].area = d->catalyst_area;
        x->end[1].laminar = d->catalyst_laminar;
    }
    for (int s = 0; s < systems; s++) {
        rb_net_pipe *t = &n->pipe[T + s], *m = &n->pipe[M + s];
        *m = (rb_net_pipe){ .role = RB_NET_ROLE_CAN, .kind = RB_PIPE_MUFFLER, .number = s,
                            .from = RB_PLACE_MUFFLER, .from_number = s,
                            .to = RB_PLACE_MUFFLER_OUTLET, .to_number = s,
                            .length = d->muffler_length, .diameter = legacy_muffler_width(d),
                            .packing = (d->muffler_type == RB_MUFFLER_STRAIGHT) ? RB_PACKING : 0.0,
                            .muffler = d->muffler_type };
        *t = (rb_net_pipe){ .role = RB_NET_ROLE_TAILPIPE, .kind = RB_PIPE_TAILPIPE, .number = s,
                            .from = RB_PLACE_MUFFLER_OUTLET, .from_number = s,
                            .to = RB_PLACE_AIR, .to_number = 0,
                            .length = d->tailpipe_length, .diameter = d->tailpipe_diameter };
        if (junction && !x_first) {
            m->end[0] = node_end(J, 0.0);
            m->from = RB_PLACE_JUNCTION, m->from_number = 0;
        }
        if (straight) {
            m->end[1] = onward_end(T + s, 1);
        } else {
            t->end[0] = into_end(M + s, 1);
            t->end[0].area = d->muffler_area;
            t->end[0].laminar = RB_BAFFLE_LAMINAR;
        }
        t->end[1] = (rb_net_end){ .kind = RB_NET_OUTLET, .index = s, .entry_loss = RB_TAILPIPE_ENTRY };
        memcpy(n->outlet[s].at, LEGACY_OUTLET[s], sizeof n->outlet[s].at);
    }
    n->outlets = systems;
}

/* ---- checks ---- */

/* 1 if end e of pipe k (in a network of n) opens somewhere it can. */
static int end_ok(const rb_exhaust_net *n, int k, int e, int cylinders)
{
    const rb_net_end *x = (k < 0) ? NULL : &n->pipe[k].end[e];
    (void)cylinders;
    if (!x) return 0;
    if (!(x->area >= 0.0 && x->area < 0.1) || !(x->laminar >= 0.0 && x->laminar <= 10000.0)
        || !(x->entry_loss >= 0.0 && x->entry_loss <= 10.0)) return 0;
    switch (x->kind) {
    case RB_NET_CLOSED: return 1;
    case RB_NET_NODE:   return x->index >= 0 && x->index < n->nodes;
    case RB_NET_INTO:   return x->index >= 0 && x->index < n->pipes && x->index != k
                               && (x->end == 0 || x->end == 1);
    case RB_NET_TAP:    return x->index >= 0 && x->index < n->pipes && x->index != k
                               && x->at >= 0.0 && x->at <= 1.0;
    case RB_NET_ONWARD: return e == 1 && x->index >= 0 && x->index < n->pipes && x->index != k
                               && (x->back == 0 || x->back == 1);
    case RB_NET_OUTLET: return e == 1 && x->index >= 0 && x->index < n->outlets;   /* the flow out
                                                                         is end 1's */
    default:            return 0;
    }
}

/* Where a gas parcel at pipe k's end e can go next, as a graph: nodes are
 * the network's volumes (0..nodes-1) and the pipes' two ends
 * (nodes + 2 k + e); outlets are reached from an OUTLET end. Fills to[] with
 * the places reached from `from` and their distances (m), returns how many. */
#define PLACES (RB_NET_NODES + 2 * RB_NET_PIPES)

static int place_of_end(const rb_exhaust_net *n, int k, int e)
{
    return n->nodes + 2 * k + e;
}

/* The place an end opens into (a pipe end's place counts as the cell: from
 * there the gas goes along that pipe). -1 for closed or an outlet. */
static int opens_into(const rb_exhaust_net *n, const rb_net_end *x, int *outlet)
{
    *outlet = -1;
    switch (x->kind) {
    case RB_NET_NODE:   return x->index;
    case RB_NET_INTO:   return place_of_end(n, x->index, x->end);
    case RB_NET_ONWARD: return place_of_end(n, x->index, 0);
    case RB_NET_TAP:    return place_of_end(n, x->index, x->at < 0.5 ? 0 : 1);
    case RB_NET_OUTLET: *outlet = x->index; return -1;
    default:            return -1;
    }
}

/* Edges, both ways: each pipe joins its two ends (its length: edge `pipe`
 * is that pipe; -1 for the rest); each end joins where it opens (0 m). A TAP
 * joins its pipe end to both ends of the tapped pipe, at the tap's share of
 * its length. */
#define EDGES (PLACES * 4)
typedef struct net_graph {
    int    places, count;
    int    to[EDGES][2], pipe[EDGES];
    double len[EDGES];
} net_graph;

static void add_edge(net_graph *g, int a, int b, double len, int pipe)
{
    if (g->count >= EDGES) return;
    g->to[g->count][0] = a, g->to[g->count][1] = b;
    g->len[g->count] = len, g->pipe[g->count] = pipe;
    g->count++;
}

static void graph_of(const rb_exhaust_net *n, net_graph *g)
{
    g->places = n->nodes + 2 * n->pipes;
    g->count = 0;
    for (int k = 0; k < n->pipes; k++) {
        const rb_net_pipe *p = &n->pipe[k];
        add_edge(g, place_of_end(n, k, 0), place_of_end(n, k, 1), p->length, k);
        for (int e = 0; e < 2; e++) {
            const rb_net_end *x = &p->end[e];
            int outlet;
            if (x->kind == RB_NET_TAP) {
                const double l = n->pipe[x->index].length;
                add_edge(g, place_of_end(n, k, e), place_of_end(n, x->index, 0), x->at * l, -1);
                add_edge(g, place_of_end(n, k, e), place_of_end(n, x->index, 1), (1.0 - x->at) * l, -1);
                continue;
            }
            const int into = opens_into(n, x, &outlet);
            if (into >= 0) add_edge(g, place_of_end(n, k, e), into, 0.0, -1);
        }
    }
}

/* The shortest way from `start` (already `start_dist` m along) to every
 * place: dist[], and the edge it came in by, prev[] (-1 at the start). */
static void shortest(const net_graph *g, int start, double start_dist, double dist[PLACES],
                     int prev[PLACES])
{
    int done[PLACES] = { 0 };
    for (int q = 0; q < g->places; q++) dist[q] = INFINITY, prev[q] = -1;
    dist[start] = start_dist;
    for (;;) {   /* Dijkstra, the plain way: a few dozen places */
        int q = -1;
        for (int r = 0; r < g->places; r++)
            if (!done[r] && dist[r] < INFINITY && (q < 0 || dist[r] < dist[q])) q = r;
        if (q < 0) break;
        done[q] = 1;
        for (int k = 0; k < g->count; k++) {
            const int a = g->to[k][0], b = g->to[k][1];
            if (a == q && dist[q] + g->len[k] < dist[b]) dist[b] = dist[q] + g->len[k], prev[b] = k;
            if (b == q && dist[q] + g->len[k] < dist[a]) dist[a] = dist[q] + g->len[k], prev[a] = k;
        }
    }
}

/* Each outlet's distance (-1: not reached), and the place its pipe end is. */
static int outlets_reached(const rb_exhaust_net *n, const double dist[PLACES],
                           double length[RB_NET_OUTLETS], int at[RB_NET_OUTLETS])
{
    int reached = 0;
    for (int o = 0; o < RB_NET_OUTLETS; o++) length[o] = -1.0, at[o] = -1;
    for (int k = 0; k < n->pipes; k++)
        for (int e = 0; e < 2; e++) {
            const rb_net_end *x = &n->pipe[k].end[e];
            if (x->kind != RB_NET_OUTLET) continue;
            const int q = place_of_end(n, k, e);
            if (dist[q] < INFINITY && (length[x->index] < 0.0 || dist[q] < length[x->index])) {
                if (length[x->index] < 0.0) reached++;
                length[x->index] = dist[q], at[x->index] = q;
            }
        }
    return reached;
}

/* The pipes along the way to place q (each once, outermost last). */
static int route_to(const net_graph *g, const int prev[PLACES], int q, int pipes[RB_NET_PIPES])
{
    int count = 0, guard = 0;
    while (q >= 0 && prev[q] >= 0 && guard++ < EDGES) {
        const int k = prev[q];
        if (g->pipe[k] >= 0 && count < RB_NET_PIPES) pipes[count++] = g->pipe[k];
        q = (g->to[k][0] == q) ? g->to[k][1] : g->to[k][0];
    }
    return count;
}

int rb_exhaust_net_paths(const rb_exhaust_net *n, const rb_engine_desc *d, int cylinder,
                         double length[RB_NET_OUTLETS])
{
    net_graph g;
    double dist[PLACES];
    int prev[PLACES], at[RB_NET_OUTLETS], outlet;
    for (int o = 0; o < RB_NET_OUTLETS; o++) length[o] = -1.0;
    if (cylinder < 0 || cylinder >= d->cylinders) return 0;
    const int start = opens_into(n, &n->header_to[cylinder], &outlet);
    if (start < 0) return 0;
    graph_of(n, &g);
    shortest(&g, start, d->header_length[cylinder], dist, prev);
    return outlets_reached(n, dist, length, at);
}

int rb_exhaust_net_route(const rb_exhaust_net *n, const rb_engine_desc *d, int cylinder, int pipe,
                         int end, double length[RB_NET_OUTLETS],
                         int route[RB_NET_OUTLETS][RB_NET_PIPES], int count[RB_NET_OUTLETS])
{
    net_graph g;
    double dist[PLACES];
    int prev[PLACES], at[RB_NET_OUTLETS], outlet, start;
    for (int o = 0; o < RB_NET_OUTLETS; o++) length[o] = -1.0, count[o] = 0;
    if (cylinder >= 0) {
        if (cylinder >= d->cylinders) return 0;
        start = opens_into(n, &n->header_to[cylinder], &outlet);
    } else {
        start = (pipe >= 0 && pipe < n->pipes && (end == 0 || end == 1)) ? place_of_end(n, pipe, end) : -1;
    }
    if (start < 0) return 0;
    graph_of(n, &g);
    shortest(&g, start, cylinder >= 0 ? d->header_length[cylinder] : 0.0, dist, prev);
    const int reached = outlets_reached(n, dist, length, at);
    for (int o = 0; o < RB_NET_OUTLETS; o++)
        if (at[o] >= 0) count[o] = route_to(&g, prev, at[o], route[o]);
    return reached;
}

int rb_exhaust_net_check(const rb_exhaust_net *n, const rb_engine_desc *d)
{
    if (!(n->pipes >= 1 && n->pipes <= RB_NET_PIPES && n->nodes >= 0 && n->nodes <= RB_NET_NODES
          && n->outlets >= 1 && n->outlets <= RB_NET_OUTLETS
          && n->snorkel_at >= 0 && n->snorkel_at <= n->pipes)) return -1;
    for (int k = 0; k < n->nodes; k++)
        if (!(n->node[k].volume > 1e-7 && n->node[k].volume <= 0.05)) return -1;
    for (int k = 0; k < n->pipes; k++) {
        const rb_net_pipe *p = &n->pipe[k];
        if (!(p->role >= RB_NET_ROLE_PIPE && p->role <= RB_NET_ROLE_HEADER)) return -1;
        if (!(p->length >= 0.02 && p->length <= 10.0 && p->diameter > 0.005 && p->diameter < 0.6))
            return -1;
        if (!(p->packing >= 0.0 && p->packing <= 20000.0) || !(p->wall >= 0.0 && p->wall < 1500.0)
            || !(p->muffler >= RB_MUFFLER_NONE && p->muffler <= RB_MUFFLER_CHAMBERED))
            return -1;
        if (!end_ok(n, k, 0, d->cylinders) || !end_ok(n, k, 1, d->cylinders)) return -1;
    }
    /* each outlet opened by exactly one pipe end */
    for (int o = 0; o < n->outlets; o++) {
        int users = 0;
        for (int k = 0; k < n->pipes; k++)
            for (int e = 0; e < 2; e++)
                users += n->pipe[k].end[e].kind == RB_NET_OUTLET && n->pipe[k].end[e].index == o;
        if (users != 1) return -1;
    }
    /* every header opens into a volume or a pipe, and reaches an outlet */
    for (int i = 0; i < d->cylinders; i++) {
        const rb_net_end *x = &n->header_to[i];
        if (!(x->kind == RB_NET_NODE || x->kind == RB_NET_INTO || x->kind == RB_NET_TAP)) return -1;
        if (x->kind == RB_NET_NODE && !(x->index >= 0 && x->index < n->nodes)) return -1;
        if (x->kind != RB_NET_NODE && !(x->index >= 0 && x->index < n->pipes)) return -1;
        double length[RB_NET_OUTLETS];
        if (rb_exhaust_net_paths(n, d, i, length) < 1) return -1;
    }
    return 0;
}
