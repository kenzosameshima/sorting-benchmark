"""Unit tests for scripts/build_report.py (standard library only)."""

from __future__ import annotations

import csv
import json
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPTS))

import build_report as br  # noqa: E402

HEADER = ["algorithm", "size", "rep", "seed", "dataset_type", "timestamp", "compiler", "flags",
          "status", "time", "comparisons", "swaps", "movements"]


def write_run(directory: Path, rows: list[dict], label: str = "run", info: dict | None = None) -> Path:
    (directory / "results").mkdir(parents=True, exist_ok=True)
    with (directory / "results" / "results.csv").open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=HEADER)
        writer.writeheader()
        for row in rows:
            writer.writerow({"seed": 42, "timestamp": "t", "compiler": "gcc", "flags": "-O2", "status": "OK", **row})
    payload = {"label": label, "display_name": label, **(info or {})}
    (directory / "run_info.json").write_text(json.dumps(payload), encoding="utf-8")
    return directory


def make_rows(scale: float = 1.0, comparisons_shift: int = 0) -> list[dict]:
    rows = []
    for algorithm, base in (("quick", 1.0), ("bubble", 4.0)):
        for rep in range(1, 7):
            rows.append({
                "algorithm": algorithm, "size": 100, "rep": rep, "dataset_type": "random",
                "time": f"{base * scale * (1 + rep * 0.01):.9f}",
                "comparisons": 1000 + rep + comparisons_shift, "swaps": 10 * rep, "movements": 30 * rep,
            })
    return rows


class StatisticsTests(unittest.TestCase):
    def test_t_critical_table_and_tail(self) -> None:
        self.assertAlmostEqual(br.t_critical(1), 12.706)
        self.assertAlmostEqual(br.t_critical(30), 2.042)
        self.assertAlmostEqual(br.t_critical(60), 2.000, delta=0.01)
        self.assertAlmostEqual(br.t_critical(120), 1.980, delta=0.01)

    def test_describe_uses_sample_standard_deviation(self) -> None:
        stats = br.describe([1.0, 2.0, 3.0, 4.0])
        self.assertEqual(stats["n"], 4)
        self.assertAlmostEqual(stats["mean"], 2.5)
        self.assertAlmostEqual(stats["median"], 2.5)
        self.assertAlmostEqual(stats["sd"], 1.2909944, places=6)  # N-1 denominator (population would be 1.118)
        self.assertAlmostEqual(stats["ci95"], 3.182 * 1.2909944 / 2, places=4)

    def test_describe_single_value_has_no_spread(self) -> None:
        stats = br.describe([5.0])
        self.assertIsNone(stats["sd"])
        self.assertIsNone(stats["ci95"])

    def test_percentile_interpolates(self) -> None:
        self.assertAlmostEqual(br.percentile([1, 2, 3, 4, 5], 0.25), 2.0)
        self.assertAlmostEqual(br.percentile([1, 2], 0.5), 1.5)


class ReportTests(unittest.TestCase):
    def setUp(self) -> None:
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = Path(self._tmp.name)

    def tearDown(self) -> None:
        self._tmp.cleanup()

    def payload(self, *runs: Path):
        return br.build_payload([br.load_run(r) for r in runs], "quick", "T")

    def test_identical_counters_pass_and_ratio_reflects_scale(self) -> None:
        a = write_run(self.tmp / "a", make_rows(1.0), "a")
        b = write_run(self.tmp / "b", make_rows(2.0), "b")
        payload = self.payload(a, b)
        check = payload["counter_checks"][0]
        self.assertEqual(check["mismatches"], 0)
        self.assertEqual(check["compared"], 12)
        ratio = payload["cross"]["0>1"]["quick|random|100"]
        self.assertAlmostEqual(ratio["ratio"], 2.0, places=6)
        self.assertTrue(ratio["significant"])  # CI of a pure 2x scaling excludes 1
        self.assertTrue(ratio["counters_equal"])

    def test_counter_divergence_is_reported(self) -> None:
        a = write_run(self.tmp / "a", make_rows(), "a")
        b = write_run(self.tmp / "b", make_rows(comparisons_shift=1), "b")
        check = self.payload(a, b)["counter_checks"][0]
        self.assertEqual(check["mismatches"], 12)
        self.assertTrue(check["examples"])

    def test_invalid_rows_are_excluded_and_counted(self) -> None:
        rows = make_rows()
        rows[0]["status"] = "INVALID"
        run = write_run(self.tmp / "a", rows, "a")
        payload = self.payload(run)
        self.assertEqual(payload["runs"][0]["invalid_rows"], 1)
        self.assertEqual(payload["runs"][0]["cells"]["quick|random|100"]["n"], 5)

    def test_paired_baseline_ratio(self) -> None:
        run = write_run(self.tmp / "a", make_rows(), "a")
        cells = self.payload(run)["runs"][0]["cells"]
        self.assertAlmostEqual(cells["quick|random|100"]["vs_baseline"]["ratio"], 1.0)
        self.assertAlmostEqual(cells["bubble|random|100"]["vs_baseline"]["ratio"], 4.0, places=6)

    def test_html_embeds_data_without_breaking_out_of_script(self) -> None:
        run = write_run(self.tmp / "a", make_rows(), "</script><img src=x onerror=alert(1)>")
        html = br.render_html(br.build_payload([br.load_run(run)], "quick", "<b>title</b>"))
        self.assertNotIn("</script><img", html)
        self.assertNotIn("<b>title</b>", html)
        self.assertNotIn("__REPORT_TITLE__", html)
        self.assertNotIn("/*__REPORT_DATA__*/", html)
        self.assertEqual(html.count("</script>"), 1)  # only the template's own closing tag

    def test_bare_results_csv_is_accepted(self) -> None:
        run = write_run(self.tmp / "a", make_rows(), "a")
        payload = self.payload(run / "results" / "results.csv")
        self.assertEqual(len(payload["runs"]), 1)


if __name__ == "__main__":
    unittest.main()
