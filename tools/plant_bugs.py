#!/usr/bin/env python3
"""
plant_bugs.py - checks that the tests can fail: plants a stage's bugs one at a
time in a copy of the sources and runs the tests that should catch each.

    python3 tools/plant_bugs.py STAGE WORKDIR NAME...    (or NAME = all)
    python3 tools/plant_bugs.py STAGE --list
    python3 tools/plant_bugs.py [STAGE] --check

--check reports every bug, in STAGE's table or in all of tools/bugs/, whose
text no longer matches exactly once in today's sources (a stale bug, which
would plant nothing), and exits 1 if there is one. The Python suite runs it.

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
import glob
import importlib.util
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.dirname(HERE)


def load_file(path):
    """The BUGS dict of the table at path."""
    name = os.path.splitext(os.path.basename(path))[0]
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod.BUGS


def load_table(stage):
    path = os.path.join(HERE, "bugs", f"stage{stage}.py")
    if not os.path.exists(path):
        sys.exit(f"plant_bugs: no table {path}")
    return load_file(path)


def stale_reason(root, bug):
    """Why a bug (file, old, new, ...) wouldn't apply to the sources under
    root, or None if it would: each old text must match exactly once, in turn."""
    path, old, new = bug[0], bug[1], bug[2]
    target = os.path.join(root, path)
    if not os.path.exists(target):
        return f"{path} is gone"
    with open(target, encoding="utf-8") as f:
        text = f.read()
    pairs = list(zip(old, new)) if isinstance(old, tuple) else [(old, new)]
    for o, n in pairs:
        count = text.count(o)
        if count != 1:
            return f"{count} matches in {path} for {o[:50]!r}"
        text = text.replace(o, n)
    return None


def stale_bugs(root, tables, stage=None):
    """(table, bug, reason) for every stale bug in the tables directory
    (stage's table only, if given)."""
    pattern = f"stage{stage}.py" if stage else "stage*.py"
    found = []
    for path in sorted(glob.glob(os.path.join(str(tables), pattern))):
        table = os.path.splitext(os.path.basename(path))[0]
        for name, bug in load_file(path).items():
            reason = stale_reason(str(root), bug)
            if reason:
                found.append((table, name, reason))
    return found


def check(stage=None):
    """--check: prints the stale bugs; 1 if there are any."""
    stale = stale_bugs(SRC, os.path.join(HERE, "bugs"), stage)
    for table, name, reason in stale:
        print(f"{table}: {name}: STALE ({reason})")
    print(f"plant_bugs: {len(stale)} stale bugs")
    return 1 if stale else 0


def run(bugs, work, name):
    path, old, new, tests, timeout = bugs[name]
    if os.path.exists(work):
        shutil.rmtree(work)
    shutil.copytree(SRC, work, ignore=shutil.ignore_patterns(
        "web", "build*", "renders", ".git", "docs", ".venv*", "venv*",
        "__pycache__", "graphify-out", ".agents", "skills-lock.json",
        "*.zip", "*.mkv", "*.webm", "*.mp3", "*.mp4", "*.mov", "*.wav", "*.flac"))
    target = os.path.join(work, path)
    with open(target, encoding="utf-8") as f:
        text = f.read()
    pairs = list(zip(old, new)) if isinstance(old, tuple) else [(old, new)]
    for o, n in pairs:   # several replacements in one file make one bug
        if text.count(o) != 1:
            return f"{name}: NOT APPLIED ({text.count(o)} matches in {path})"
        text = text.replace(o, n)
    with open(target, "w", encoding="utf-8") as f:
        f.write(text)
    build = os.path.join(work, "b")
    # forward slashes: make's shell would eat a Windows path's backslashes
    posix = build.replace(os.sep, "/")
    r = subprocess.run(["make", "-s", "-C", work, f"BUILD={posix}", f"{posix}/rb_tests"],
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
    if sys.argv[1:] == ["--check"]:
        sys.exit(check())
    if len(sys.argv) == 3 and sys.argv[2] == "--check":
        sys.exit(check(sys.argv[1]))
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
