"""
layout_shards.py - runs stage 131's drive of every engine layout (tests/test_layouts.c,
test_layout_every_combination_runs) in parallel processes, each its share of the combinations
(RB_LAYOUT_SHARD=k/N), and fails if any share fails. The `layouts` suite of tools/run_tests.py
runs it; by hand:

    python tools/layout_shards.py PATH/TO/rb_tests [--shards N]

Each combination is about a minute of engine and 4 to 10 minutes of computer, so N defaults to
the processor's threads (at most the 28 combinations). Each share's output is printed whole when
it finishes, with how long it took.
"""
import argparse
import os
import subprocess
import sys
import time

COMBINATIONS = 28   # tests/test_layouts.c checks the core offers this many


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
    parser.add_argument("tests", help="the rb_tests program")
    parser.add_argument("--shards", type=int, default=min(os.cpu_count() or 2, COMBINATIONS))
    args = parser.parse_args(argv)
    n = max(1, min(args.shards, COMBINATIONS))
    start = time.monotonic()
    running = []
    for k in range(n):
        env = dict(os.environ, RB_LAYOUT_SHARD=f"{k}/{n}")
        process = subprocess.Popen([args.tests, "test_layout_every_combination_runs"], env=env,
                                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                                   encoding="utf-8", errors="replace")
        running.append((k, process))
    failed = 0
    for k, process in running:
        output, _ = process.communicate()
        took = time.monotonic() - start
        print(f"--- share {k}/{n} ({'pass' if process.returncode == 0 else 'FAIL'}, {took:.0f} s)\n{output}", flush=True)
        failed += process.returncode != 0
    print(f"layout_shards: {n} shares, {failed} failed, {time.monotonic() - start:.0f} s", flush=True)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
