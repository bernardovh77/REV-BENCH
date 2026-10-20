#!/usr/bin/env python3
"""
plant_bugs.py - checks that the tests can fail: plants a stage's bugs one at a
time in a copy of the sources and runs the tests that should catch each.

    python3 tools/plant_bugs.py STAGE WORKDIR NAME...    (or NAME = all)
    python3 tools/plant_bugs.py STAGE --list

STAGE picks the table of bugs, tools/bugs/stageSTAGE.py (12, 13, ...). For
every named bug: copies the sources (without web/, build output, renders/,
docs/ and .git) into WORKDIR, fresh each time, applies the bug (a text
replacement that must match exactly once), builds the test binary there and
runs the tests named for it. A bug is caught when that run fails. Prints a
line per bug. Two work dirs run two bugs at once; a killed run leaves its bug
only in its copy.

A table is a dict BUGS of
    name: (file, old text, new text, [tests], timeout in s)
where old and new may be tuples, several replacements making one bug.
"""
import importlib.util
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.dirname(HERE)


def load_table(stage):
    path = os.path.join(HERE, "bugs", f"stage{stage}.py")
    if not os.path.exists(path):
        sys.exit(f"plant_bugs: no table {path}")
    spec = importlib.util.spec_from_file_location(f"stage{stage}", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod.BUGS


def run(bugs, work, name):
    path, old, new, tests, timeout = bugs[name]
    if os.path.exists(work):
        shutil.rmtree(work)
    shutil.copytree(SRC, work, ignore=shutil.ignore_patterns("web", "build*", "renders", ".git", "docs"))
    target = os.path.join(work, path)
    with open(target) as f:
        text = f.read()
    pairs = list(zip(old, new)) if isinstance(old, tuple) else [(old, new)]
    for o, n in pairs:   # several replacements in one file make one bug
        if text.count(o) != 1:
            return f"{name}: NOT APPLIED ({text.count(o)} matches in {path})"
        text = text.replace(o, n)
    with open(target, "w") as f:
        f.write(text)
    build = os.path.join(work, "b")
    r = subprocess.run(["make", "-s", "-C", work, f"BUILD={build}", f"{build}/rb_tests"],
                       capture_output=True, text=True)
    if r.returncode != 0:
        return f"{name}: BUILD FAILED\n{r.stderr[-2000:]}"
    try:
        r = subprocess.run([f"{build}/rb_tests"] + tests, capture_output=True, text=True,
                           timeout=timeout, env=dict(os.environ, RBT_TMPDIR=build))
    except subprocess.TimeoutExpired:
        return f"{name}: caught (timed out after {timeout} s)"
    failed = [l.split()[-1] for l in r.stdout.splitlines() if l.strip().startswith("FAIL  ")]
    if r.returncode != 0:
        return f"{name}: caught by {', '.join(failed) or 'a crash'}"
    return f"{name}: MISSED ({', '.join(tests)} all pass)"


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    bugs = load_table(sys.argv[1])
    if sys.argv[2] == "--list":
        for name, (path, _, _, tests, _) in bugs.items():
            print(f"{name:28s} {path:24s} {', '.join(tests)}")
        return
    work, names = sys.argv[2], sys.argv[3:]
    if names == ["all"]:
        names = list(bugs)
    for name in names:
        if name not in bugs:
            print(f"{name}: unknown bug")
            continue
        print(run(bugs, work, name), flush=True)


if __name__ == "__main__":
    main()
