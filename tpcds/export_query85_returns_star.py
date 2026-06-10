#!/usr/bin/env python3
"""
Export the red-box subtree from TPC-DS Q85:

    web_returns
    ├── customer_demographics cd1
    ├── customer_demographics cd2
    ├── customer_address
    └── reason

Selection predicates are ignored; this script only exports the PK-FK join tree.
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
    addr_counts = load_key_counts(table_path(data_dir, "customer_address"), 0)
    reason_counts = load_key_counts(table_path(data_dir, "reason"), 0)

    total = 0
    # web_returns columns:
    # wr_refunded_cdemo_sk=4, wr_refunded_addr_sk=6,
    # wr_returning_cdemo_sk=8, wr_reason_sk=12.
    with table_path(data_dir, "web_returns").open("r", encoding="utf-8", errors="ignore") as f:
        for line_no, line in enumerate(f, start=1):
            row = split_dat(line)
            if len(row) <= 12:
                continue
            count = (
                cd_counts[to_int(row[4])]
                * cd_counts[to_int(row[8])]
                * addr_counts[to_int(row[6])]
                * reason_counts[to_int(row[12])]
            )
            total += count
            if total > threshold:
                print(f"[stop] count exceeded threshold={threshold} at web_returns line={line_no}", flush=True)
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
    # R1: web_returns(wr_refunded_cdemo_sk, wr_returning_cdemo_sk, wr_refunded_addr_sk, wr_reason_sk)
    rows["R1_web_returns"] = write_projection(
        table_path(data_dir, "web_returns"), out_dir / "R1_web_returns.tbl", [4, 8, 6, 12]
    )
    rows["R2_cd1"] = write_projection(
        table_path(data_dir, "customer_demographics"), out_dir / "R2_cd1.tbl", [0, 0]
    )
    rows["R3_cd2"] = write_projection(
        table_path(data_dir, "customer_demographics"), out_dir / "R3_cd2.tbl", [0, 0]
    )
    rows["R4_customer_address"] = write_projection(
        table_path(data_dir, "customer_address"), out_dir / "R4_customer_address.tbl", [0, 0]
    )
    rows["R5_reason"] = write_projection(table_path(data_dir, "reason"), out_dir / "R5_reason.tbl", [0, 0])

    with (out_dir / "expected.txt").open("w", encoding="utf-8") as f:
        f.write(f"{expected}\n")
    with (out_dir / "stats.txt").open("w", encoding="utf-8") as f:
        f.write(f"scale={scale}\n")
        f.write(f"expected={expected}\n")
        for name, row_count in rows.items():
            f.write(f"{name}={row_count}\n")

    print(
        f"[export] {out_dir} scale={scale} expected={expected} web_returns_rows={rows['R1_web_returns']}",
        flush=True,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Export TPC-DS Q85 web_returns-star benchmark tables.")
    parser.add_argument("--data-dir", default="tpcds/tools", help="Directory containing TPC-DS .dat files.")
    parser.add_argument("--out-root", default="tpcds/sql85_returns_star_projected")
    parser.add_argument("--scale", default="1")
    parser.add_argument("--threshold", type=int, default=5_000_000)
    args = parser.parse_args()

    export_case(Path(args.data_dir).resolve(), Path(args.out_root).resolve() / scale_label(args.scale), args.scale, args.threshold)
    return 0


if __name__ == "__main__":
    sys.exit(main())
