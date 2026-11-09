"""host_parity: rb_cli's drive against the host's offline render of its trace.

The programs are stood in for by a fake `run`: the checker's own decisions are what is tested
(the real programs are rb_cli and rb_host_render, checked by `run_tests.py host`)."""
from pathlib import Path
import io
import tempfile
import unittest
from contextlib import redirect_stdout

import host_parity


class FakePrograms:
    """Stands in for host_parity.run: rb_cli writes a WAV and a trace, rb_host_render a WAV."""

    def __init__(self, cli_wav, host_wav, cli_code=0, host_code=0):
        self.cli_wav, self.host_wav = cli_wav, host_wav
        self.cli_code, self.host_code = cli_code, host_code
        self.calls = []

    def __call__(self, command):
        command = [str(c) for c in command]
        self.calls.append(command)
        if command[0] == "cli":
            Path(command[command.index("-o") + 1]).write_bytes(self.cli_wav)
            Path(command[command.index("--trace") + 1]).write_text("rbtrace 1\nframes 10\n", encoding="utf-8")
            return self.cli_code, ""
        Path(command[command.index("-o") + 1]).write_bytes(self.host_wav)
        return self.host_code, ""


class HostParityTests(unittest.TestCase):
    def check(self, programs, names=("idle-i4-low",)):
        real = host_parity.run
        host_parity.run = programs
        try:
            with tempfile.TemporaryDirectory() as tmp, redirect_stdout(io.StringIO()) as out:
                code = host_parity.check("cli", "host", Path(tmp), list(names))
        finally:
            host_parity.run = real
        return code, out.getvalue()

    def test_identical_renders_pass(self):
        code, text = self.check(FakePrograms(b"RIFF" + bytes(range(200)), b"RIFF" + bytes(range(200))))
        self.assertEqual(code, 0, text)
        self.assertIn("identical", text)

    def test_one_byte_off_is_reported_where_it_is(self):
        a = bytes(range(100))
        b = bytearray(a)
        b[57] ^= 1
        code, text = self.check(FakePrograms(a, bytes(b)))
        self.assertEqual(code, 1)
        self.assertIn("DIFFERS at byte 57", text)

    def test_a_truncated_render_differs(self):
        code, text = self.check(FakePrograms(bytes(100), bytes(90)))
        self.assertEqual(code, 1, text)
        self.assertIn("byte 90", text)

    def test_a_failed_program_fails_the_case(self):
        self.assertEqual(self.check(FakePrograms(bytes(10), bytes(10), cli_code=2))[0], 1)
        self.assertEqual(self.check(FakePrograms(bytes(10), bytes(10), host_code=1))[0], 1)

    def test_the_host_plays_the_trace_the_drive_wrote(self):
        programs = FakePrograms(bytes(10), bytes(10))
        self.check(programs)
        cli, host = programs.calls
        self.assertIn("--trace", cli)
        self.assertEqual(host[1], cli[cli.index("--trace") + 1])        # the same file
        self.assertIn("--float", cli)                                   # no dither, no quantising
        self.assertIn("--float", host)

    def test_every_case_is_a_rb_cli_sound_drive(self):
        for name, args in host_parity.CASES.items():
            self.assertTrue(any(a in args for a in ("--idle", "--rev", "--sweep", "--hold")), name)
            self.assertIn("--preset", args, name)

    def test_unknown_cases_are_refused(self):
        with self.assertRaises(SystemExit):
            host_parity.main(["host_parity.py", "cli", "host", "out", "no-such-case"])


if __name__ == "__main__":
    unittest.main()
