import copy
from pathlib import Path
import tempfile
import unittest

import analyze_acoustic_candidates as analysis
from run_acoustic_candidates import read_row, check_topology_identity


def fixture():
    fv = dict(case="fv-000", group="matrix", kind="A1", status="ok", dx_m=.01,
              temperature_K=300., mean_flow_m_s=0., frequency_Hz=2000., rate_Hz=192000.,
              carrier_magnitude=.5, carrier_phase_error_deg=5., mass_relative_error=1e-15,
              momentum_relative_error=1e-15, total_energy_relative_error=1e-15,
              tag_relative_error=1e-15, failures=0., reconstruction_fallbacks=0., flux_fallbacks=0.)
    wave = dict(case="wave-000", group="transfer", kind="B0-transfer", status="ok",
                interpolation="allpass", frequency_Hz=4000., carrier_magnitude=1.,
                carrier_phase_error_deg=2., failures=0., relative_ledger_error=1e-15,
                energy_ratio=1., max_energy_ratio=1.)
    loop = dict(wave, case="wave-001", group="long-closed-loop", kind="B0-loop")
    topology = dict(unknown_connections=0, engines=[dict(preset=p, pipes=[], rev_limit=7500)
                    for p in ("i4", "i6", "v8-cross", "v8-flat", "boxer4")])
    manifest = dict(records=[dict(case=r["case"]) for r in (fv, wave, loop)])
    return [fv], [wave, loop], topology, manifest


class CandidateEvidence(unittest.TestCase):
    def test_fractional_delay_z_domain_reference(self):
        row = dict(rate_Hz=48000., sound_m_s=343., segments=1.,
                   length_m=16.5*343/48000, frequency_Hz=12000., interpolation="linear")
        self.assertAlmostEqual(analysis.analytic_line(row).real, .5)
        self.assertAlmostEqual(analysis.analytic_line(row).imag, -.5)
        row["interpolation"] = "allpass"
        self.assertAlmostEqual(analysis.analytic_line(row).real, .6)
        self.assertAlmostEqual(analysis.analytic_line(row).imag, -.8)

    def test_exhaust_identity_must_survive_unrelated_topology_changes(self):
        original = dict(unknown_connections=0, engines=[dict(preset="i6", pipes=[
            dict(component="header-0", kind="header", id="i6/header-0", length_m=.5, area_m2=.001)])])
        changed = copy.deepcopy(original)
        check_topology_identity(original, changed)
        changed["engines"][0]["pipes"][0]["id"] = "i6/pipe-0"
        with self.assertRaises(ValueError):
            check_topology_identity(original, changed)

    def test_accuracy_failure_is_not_a_harness_failure(self):
        report = analysis.summarize(*fixture())
        self.assertTrue(report["harness_pass"])
        self.assertEqual(report["promotion_gates"]["A1_matrix_at_each_grid"][0]["passing"], 0)
        self.assertTrue(report["promotion_gates"]["B0_quiescent_1m_200_4000_Hz"])

    def test_repair_and_growth_are_rejected(self):
        data = fixture()
        data[1][-1]["energy_ratio"] = 1.1
        self.assertFalse(analysis.summarize(*data)["harness_pass"])
        data = fixture()
        data[0][0]["failures"] = 1
        self.assertFalse(analysis.summarize(*data)["harness_pass"])

    def test_missing_or_duplicate_case_is_rejected(self):
        data = fixture()
        extra = dict(data[1][0], case="wave-002")
        data[1].append(extra)
        data[3]["records"].append(dict(case="wave-002"))
        data[1].pop(0)
        self.assertFalse(analysis.summarize(*data)["harness_pass"])
        data = fixture()
        data[1].pop(0)
        with self.assertRaises(ValueError):
            analysis.summarize(*data)
        data = fixture()
        data[1].append(copy.deepcopy(data[1][0]))
        self.assertFalse(analysis.summarize(*data)["harness_pass"])

    def test_nonfinite_and_malformed_csv_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)/"bad.csv"
            path.write_text("kind,status,carrier_magnitude\nA1,ok,nan\n")
            with self.assertRaises(ValueError):
                analysis.read(path)
            path.write_text("kind,status,carrier_magnitude\nA1,ok\n")
            with self.assertRaises(ValueError):
                analysis.read(path)
        with self.assertRaises(ValueError):
            read_row("status,x\nok,1\nok,2\n")

    def test_publisher_preserves_audio_and_is_repeatable(self):
        summary = analysis.summarize(*fixture())
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)/"RESULTS.html"
            audio = '<audio controls src="data:audio/wav;base64,UNCHANGED"></audio>'
            original = f"<html><nav></nav><main>{audio}</main></html>"
            path.write_text(original)
            analysis.publish(path, summary, [])
            first = path.read_text()
            analysis.publish(path, summary, [])
            self.assertEqual(path.read_text(), first)
            self.assertEqual(first.count(audio), 1)
            self.assertEqual(first.count(analysis.BEGIN), 1)
            self.assertEqual(first.count('href="#transport-candidates"'), 1)

    def test_incomplete_result_is_not_overwritten(self):
        summary = analysis.summarize(*fixture())
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)/"RESULTS.html"
            original = "<main>"+analysis.BEGIN+"</main>"
            path.write_text(original)
            with self.assertRaises(ValueError):
                analysis.publish(path, summary, [])
            self.assertEqual(path.read_text(), original)


if __name__ == "__main__":
    unittest.main()
