"""Stage 119's planted bugs for the Python tools' tests.

python tools/bugs/stage119_python.py [NAME...]
Each bug is applied to a fresh copy of tools/ (with the repo's contracts/ and
the files the tests read), and the named test runs there. A bug is caught
when that test fails. plant_bugs.py does the same for the C tests.
"""
import pathlib
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]

BUGS = {
    # the old publish: new diagnostics written into renders/current/experiment
    # before the page and manifest are, and left there when they fail
    'publish-before-commit': ("tools/checkpoint_sections.py",
        "        following = staged_experiment(renders, Path(diagnostics))",
        "        shutil.copytree(diagnostics, experiment, dirs_exist_ok=True)\n"
        "        following = staged_experiment(renders, Path(diagnostics))",
        "test_checkpoint_sections.CheckpointSectionTests.test_failed_experiment_publish_keeps_its_diagnostics"),
    # the old mono fold
    'mono-fold': ("tools/compare_real.py",
        "    return x, rate\n\n\ndef band",
        "    return x.mean(axis=1, keepdims=True), rate\n\n\ndef band",
        "test_audio_analysis.AudioAnalysisTests.test_stereo_is_measured_channel_by_channel"),
    # the old default: mono unless asked for stereo
    'mono-by-default': ("tools/prepare_user_references.py",
        'channels = recording.get("measurement_channels", 2)',
        'channels = recording.get("measurement_channels", 1)',
        "test_reference_filtering.ReferenceFilteringTests.test_measurements_are_stereo_unless_pinned_to_the_legacy_fold"),
    # the old instant: the first row showing the change, a block late
    'lift-at-row-end': ("tools/checks.py",
        "            return float(rows[index - 1][\"t_s\"]), rows[index - 1]",
        "            return float(row[\"t_s\"]), row",
        "test_checks"),
    # a check that python -O drops
    'assert-check': ("tools/checks.py",
        "    if not ok:\n        raise CheckFailed(what)",
        "    assert ok, what",
        "test_checks.CheckTests.test_require_survives_python_optimisation"),
    # make given the paths as Python spells them (backslashes on Windows)
    'make-backslashes': ("tools/render_results.py",
        'f"--assume-old={make_target(cli)}"',
        'f"--assume-old={cli}"',
        "test_render_results.RenderResultsTests.test_make_gets_forward_slashes"),
}


def run(name):
    path, old, new, test = BUGS[name]
    with tempfile.TemporaryDirectory(prefix="rb-pybug-") as work:
        work = pathlib.Path(work)
        for folder in ("tools", "contracts"):
            shutil.copytree(ROOT / folder, work / folder,
                            ignore=shutil.ignore_patterns("__pycache__"))
        target = work / path
        text = target.read_text(encoding="utf-8")
        if text.count(old) != 1:
            return f"{name}: NOT APPLIED ({text.count(old)} matches in {path})"
        target.write_text(text.replace(old, new), encoding="utf-8", newline="\n")
        if name == "make-backslashes" and "\\" not in str(work):
            return f"{name}: skipped (only Windows paths have backslashes)"
        r = subprocess.run([sys.executable, "-m", "unittest", test], cwd=work / "tools",
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
        if r.returncode:
            return f"{name}: caught by {test.split('.')[-1]}"
        return f"{name}: MISSED ({test} passes)"


if __name__ == "__main__":
    for name in sys.argv[1:] or BUGS:
        print(run(name), flush=True)
