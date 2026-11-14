/*
 * cores.h - which logical processors the render thread keeps to (stage 066's
 * gap, fixed before it). On a hybrid CPU (performance and efficiency cores)
 * the engine runs at about 0.6 times the speed on an efficiency core
 * (docs/stage-118.md), so the render thread asks to stay on the fastest
 * class. Where every core is alike it asks for nothing. Platform-neutral, so
 * its tests run everywhere; platform_win.c applies it.
 */
#ifndef RBH_CORES_H
#define RBH_CORES_H

/* Given each logical processor's efficiency class (higher is faster, as
 * Windows numbers them), writes the indices of those in the highest class
 * to `picked` and returns how many: 0 when there is only one class (no
 * restriction), no processors, or no classes. */
int rbh_pick_fast_cores(const unsigned char *classes, int count, int *picked);

#endif /* RBH_CORES_H */
