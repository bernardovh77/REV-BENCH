"""The app's packaging glue (stage 124): tools/package_app.py, tools/build_addon.py, the CI workflow.
Nothing here builds a package; app/tests/packaging.test.ts checks the config's own values and
package_app.py checks a real package."""
import re
import unittest

import build_addon
import package_app
import run_tests

CONFIG = (run_tests.ROOT / "app" / "electron-builder.config.cjs").read_text(encoding="utf-8")
CI = (run_tests.ROOT / ".github" / "workflows" / "ci.yml").read_text(encoding="utf-8")


def config_fuses():
    block = re.search(r"electronFuses:\s*\{(.*?)\}", CONFIG, re.S).group(1)
    return {name: value == "true" for name, value in re.findall(r"(\w+):\s*(true|false)", block)}


class PackageToolTests(unittest.TestCase):
    def test_the_fuses_the_tool_checks_are_the_fuses_the_config_sets(self):
        # electron-fuses names them in CamelCase, the config in camelCase
        wanted = {name[0].lower() + name[1:]: value == "Enabled" for name, value in package_app.EXPECTED_FUSES.items()}
        self.assertEqual(wanted, config_fuses())

    def test_the_add_on_is_built_where_the_engine_process_looks_in_development(self):
        loader = (run_tests.ROOT / "app" / "src" / "engine" / "addon-loader.ts").read_text(encoding="utf-8")
        self.assertIn('"native", "build", ADDON_FILE', loader)
        tool = (run_tests.ROOT / "tools" / "build_addon.py").read_text(encoding="utf-8")
        self.assertIn('run_tests.ROOT / "app" / "native" / "build"', tool)
        self.assertTrue(callable(build_addon.main))

    def test_the_add_on_build_is_pinned_in_one_place(self):
        package = (run_tests.ROOT / "app" / "package.json").read_text(encoding="utf-8")
        self.assertNotIn("cmake-js", package)
        self.assertNotIn("node-api-headers", package)
        tools = (run_tests.ROOT / "app" / "native" / "build-tools" / "package.json").read_text(encoding="utf-8")
        self.assertRegex(tools, r'"cmake-js": "\d+\.\d+\.\d+"')
        self.assertRegex(tools, r'"node-api-headers": "\d+\.\d+\.\d+"')

    def test_the_package_takes_the_add_on_the_tool_built(self):
        self.assertIn("process.env.RB_NATIVE_PATH", CONFIG)
        self.assertIn("RB_NATIVE_PATH", (run_tests.ROOT / "tools" / "package_app.py").read_text(encoding="utf-8"))


class WorkflowTests(unittest.TestCase):
    def job(self, name):
        start = CI.index(f"\n  {name}:\n")
        end = re.search(r"\n  [\w-]+:\n", CI[start + 1:])
        return CI[start:start + 1 + end.start()] if end else CI[start:]

    def test_every_action_is_pinned_to_a_commit(self):
        for uses in re.findall(r"uses:\s*(\S+)", CI):
            self.assertRegex(uses, r"@[0-9a-f]{40}$", uses)

    def test_windows_builds_and_starts_the_installer_and_keeps_the_packages(self):
        windows = self.job("windows")
        self.assertIn("package_app.py --install-check", windows)
        self.assertIn("Rev-Bench-Setup-*.exe", windows)
        self.assertIn("-win-x64.zip", windows)
        self.assertNotIn("--skip e2e app-e2e", windows)   # so it runs the addon, app and app-e2e suites

    def test_the_upload_takes_the_packages_from_where_package_app_writes_them(self):
        """CI's second run built the installer into ~/rb-build/tests/package and uploaded from
        ~/rb-build/package: no files. The step names its folder, and the upload uses the same."""
        windows = self.job("windows")
        out = re.search(r"package_app\.py --install-check --out \"\$RUNNER_TEMP/([\w-]+)\"", windows)
        self.assertIsNotNone(out, "the packaging step names its folder in $RUNNER_TEMP")
        for name in ("Rev-Bench-Setup-*.exe", "Rev-Bench-*-win-x64.zip"):
            self.assertIn("${{ runner.temp }}/%s/release/%s" % (out.group(1), name), windows)

    def test_linux_runs_the_add_on_and_the_interface_but_not_the_electron_test(self):
        linux = self.job("linux")
        self.assertIn("--skip e2e app-e2e", linux)
        self.assertNotIn("package_app", linux)


if __name__ == "__main__":
    unittest.main()
