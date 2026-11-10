#!/usr/bin/env python3
"""Builds the add-on where the app looks for it in development (stage 124).

    python tools/build_addon.py [--out FOLDER]

The one convention for rb_native.node: the add-on build always writes <build folder>/rb_native.node.
  development   app/native/build/rb_native.node  (this tool; git-ignored; the engine process looks
                there after $RB_NATIVE_PATH, src/engine/addon-loader.ts)
  tests         <out>/addon-build/rb_native.node  (tools/run_tests.py addon and app-e2e; they set
                RB_NATIVE_PATH)
  a package     <resources>/rb_native.node  (app/electron-builder.config.cjs copies the built file
                there, outside the asar; the loader's second place)
cmake-js and node-api-headers are pinned in one place only, app/native/build-tools (with their own
lock), and installed outside the repo, in <out>/addon-node ($RB_TESTS_OUT or ~/rb-build/tests),
never inside it (CLAUDE.md). Needs node, npm, cmake and ninja (docs/workspace.md), and on Windows
Visual Studio's C/C++ tools (MSVC, through tools/msvc_env.py).
"""
import argparse
import subprocess
import sys

import run_tests


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", default=str(run_tests.ROOT / "app" / "native" / "build"),
                        help="the build folder (default app/native/build)")
    args = parser.parse_args()
    env = run_tests.node_env()
    problems = run_tests.preflight(["addon"], env)
    if problems:
        sys.exit("\n".join(f"build_addon: {p}" for p in problems))
    run_tests.OUT.mkdir(parents=True, exist_ok=True)
    for command in run_tests.addon_build_commands(env, build=args.out):
        if subprocess.run(command, cwd=run_tests.ROOT, env=env).returncode:
            sys.exit(f"build_addon: {command[0]} failed")
    print(f"built {args.out}/rb_native.node")


if __name__ == "__main__":
    main()
