#!/usr/bin/env python3
"""
Export a compact TPC-DS SQL64-style chain join:

    customer -- customer_demographics -- store_returns

The join keys are:
  customer.c_current_cdemo_sk = customer_demographics.cd_demo_sk
  store_returns.sr_cdemo_sk  = customer_demographics.cd_demo_sk
"""

from __future__ import annotations

import argparse
import shutil
import sys
from collections import Counter
from pathlib import Path


def split_dat(line: str) -> list[str]:
    line = line.rstrip("\n")
    if line.endswith("|"):
        line = line[:-1]
    return line.split("|")


def to_int(value: str) -> int:
    return int(value) if value else 0


def table_path(data_dir: Path, table: str) -> Path:
    path = data_dir / f"{table}.dat"
    if not path.exists():
        raise FileNotFoundError(f"missing {path}")
    return path


def scale_label(scale: str) -> str:
    return "sf_" + scale.replace(".", "p")


def load_key_counts(path: Path, key_col: int) -> Counter[int]:
    counts: Counter[int] = Counter()
    with path.open("r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            row = split_dat(line)
            if len(row) > key_col and row[key_col]:
                counts[to_int(row[key_col])] += 1
    return counts


def compute_expected(data_dir: Path, threshold: int) -> tuple[int, bool]:
    cd_counts = load_key_counts(table_path(data_dir, "customer_demographics"), 0)
    customer_counts = load_key_counts(table_path(data_dir, "customer"), 2)
    store_return_counts = load_key_counts(table_path(data_dir, "store_returns"), 4)

    total = 0
    for key, cd_count in cd_counts.items():
        total += cd_count * customer_counts[key] * store_return_counts[key]
        if total > threshold:
            return total, True
    return total, False


def write_projection(src: Path, dst: Path, columns: list[int]) -> int:
    rows = 0
    with src.open("r", encoding="utf-8", errors="ignore") as fin, dst.open("w", encoding="utf-8") as fout:
        for line in fin:
            row = split_dat(line)
            if len(row) <= max(columns):
                continue
            fout.write(" ".join(str(to_int(row[c])) for c in columns))
            fout.write("\n")
            rows += 1
    return rows


def export_case(data_dir: Path, out_dir: Path, scale: str, threshold: int) -> None:
    expected, stopped = compute_expected(data_dir, threshold)
    if stopped:
        print(
            f"[stop] {out_dir.name}: expected output exceeds threshold={threshold}; please confirm before testing.",
            flush=True,
        )
        raise SystemExit(2)

    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    rows = {}
    # R1: customer(c_current_cdemo_sk, c_customer_sk)
    rows["R1_customer"] = write_projection(table_path(data_dir, "customer"), out_dir / "R1_customer.tbl", [2, 0])
    # R2: customer_demographics(cd_demo_sk, payload)
    rows["R2_customer_demographics"] = write_projection(
        table_path(data_dir, "customer_demographics"), out_dir / "R2_customer_demographics.tbl", [0, 0]
    )
    # R3: store_returns(sr_cdemo_sk, sr_customer_sk)
    rows["R3_store_returns"] = write_projection(
        table_path(data_dir, "store_returns"), out_dir / "R3_store_returns.tbl", [4, 3]
    )

    with (out_dir / "expected.txt").open("w", encoding="utf-8") as f:
        f.write(f"{expected}\n")
    with (out_dir / "stats.txt").open("w", encoding="utf-8") as f:
        f.write(f"scale={scale}\n")
        f.write(f"expected={expected}\n")
        for name, row_count in rows.items():
            f.write(f"{name}={row_count}\n")

    print(
        f"[export] {out_dir} scale={scale} expected={expected} customer_rows={rows['R1_customer']} store_return_rows={rows['R3_store_returns']}",
        flush=True,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Export TPC-DS SQL64 chain benchmark tables.")
    parser.add_argument("--data-dir", default="tpcds/tools", help="Directory containing TPC-DS .dat files.")
    parser.add_argument("--out-root", default="tpcds/sql64_projected")
    parser.add_argument("--scale", default="1")
    parser.add_argument("--threshold", type=int, default=5_000_000)
    args = parser.parse_args()

    export_case(Path(args.data_dir).resolve(), Path(args.out_root).resolve() / scale_label(args.scale), args.scale, args.threshold)
    return 0


if __name__ == "__main__":
    sys.exit(main())
