"""Where FFmpeg is, for the tools and their tests, on Windows and Linux alike.

find_ffmpeg() tries, in order:
1. $RB_REFERENCE_FFMPEG, when set;
2. the static FFmpeg of the imageio-ffmpeg package (pip install imageio-ffmpeg),
   the same on every system;
3. an `ffmpeg` on PATH, unless it is a snap: a snap's FFmpeg can't open files
   in /tmp, where the tests work (Ubuntu's in WSL is one).
It returns None when there is none that runs.
"""
import os
import shutil
import subprocess


def runs(path):
    try:
        return subprocess.run([path, "-version"], capture_output=True, timeout=30).returncode == 0
    except (OSError, subprocess.TimeoutExpired):
        return False


def find_ffmpeg():
    chosen = os.environ.get("RB_REFERENCE_FFMPEG")
    if chosen:
        return chosen if runs(chosen) else None
    try:
        import imageio_ffmpeg
        bundled = imageio_ffmpeg.get_ffmpeg_exe()
        if runs(bundled):
            return bundled
    except (ImportError, RuntimeError):
        pass
    found = shutil.which("ffmpeg")
    if found and not os.path.realpath(found).startswith("/snap/") and runs(found):
        return found
    return None
