"""The repo's root holds only what belongs there.

A Windows path given to a POSIX shell without forward slashes loses its
backslashes, and make then creates "C:UsersbernaAppData...build" inside the
repo (two such folders sat there from 2026-11-01 until stage 127). Tools pass
make forward-slash paths (CLAUDE.md); this fails if one stops doing so."""
import os
import re
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# MSYS writes a colon, which Windows forbids in a name, as this private character.
MSYS_COLON = ""


def mangled_paths(names):
    """The names that are a Windows path with its separators eaten: a drive
    letter and a colon (or MSYS's stand-in for one), then more."""
    return sorted(n for n in names if re.match("^[A-Za-z][:" + MSYS_COLON + "].", n))


class RepoRootTests(unittest.TestCase):
    def test_a_mangled_windows_path_is_recognised(self):
        msys = "C" + MSYS_COLON + "Usersbernabuild"
        names = ["core", "C:UsersbernaAppDataLocalTemptmp3ujjirhlbuild", "d:x", "C", "README.md", msys]
        self.assertEqual(mangled_paths(names),
                         ["C:UsersbernaAppDataLocalTemptmp3ujjirhlbuild", msys, "d:x"])

    def test_the_root_has_no_mangled_windows_path(self):
        self.assertEqual(mangled_paths(os.listdir(ROOT)), [])


if __name__ == "__main__":
    unittest.main()
