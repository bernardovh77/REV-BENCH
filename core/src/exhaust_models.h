#ifndef RB_EXHAUST_MODELS_H
#define RB_EXHAUST_MODELS_H
#include "engine.h"
/* Primary-source fitment and uncertainty ledger: docs/stage-13b-p.md.
 * These are 1D research configurations, not exact manufacturer replicas. */
void rb_exhaust_model(rb_engine_desc *d, int preset);
#endif
