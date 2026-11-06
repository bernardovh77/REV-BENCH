"""Shared helpers for the tools' acceptance checks and drive reports (stage 119).

require(): an acceptance check that stays on under `python -O`, which drops
every `assert`. A failed check raises CheckFailed with what failed.

program(): a built program's real file (NAME.exe on Windows).

lift_at() and clutch_at() (and their _instant forms, the time alone): when
`rb_cli sound`'s driver lifted off the pedal and let the clutch in, from its
--csv telemetry. The CLI acts
at the start of a 128-frame block and writes the block's row at its end, so
the first row showing the change is one block late (2.67 ms at 48 kHz). The
instant is that row's block start: the previous row's time, or 0 for the
first row. Every report takes these instants, so "lift" means one moment.
"""
import os
from pathlib import Path


def program(path):
    """The program a path names: on Windows a compiler writes NAME.exe for
    `-o NAME`, so that file is the program when NAME itself isn't there.
    Running NAME finds NAME.exe on its own; reading it (to fingerprint it)
    needs the real name."""
    path = Path(path)
    if os.name == "nt" and not path.exists() and path.with_name(path.name + ".exe").exists():
        return path.with_name(path.name + ".exe")
    return path


class CheckFailed(ValueError):
    """An acceptance check that failed (a ValueError, as the tools' other
    refusals are, so callers that catch those catch it too)."""


def require(ok, what):
    """Raises CheckFailed(what) unless ok."""
    if not ok:
        raise CheckFailed(what)


def change_at(rows, changed):
    """When the driver made a change: the start of the first block whose row
    satisfies changed(row), in s, and the row written at that instant (the
    previous block's, so its rpm is the speed then; the first row if the
    change was there from the start). rows are the CSV's dicts in order, with
    t_s. Raises CheckFailed if no row shows the change."""
    for index, row in enumerate(rows):
        if changed(row):
            if index == 0:
                return 0.0, row
            return float(rows[index - 1]["t_s"]), rows[index - 1]
    raise CheckFailed("the drive never made the change")


def lifted(row):
    return float(row["throttle"]) == 0.0


def clutched(row, car_inertia=2.9):
    """The clutch is in once the car's inertia has left the crank."""
    return float(row["crank_inertia_kgm2"]) < car_inertia


def lift_at(rows):
    """When the driver lifted off (the first block with the throttle shut), s,
    and the row at that instant."""
    return change_at(rows, lifted)


def clutch_at(rows):
    """When the clutch went in (the first block without the car's inertia), s,
    and the row at that instant."""
    return change_at(rows, clutched)


def lift_instant(rows):
    return lift_at(rows)[0]


def clutch_instant(rows):
    return clutch_at(rows)[0]
