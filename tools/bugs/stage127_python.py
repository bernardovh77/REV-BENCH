"""Stage 127's planted bugs for its Python tools and its push hook.

python tools/bugs/stage127_python.py [NAME...]
Each bug is applied to a fresh copy of tools/ and .githooks/ (with the repo's
contracts/), and the named test runs there. A bug is caught when that test fails.

Not planted: the hook's check that a Python it found really runs (the Store's
"python3" stub on Windows). Only a machine with that stub can catch its removal.
"""
import pathlib
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
HOOK = "test_pre_push_hook.PrePushHookTests"
SUITES = "test_run_tests.ChosenSuitesTests"
ONCE = "test_plant_bugs.ReplaceOnceTests"
REPO = "test_repo_root.RepoRootTests"

BUGS = {
    # the push hook
    'hook-lets-a-failure-through': (".githooks/pre-push",
        '    echo "pre-push: a suite failed, so nothing was pushed" >&2\n    exit 1\n',
        '    echo "pre-push: a suite failed, so nothing was pushed" >&2\n    exit 0\n',
        HOOK + ".test_a_failing_suite_refuses_the_push"),
    'hook-tests-a-deleted-branch': (".githooks/pre-push",
        "        0000000000000000000000000000000000000000) ;;\n",
        "        0000000000000000000000000000000000000000) sends=1 ;;\n",
        HOOK + ".test_deleting_a_branch_runs_nothing"),
    'hook-skip-still-runs': (".githooks/pre-push",
        '    echo "pre-push: skipped (RB_SKIP_PREPUSH=1)" >&2\n    exit 0\n',
        '    echo "pre-push: skipped (RB_SKIP_PREPUSH=1)" >&2\n',
        HOOK + ".test_skipping_is_explicit_and_runs_nothing"),
    'hook-ignores-the-chosen-suites': (".githooks/pre-push",
        "suites=${RB_PREPUSH_SUITES:-native python host}",
        'suites="native python host"',
        HOOK + ".test_the_suites_can_be_chosen"),
    'hook-tests-an-empty-push': (".githooks/pre-push",
        '[ "$sends" = "1" ] || exit 0\n',
        "",
        HOOK + ".test_a_push_of_nothing_runs_nothing"),
    # which suites a run does
    'run-tests-e2e-by-default': ("tools/run_tests.py",
        "    wanted = named or [s for s in SUITES if s not in ON_REQUEST]",
        "    wanted = named or list(SUITES)",
        SUITES + ".test_a_plain_run_leaves_out_the_suites_on_request"),
    'run-tests-skip-ignored': ("tools/run_tests.py",
        "    return [s for s in wanted if s not in skipped]",
        "    return list(wanted)",
        SUITES + ".test_skipped_suites_are_left_out"),
    'run-tests-on-request-never-runs': ("tools/run_tests.py",
        "    wanted = named or [s for s in SUITES if s not in ON_REQUEST]",
        "    wanted = [s for s in (named or SUITES) if s not in ON_REQUEST]",
        SUITES + ".test_a_suite_on_request_runs_when_named"),
    # a planted bug's text, found with its spacing changed
    'plant-joins-words': ("tools/plant_bugs.py",
        '    pattern = r"\\s+".join(re.escape(w) for w in words)',
        '    pattern = r"\\s*".join(re.escape(w) for w in words)',
        ONCE + ".test_words_are_not_joined_or_split"),
    'plant-takes-the-first-of-two': ("tools/plant_bugs.py",
        "    if len(found) != 1:\n        return text, len(found)\n",
        "    if not found:\n        return text, 0\n",
        ONCE + ".test_two_places_by_spacing_refuse"),
    'plant-eats-the-next-indent': ("tools/plant_bugs.py",
        '        pattern = pattern + r"[ \\t]*\\n"',
        '        pattern = pattern + r"\\s*"',
        ONCE + ".test_reindented_and_rewrapped_text_is_still_found"),
    'plant-replaces-all-as-written': ("tools/plant_bugs.py",
        "    if count > 1:\n        return text, count\n",
        "    if count > 1:\n        return text.replace(old, new), 1\n",
        ONCE + ".test_two_places_as_written_refuse"),
    'plant-spacing-never-tried': ("tools/plant_bugs.py",
        "    words = old.split()\n    if not words:\n        return text, 0\n",
        "    words = []\n    if not words:\n        return text, 0\n",
        ONCE + ".test_reindented_and_rewrapped_text_is_still_found"),
    # the repo's root
    'root-misses-msys-colon': ("tools/test_repo_root.py",
        'return sorted(n for n in names if re.match("^[A-Za-z][:" + MSYS_COLON + "].", n))',
        'return sorted(n for n in names if re.match("^[A-Za-z][:].", n))',
        REPO + ".test_a_mangled_windows_path_is_recognised"),
}


def run(name):
    path, old, new, test = BUGS[name]
    with tempfile.TemporaryDirectory(prefix="rb-pybug-") as work:
        work = pathlib.Path(work)
        for folder in ("tools", "contracts", ".githooks"):
            shutil.copytree(ROOT / folder, work / folder,
                            ignore=shutil.ignore_patterns("__pycache__"))
        target = work / path
        text = target.read_text(encoding="utf-8")
        if text.count(old) != 1:
            return f"{name}: NOT APPLIED ({text.count(old)} matches in {path})"
        target.write_text(text.replace(old, new), encoding="utf-8", newline="\n")
        r = subprocess.run([sys.executable, "-m", "unittest", test], cwd=work / "tools",
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
        if r.returncode:
            return f"{name}: caught by {test.split('.')[-1]}"
        return f"{name}: MISSED ({test} passes)"


def main():
    names = sys.argv[1:] or list(BUGS)
    unknown = [n for n in names if n not in BUGS]
    if unknown:
        sys.exit(f"stage127_python: unknown bugs {unknown}")
    missed = 0
    for name in names:
        line = run(name)
        print(line, flush=True)
        missed += "caught" not in line
    print(f"{len(names) - missed} of {len(names)} caught")
    sys.exit(1 if missed else 0)


if __name__ == "__main__":
    main()
