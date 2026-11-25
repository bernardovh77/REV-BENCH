/* Audio-rate propagation of separate engine sources; no allocation in step. */
#ifndef RB_SCENE_H
#define RB_SCENE_H
#include "audio.h"
#include <stdint.h>

#define RB_SCENE_OUTLETS RB_LISTEN_OUTLETS
#define RB_SCENE_PLACED (RB_SCENE_OUTLETS + 2) /* the outlets, mouth, bay   */
#define RB_SCENE_MOUNTS RB_SCENE_PLACED        /* the cabin mounts: unplaced */
#define RB_SCENE_SOURCES (RB_SCENE_PLACED + 1)
#define RB_SCENE_RING 32768
#define RB_ROOM_LINES 4
#define RB_ROOM_RING 8192

typedef struct rb_scene_path {
    double delay, delay_step, gain, gain_step, spread, spread_step;
    double shadow, shadow_step, low;
} rb_scene_path;

typedef struct rb_scene {
    double fs, c, time;
    int kind, tailpipes, pos, geometry_tick;
    double at[RB_SCENE_PLACED][3];              /* where each source is (m)      */
    double diameter[RB_SCENE_OUTLETS];          /* each outlet's opening (m)     */
    float source[RB_SCENE_PLACED][RB_SCENE_RING];
    rb_scene_path path[2][RB_SCENE_PLACED][3]; /* ear, source, direct/ground/wall */
    rb_cabin cabin[2];
    float room[RB_ROOM_LINES][RB_ROOM_RING];
    int room_pos[RB_ROOM_LINES], room_length[RB_ROOM_LINES];
    double room_gain[RB_ROOM_LINES], room_low[RB_ROOM_LINES], room_a, head_a;
} rb_scene;

int rb_scene_init(rb_scene *s, int kind, double fs, double c, int tailpipes, double diameter);
/* The same for `outlets` outlets, outlet o at at[3 o .. 3 o + 2] (m), each
 * diameter[o] wide (stage 138). */
int rb_scene_init_at(rb_scene *s, int kind, double fs, double c, const double at[],
                     const double diameter[], int outlets);
void rb_scene_step(rb_scene *s, const double source[RB_SCENE_SOURCES], double out[2]);
/* Exact retarded propagation time for a constant-velocity source. */
double rb_scene_flight_time(const double relative[3], double velocity, double c);
#endif
