"""Stage 059: the five-engine qualification's frozen envelope (tests/test_qualification.c) can fail.

python tools/plant_bugs.py 059 WORKDIR all
Friction's constant part doubled (every brake figure falls out of its band); the burn's start
varying three times as much (the COV leaves Heywood's band), or a seventh as much (the COV under
its floor).
"""
RATINGS = ["test_qualification_against_published_ratings"]
COV = ["test_qualification_cov_over_seeds"]

BUGS = {
    'friction-doubled': ("core/src/engine_build.c",
        "    d->fmep_constant = 0.38e5;",
        "    d->fmep_constant = 0.76e5;",
        RATINGS, 900),
    'start-varies-triple': ("core/src/engine_parts.h",
        "#define VARY_START   0.07 ",
        "#define VARY_START   0.21 ",
        COV, 900),
    'variation-weak': ("core/src/engine_parts.h",
        "#define VARY_START   0.07 ",
        "#define VARY_START   0.01 ",
        COV, 900),
}
