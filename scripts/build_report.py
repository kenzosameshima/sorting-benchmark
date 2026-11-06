"""Build a self-contained, interactive HTML report from one or more benchmark runs.

    python scripts/build_report.py runs/windows-... runs/wsl-...  -o report.html
    python scripts/build_report.py results/results.csv                # bare CSV also works

Each input is a run directory produced by scripts/run_suite.py (results/results.csv,
results/summary.csv, run_info.json), a results/ directory, or a results.csv file.

Statistics are recomputed from the per-repetition results.csv (sample standard
deviation, t-based 95% CI of the mean, bootstrap CI for ratios of medians), not
taken from summary.csv, whose stddev is a population estimate.

Only the Python standard library is required. The HTML has no external assets.
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import random
import statistics
import sys
from pathlib import Path
from typing import Any

SCRIPT_DIR = Path(__file__).resolve().parent
TEMPLATE = SCRIPT_DIR / "report_template.html"

ALGORITHM_ORDER = ["bubble", "selection", "insertion", "quick", "quick_naive", "merge", "heap", "radix", "shell", "qsort", "std_sort", "std_stable_sort"]
# Library baselines: their counters depend on the platform libc/libstdc++, so they are
# excluded from the cross-platform counter-equality check (timings are still compared).
LIBRARY_BASELINES = {"qsort", "std_sort", "std_stable_sort"}
DATASET_ORDER = ["random", "sorted", "reversed", "nearly_sorted", "few_unique", "organ_pipe"]
BOOTSTRAP_ROUNDS = 1000
CV_WARN = 0.10  # coefficient of variation above which a cell is flagged noisy
FIRST_REP_WARN = 1.30  # rep1 / median(rest) above which warm-up bias is flagged
MIN_REPS_WARN = 10


# ---------------------------------------------------------------------------
# statistics
# ---------------------------------------------------------------------------
_T975 = [
    12.706, 4.303, 3.182, 2.776, 2.571, 2.447, 2.365, 2.306, 2.262, 2.228,
    2.201, 2.179, 2.160, 2.145, 2.131, 2.120, 2.110, 2.101, 2.093, 2.086,
    2.080, 2.074, 2.069, 2.064, 2.060, 2.056, 2.052, 2.048, 2.045, 2.042,
]


def t_critical(df: int) -> float:
    """Two-sided 95% Student t critical value (table up to 30, then a tight approximation)."""
    if df < 1:
        return float("nan")
    if df <= len(_T975):
        return _T975[df - 1]
    return 1.96 + 2.4 / df


def percentile(sorted_values: list[float], q: float) -> float:
    if len(sorted_values) == 1:
        return sorted_values[0]
    position = q * (len(sorted_values) - 1)
    low = int(math.floor(position))
    high = min(low + 1, len(sorted_values) - 1)
    return sorted_values[low] + (sorted_values[high] - sorted_values[low]) * (position - low)


def describe(values: list[float]) -> dict[str, float | int | None]:
    n = len(values)
    ordered = sorted(values)
    mean = statistics.fmean(values)
    sd = statistics.stdev(values) if n > 1 else None
    ci95 = t_critical(n - 1) * sd / math.sqrt(n) if sd is not None else None
    return {
        "n": n,
        "mean": mean,
        "median": statistics.median(ordered),
        "sd": sd,
        "cv": (sd / mean) if sd is not None and mean > 0 else None,
        "ci95": ci95,
        "min": ordered[0],
        "max": ordered[-1],
        "q1": percentile(ordered, 0.25),
        "q3": percentile(ordered, 0.75),
    }


def bootstrap_ratio_ci(a: list[float], b: list[float], rng: random.Random) -> tuple[float, float] | None:
    """95% CI of median(a)/median(b) for independent samples."""
    if len(a) < 5 or len(b) < 5:
        return None
    ratios = []
    for _ in range(BOOTSTRAP_ROUNDS):
        ma = statistics.median(rng.choices(a, k=len(a)))
        mb = statistics.median(rng.choices(b, k=len(b)))
        if mb > 0:
            ratios.append(ma / mb)
    ratios.sort()
    return percentile(ratios, 0.025), percentile(ratios, 0.975)


def bootstrap_paired_ci(ratios: list[float], rng: random.Random) -> tuple[float, float] | None:
    """95% CI of the median of paired per-repetition ratios."""
    if len(ratios) < 5:
        return None
    medians = sorted(statistics.median(rng.choices(ratios, k=len(ratios))) for _ in range(BOOTSTRAP_ROUNDS))
    return percentile(medians, 0.025), percentile(medians, 0.975)


# ---------------------------------------------------------------------------
# loading
# ---------------------------------------------------------------------------
def find_run_files(path: Path) -> tuple[Path, Path | None]:
    """Return (results.csv, run_info.json or None) for a run directory or CSV path."""
    if path.is_file():
        results = path
        base = path.parent.parent if path.parent.name == "results" else path.parent
    else:
        results = path / "results" / "results.csv"
        base = path
        if not results.exists():
            results = path / "results.csv"
            base = path.parent if path.name == "results" else path
    if not results.exists():
        raise SystemExit(f"error: no results.csv found for {path}")
    info = next((c for c in (base / "run_info.json", results.parent / "run_info.json") if c.exists()), None)
    return results, info


def num(text: str) -> float | None:
    try:
        return float(text)
    except ValueError:
        return None


def load_run(path: Path) -> dict[str, Any]:
    results_path, info_path = find_run_files(path)
    info = json.loads(info_path.read_text(encoding="utf-8")) if info_path else {}

    rows: dict[tuple[str, str, int], dict[int, dict[str, Any]]] = {}
    skipped: dict[tuple[str, str, int], int] = {}
    invalid: dict[tuple[str, str, int], int] = {}
    meta = {"seed": None, "compiler": None, "flags": None}
    with results_path.open(newline="", encoding="utf-8") as handle:
        for record in csv.DictReader(handle):
            key = (record["algorithm"], record["dataset_type"], int(record["size"]))
            meta["seed"] = meta["seed"] or record["seed"]
            meta["compiler"] = meta["compiler"] or record["compiler"]
            meta["flags"] = meta["flags"] or record["flags"]
            status = record["status"]
            if status == "OK":
                rows.setdefault(key, {})[int(record["rep"])] = {
                    "time": float(record["time"]),
                    "comparisons": int(record["comparisons"]),
                    "swaps": int(record["swaps"]),
                    "movements": int(record["movements"]),
                }
            elif status == "INVALID":
                invalid[key] = invalid.get(key, 0) + 1
            else:
                skipped[key] = skipped.get(key, 0) + 1

    label = info.get("label") or (path.parent.name if path.is_file() and path.parent.name != "results" else path.name)
    name = info.get("display_name") or label
    return {"path": str(path), "label": label, "name": name, "info": info, "meta": meta,
            "rows": rows, "skipped": skipped, "invalid": invalid}


# ---------------------------------------------------------------------------
# aggregation
# ---------------------------------------------------------------------------
def cell_id(algorithm: str, dataset: str, size: int) -> str:
    return f"{algorithm}|{dataset}|{size}"


def sort_key(item: str, order: list[str]) -> tuple[int, str]:
    return (order.index(item) if item in order else len(order), item)


def aggregate_run(run: dict[str, Any], baseline: str, rng: random.Random) -> dict[str, dict[str, Any]]:
    cells: dict[str, dict[str, Any]] = {}
    for (algorithm, dataset, size), reps in run["rows"].items():
        ordered_reps = sorted(reps)
        times = [reps[r]["time"] for r in ordered_reps]
        stats = describe(times)
        first_ratio = None
        if len(times) >= 3:
            rest = statistics.median(times[1:])
            first_ratio = times[0] / rest if rest > 0 else None
        counters = {name: [reps[r][name] for r in ordered_reps] for name in ("comparisons", "swaps", "movements")}
        key = (algorithm, dataset, size)
        cell: dict[str, Any] = {
            "algorithm": algorithm, "dataset": dataset, "size": size,
            **stats,
            "first_ratio": first_ratio,
            "skipped": run["skipped"].get(key, 0),
            "invalid": run["invalid"].get(key, 0),
        }
        for name, series in counters.items():
            cell[name] = statistics.fmean(series)
            cell[name + "_constant"] = min(series) == max(series)
        cell["_times"] = dict(zip(ordered_reps, times))
        cell["_counters"] = {r: (reps[r]["comparisons"], reps[r]["swaps"], reps[r]["movements"]) for r in ordered_reps}
        cells[cell_id(algorithm, dataset, size)] = cell

    # paired ratio versus the baseline algorithm (same dataset copy in every repetition)
    for cell in cells.values():
        base = cells.get(cell_id(baseline, cell["dataset"], cell["size"]))
        cell["vs_baseline"] = None
        if base is None:
            continue
        ratios = [t / base["_times"][r] for r, t in cell["_times"].items() if r in base["_times"] and base["_times"][r] > 0]
        if not ratios:
            continue
        ci = bootstrap_paired_ci(ratios, rng) if cell is not base else (1.0, 1.0)
        cell["vs_baseline"] = {"ratio": statistics.median(ratios), "lo": ci[0] if ci else None, "hi": ci[1] if ci else None}
    return cells


def compare_runs(cells_a: dict[str, dict[str, Any]], cells_b: dict[str, dict[str, Any]], rng: random.Random) -> dict[str, Any]:
    """Ratio of B over A (median times) per shared cell, plus exact counter agreement."""
    ratios: dict[str, Any] = {}
    compared = mismatches = 0
    examples: list[str] = []
    for key in sorted(set(cells_a) & set(cells_b)):
        a, b = cells_a[key], cells_b[key]
        ta, tb = list(a["_times"].values()), list(b["_times"].values())
        ci = bootstrap_ratio_ci(tb, ta, rng)
        ratio = b["median"] / a["median"] if a["median"] > 0 else None
        shared = sorted(set(a["_counters"]) & set(b["_counters"]))
        if key.split("|", 1)[0] in LIBRARY_BASELINES:
            shared = []
        equal = all(a["_counters"][r] == b["_counters"][r] for r in shared) if shared else None
        if shared:
            compared += len(shared)
            bad = [r for r in shared if a["_counters"][r] != b["_counters"][r]]
            mismatches += len(bad)
            if bad and len(examples) < 5:
                examples.append(f"{key} rep {bad[0]}: {a['_counters'][bad[0]]} vs {b['_counters'][bad[0]]}")
        ratios[key] = {
            "ratio": ratio,
            "lo": ci[0] if ci else None,
            "hi": ci[1] if ci else None,
            "significant": bool(ci and (ci[0] > 1.0 or ci[1] < 1.0)),
            "counters_equal": equal,
        }
    return {"cells": ratios, "counter_rows_compared": compared, "counter_mismatches": mismatches, "examples": examples}


def run_warnings(run: dict[str, Any], cells: dict[str, dict[str, Any]]) -> list[str]:
    info, notes = run["info"], []
    if not info:
        notes.append("No run_info.json: environment metadata unavailable (CPU, OS, compiler, pinning).")
    if info.get("git", {}).get("dirty"):
        notes.append("Built from a working tree with uncommitted changes.")
    if info and info.get("pinning", {}).get("cpu") is None:
        notes.append("Process was not pinned to a CPU: scheduler migrations add noise.")
    if info.get("system", {}).get("is_wsl"):
        notes.append("WSL2 guest: runs inside a VM; clocks and CPU frequency are controlled by the Windows host.")
    if info.get("exit_code") not in (None, 0):
        notes.append(f"Benchmark exited with code {info['exit_code']}.")
    low = sorted({c["n"] for c in cells.values() if c["n"] < MIN_REPS_WARN})
    if low:
        notes.append(f"Some cells have fewer than {MIN_REPS_WARN} repetitions (min {low[0]}): CIs are wide.")
    invalid = sum(run["invalid"].values())
    if invalid:
        notes.append(f"{invalid} repetition(s) failed validation (INVALID) and were excluded.")
    return notes


# ---------------------------------------------------------------------------
# output
# ---------------------------------------------------------------------------
def public_cell(cell: dict[str, Any]) -> dict[str, Any]:
    return {k: v for k, v in cell.items() if not k.startswith("_")}


def unique_names(runs: list[dict[str, Any]]) -> None:
    seen: dict[str, int] = {}
    for run in runs:
        seen[run["name"]] = seen.get(run["name"], 0) + 1
    for run in runs:
        if seen[run["name"]] > 1:
            run["name"] = f"{run['name']} ({run['label']})"


def build_payload(runs: list[dict[str, Any]], baseline: str, title: str) -> dict[str, Any]:
    rng = random.Random(20260921)  # deterministic bootstrap => reproducible report
    unique_names(runs)
    aggregated = [aggregate_run(run, baseline, rng) for run in runs]

    algorithms = sorted({c["algorithm"] for cells in aggregated for c in cells.values()}, key=lambda a: sort_key(a, ALGORITHM_ORDER))
    datasets = sorted({c["dataset"] for cells in aggregated for c in cells.values()}, key=lambda d: sort_key(d, DATASET_ORDER))
    sizes = sorted({c["size"] for cells in aggregated for c in cells.values()})

    cross: dict[str, Any] = {}
    counter_checks = []
    for i in range(len(runs)):
        for j in range(len(runs)):
            if i == j:
                continue
            result = compare_runs(aggregated[i], aggregated[j], rng)
            cross[f"{i}>{j}"] = result["cells"]
            if i < j:
                counter_checks.append({
                    "a": i, "b": j,
                    "compared": result["counter_rows_compared"],
                    "mismatches": result["counter_mismatches"],
                    "examples": result["examples"],
                })

    global_warnings: list[str] = []
    if len(runs) > 1:
        seeds = {r["meta"]["seed"] for r in runs}
        if len(seeds) > 1:
            global_warnings.append(f"Runs used different seeds ({', '.join(sorted(str(s) for s in seeds))}): counters and datasets are not comparable.")
        compilers = {(r["info"].get("compiler", {}) or {}).get("banner") or r["meta"]["compiler"] for r in runs}
        if len(compilers) > 1:
            global_warnings.append("Runs used different compilers: time ratios mix OS effects with compiler/codegen effects.")
        flags = {r["meta"]["flags"] for r in runs}
        if len(flags) > 1:
            global_warnings.append("Runs were built with different compilation flags.")
        systems = {(r["info"].get("system", {}) or {}).get("cpu_model") for r in runs}
        if len(systems) > 1:
            global_warnings.append("Runs come from different CPUs: absolute times are not comparable, only relative shapes are.")

    return {
        "title": title,
        "baseline": baseline if any(baseline in {c["algorithm"] for c in cells.values()} for cells in aggregated) else None,
        "algorithms": algorithms,
        "datasets": datasets,
        "sizes": sizes,
        "runs": [
            {
                "index": i,
                "name": run["name"],
                "label": run["label"],
                "info": run["info"],
                "csv": run["meta"],
                "warnings": run_warnings(run, aggregated[i]),
                "cells": {k: public_cell(v) for k, v in aggregated[i].items()},
                "invalid_rows": sum(run["invalid"].values()),
            }
            for i, run in enumerate(runs)
        ],
        "cross": cross,
        "counter_checks": counter_checks,
        "warnings": global_warnings,
        "thresholds": {"cv": CV_WARN, "first_rep": FIRST_REP_WARN, "min_reps": MIN_REPS_WARN},
    }


def render_html(payload: dict[str, Any]) -> str:
    template = TEMPLATE.read_text(encoding="utf-8")
    data = json.dumps(payload, separators=(",", ":"), allow_nan=False, default=str)
    data = data.replace("</", "<\\/").replace("\u2028", "\\u2028").replace("\u2029", "\\u2029")
    return template.replace("__REPORT_TITLE__", _escape(payload["title"])).replace("/*__REPORT_DATA__*/null", data)


def _escape(text: str) -> str:
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def render_markdown(payload: dict[str, Any]) -> str:
    lines = [f"## {payload['title']}", "", "| Run | CPU | OS | Compiler | Pinned CPU | Commit |", "|---|---|---|---|---|---|"]
    for run in payload["runs"]:
        info = run["info"]
        system, compiler = info.get("system", {}), info.get("compiler", {})
        commit = (info.get("git", {}).get("commit") or "")[:8] + (" (dirty)" if info.get("git", {}).get("dirty") else "")
        lines.append(
            f"| {run['name']} | {system.get('cpu_model', '?')} | {system.get('os', '?')} | "
            f"{compiler.get('banner') or run['csv'].get('compiler') or '?'} | {info.get('pinning', {}).get('cpu', '-')} | {commit or '-'} |"
        )
    lines += ["", "### Integrity", ""]
    if not payload["counter_checks"]:
        lines.append("- Single run: no cross-platform counter check.")
    for check in payload["counter_checks"]:
        a, b = payload["runs"][check["a"]]["name"], payload["runs"][check["b"]]["name"]
        status = "OK" if check["mismatches"] == 0 and check["compared"] > 0 else "MISMATCH"
        lines.append(f"- **{status}** counters `{a}` vs `{b}`: {check['compared']} repetitions compared, {check['mismatches']} mismatches")
        lines += [f"  - {example}" for example in check["examples"]]
    for run in payload["runs"]:
        if run["invalid_rows"]:
            lines.append(f"- **INVALID** rows in `{run['name']}`: {run['invalid_rows']}")
    lines += [f"- Warning: {w}" for w in payload["warnings"]]
    for run in payload["runs"]:
        lines += [f"- `{run['name']}`: {w}" for w in run["warnings"]]

    for key, cells in payload["cross"].items():
        i, j = (int(x) for x in key.split(">"))
        if i != 0:
            continue
        lines += ["", f"### Median time ratio: {payload['runs'][j]['name']} / {payload['runs'][0]['name']}", "",
                  "| Algorithm | Median ratio over cells (size >= 1000) | Cells | Significant |", "|---|---|---|---|"]
        for algorithm in payload["algorithms"]:
            picked = [v for k, v in cells.items() if k.startswith(algorithm + "|") and int(k.rsplit("|", 1)[1]) >= 1000 and v["ratio"]]
            if picked:
                significant = sum(1 for v in picked if v["significant"])
                lines.append(f"| {algorithm} | {statistics.median(v['ratio'] for v in picked):.2f} | {len(picked)} | {significant} |")
        lines.append("")
    lines += ["_Times include the cost of the operation counters (instrumentation) inside the timed region._", ""]
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("inputs", nargs="+", type=Path, help="run directories, results/ directories or results.csv files")
    parser.add_argument("-o", "--output", type=Path, default=Path("report.html"))
    parser.add_argument("--markdown", type=Path, help="also write a Markdown summary (useful for CI job summaries)")
    parser.add_argument("--title", default="Sorting benchmark report")
    parser.add_argument("--baseline", default="quick", help="algorithm used for paired speedup ratios (default: quick)")
    parser.add_argument("--fail-on-mismatch", action="store_true",
                        help="exit with status 3 if counters differ across runs or any repetition was INVALID")
    args = parser.parse_args()

    runs = [load_run(path) for path in args.inputs]
    payload = build_payload(runs, args.baseline, args.title)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render_html(payload), encoding="utf-8", newline="\n")
    print(f"report written: {args.output} ({args.output.stat().st_size // 1024} KiB, {len(runs)} run(s))")
    if args.markdown:
        args.markdown.parent.mkdir(parents=True, exist_ok=True)
        args.markdown.write_text(render_markdown(payload), encoding="utf-8", newline="\n")
        print(f"markdown written: {args.markdown}")

    problems = any(c["mismatches"] for c in payload["counter_checks"]) or any(r["invalid_rows"] for r in payload["runs"])
    for check in payload["counter_checks"]:
        if check["mismatches"]:
            print(f"COUNTER MISMATCH: {check['mismatches']} of {check['compared']} repetitions differ", file=sys.stderr)
            for example in check["examples"]:
                print(f"  {example}", file=sys.stderr)
    if problems and args.fail_on_mismatch:
        return 3
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
