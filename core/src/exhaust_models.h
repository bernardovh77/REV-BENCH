#ifndef RB_EXHAUST_MODELS_H
#define RB_EXHAUST_MODELS_H
#include "engine.h"
/* Sets the exhaust fields of preset `preset`'s description (0-4: the four,
 * the six, the cross-plane V8, the flat-plane V8, the boxer) to its
 * researched system: pipe bores and lengths, centre pipes and links, the
 * junction and the muffler. presets.c calls it last.
 * Primary-source fitment and uncertainty ledger: docs/stage-13b-p.md.
 * These are 1D research configurations, not exact manufacturer replicas. */
void rb_exhaust_model(rb_engine_desc *d, int preset);
#endif
