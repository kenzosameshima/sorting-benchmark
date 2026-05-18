"""Generate plots from sorting benchmark summary results."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


PROJECT_ROOT = Path(__file__).resolve().parents[1]

REQUIRED_COLUMNS = {
    "algorithm",
    "dataset_type",
    "size",
    "status",
    "time_mean",
    "time_median",
    "time_stddev",
    "time_min",
    "time_max",
    "comparisons_mean",
    "swaps_mean",
    "movements_mean",
}

COLUMN_ALIASES = {
    "average_comparisons": "comparisons_mean",
    "average_swaps": "swaps_mean",
    "average_movements": "movements_mean",
}

NUMERIC_COLUMNS = [
    "size",
    "time_mean",
    "time_median",
    "time_stddev",
    "time_min",
    "time_max",
    "comparisons_mean",
    "swaps_mean",
    "movements_mean",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate offline plots from sorting benchmark summary CSV data."
    )
    parser.add_argument(
        "--input",
        type=Path,
        default=PROJECT_ROOT / "results" / "summary.csv",
        help="path to summary CSV (default: results/summary.csv)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=PROJECT_ROOT / "plots",
        help="output directory for plots (default: plots)",
    )
    return parser.parse_args()


def load_summary(path: Path) -> pd.DataFrame:
    if not path.exists():
        raise ValueError(f"Input CSV does not exist: {path}")
    if not path.is_file():
        raise ValueError(f"Input path is not a file: {path}")

    try:
        df = pd.read_csv(path)
    except pd.errors.EmptyDataError as exc:
        raise ValueError(f"Input CSV is empty: {path}") from exc
    except OSError as exc:
        raise ValueError(f"Could not read input CSV {path}: {exc}") from exc

    if df.empty:
        raise ValueError(f"Input CSV contains no data rows: {path}")

    return df


def validate_columns(df: pd.DataFrame) -> None:
    available = set(df.columns)
    effective_columns = set(available)

    for source, target in COLUMN_ALIASES.items():
        if source in available:
            effective_columns.add(target)

    missing = sorted(REQUIRED_COLUMNS - effective_columns)
    if missing:
        missing_text = ", ".join(missing)
        raise ValueError(f"Input CSV is missing required column(s): {missing_text}")


def _normalize_columns(df: pd.DataFrame) -> pd.DataFrame:
    rename_columns = {
        source: target
        for source, target in COLUMN_ALIASES.items()
        if source in df.columns and target not in df.columns
    }
    return df.rename(columns=rename_columns)


def prepare_data(df: pd.DataFrame) -> pd.DataFrame:
    prepared = _normalize_columns(df).copy()
    ok_rows = prepared[prepared["status"] == "OK"].copy()

    if ok_rows.empty:
        raise ValueError("No plottable rows found: all benchmark rows are SKIPPED or non-OK.")

    for column in NUMERIC_COLUMNS:
        ok_rows[column] = pd.to_numeric(ok_rows[column], errors="coerce")

    before_required_drop = len(ok_rows)
    ok_rows = ok_rows.dropna(subset=["size", "time_mean"])
    if ok_rows.empty:
        raise ValueError(
            "No valid rows remain after filtering: size and time_mean must be numeric."
        )

    dropped = before_required_drop - len(ok_rows)
    if dropped:
        print(
            f"Warning: dropped {dropped} OK row(s) with invalid size or time_mean.",
            file=sys.stderr,
        )

    group_columns = ["algorithm", "dataset_type", "size"]
    value_columns = [column for column in NUMERIC_COLUMNS if column != "size"]
    aggregated = (
        ok_rows.groupby(group_columns, as_index=False, sort=True)[value_columns]
        .mean()
        .sort_values(group_columns)
    )

    if aggregated.empty:
        raise ValueError("No valid rows remain after aggregating benchmark data.")

    return aggregated


def _set_log_scale_if_positive(ax: plt.Axes, axis: str, values: pd.Series) -> None:
    numeric_values = pd.to_numeric(values, errors="coerce").dropna()
    if not numeric_values.empty and (numeric_values > 0).all():
        if axis == "x":
            ax.set_xscale("log")
        else:
            ax.set_yscale("log")


def _finish_plot(fig: plt.Figure, path: Path) -> None:
    fig.tight_layout()
    fig.savefig(path, dpi=150)
    plt.close(fig)


def plot_runtime_by_size(df: pd.DataFrame, output_dir: Path) -> None:
    plot_df = (
        df.groupby(["algorithm", "size"], as_index=False, sort=True)["time_mean"]
        .mean()
        .sort_values(["algorithm", "size"])
    )

    fig, ax = plt.subplots(figsize=(10, 6))
    for algorithm, algorithm_df in plot_df.groupby("algorithm", sort=True):
        ax.plot(
            algorithm_df["size"],
            algorithm_df["time_mean"],
            marker="o",
            linewidth=1.8,
            label=str(algorithm),
        )

    ax.set_title("Runtime by Input Size")
    ax.set_xlabel("Input size")
    ax.set_ylabel("Mean runtime (seconds)")
    ax.grid(True, which="both", linestyle="--", alpha=0.4)
    ax.legend(title="Algorithm")
    _set_log_scale_if_positive(ax, "x", plot_df["size"])
    _set_log_scale_if_positive(ax, "y", plot_df["time_mean"])
    _finish_plot(fig, output_dir / "runtime_by_size.png")


def plot_runtime_by_dataset(df: pd.DataFrame, output_dir: Path) -> None:
    plot_df = (
        df.groupby(["dataset_type", "algorithm"], as_index=False, sort=True)["time_mean"]
        .mean()
        .sort_values(["dataset_type", "algorithm"])
    )
    table = plot_df.pivot(
        index="dataset_type", columns="algorithm", values="time_mean"
    ).sort_index()

    fig, ax = plt.subplots(figsize=(10, 6))
    table.plot(kind="bar", ax=ax, width=0.82)

    ax.set_title("Average Runtime by Dataset Type")
    ax.set_xlabel("Dataset type")
    ax.set_ylabel("Mean runtime (seconds)")
    ax.grid(True, axis="y", which="both", linestyle="--", alpha=0.4)
    ax.legend(title="Algorithm")
    _set_log_scale_if_positive(ax, "y", plot_df["time_mean"])
    ax.tick_params(axis="x", rotation=35)
    _finish_plot(fig, output_dir / "runtime_by_dataset.png")


def plot_comparisons_by_size(df: pd.DataFrame, output_dir: Path) -> None:
    comparisons_df = df.dropna(subset=["comparisons_mean"]).copy()
    if comparisons_df.empty:
        raise ValueError("No valid comparisons_mean values are available for plotting.")

    plot_df = (
        comparisons_df.groupby(["algorithm", "size"], as_index=False, sort=True)[
            "comparisons_mean"
        ]
        .mean()
        .sort_values(["algorithm", "size"])
    )

    fig, ax = plt.subplots(figsize=(10, 6))
    for algorithm, algorithm_df in plot_df.groupby("algorithm", sort=True):
        ax.plot(
            algorithm_df["size"],
            algorithm_df["comparisons_mean"],
            marker="o",
            linewidth=1.8,
            label=str(algorithm),
        )

    ax.set_title("Comparisons by Input Size")
    ax.set_xlabel("Input size")
    ax.set_ylabel("Mean comparisons")
    ax.grid(True, which="both", linestyle="--", alpha=0.4)
    ax.legend(title="Algorithm")
    _set_log_scale_if_positive(ax, "x", plot_df["size"])
    _set_log_scale_if_positive(ax, "y", plot_df["comparisons_mean"])
    _finish_plot(fig, output_dir / "comparisons_by_size.png")


def main() -> int:
    args = parse_args()
    input_path = args.input if args.input.is_absolute() else args.input.resolve()
    output_dir = args.output if args.output.is_absolute() else args.output.resolve()

    try:
        df = load_summary(input_path)
        validate_columns(df)
        prepared = prepare_data(df)
        output_dir.mkdir(parents=True, exist_ok=True)

        plot_functions = [
            plot_runtime_by_size,
            plot_runtime_by_dataset,
            plot_comparisons_by_size,
        ]
        for plot_function in plot_functions:
            plot_function(prepared, output_dir)

    except ValueError as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    generated_files = [
        output_dir / "runtime_by_size.png",
        output_dir / "runtime_by_dataset.png",
        output_dir / "comparisons_by_size.png",
    ]

    print(
        f"Analysis completed: {len(generated_files)} plots generated from "
        f"{len(prepared)} valid rows."
    )
    print("\nGenerated files:")
    for path in generated_files:
        print(f"- {path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
