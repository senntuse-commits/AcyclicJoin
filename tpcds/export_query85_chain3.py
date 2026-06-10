#!/usr/bin/env python3
"""
Export a compact three-table chain from the TPC-DS Q85 join tree:

    web_sales -- web_returns -- customer_demographics cd1

The web_sales-web_returns edge is the Q85 composite key:
  ws_item_sk = wr_item_sk AND ws_order_number = wr_order_number
It is encoded as one synthetic integer key so the benchmark's single-column
join interface can run the chain directly.
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


def build_return_pair_ids(data_dir: Path) -> tuple[dict[tuple[int, int], int], Counter[int]]:
    cd_counts = load_key_counts(table_path(data_dir, "customer_demographics"), 0)
    pair_to_id: dict[tuple[int, int], int] = {}
    wr_output_counts: Counter[int] = Counter()

    # web_returns columns:
    # wr_item_sk=2, wr_refunded_cdemo_sk=4, wr_order_number=13.
    with table_path(data_dir, "web_returns").open("r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            row = split_dat(line)
            if len(row) <= 13:
                continue
            cd_count = cd_counts[to_int(row[4])]
            if cd_count <= 0:
                continue
            pair = (to_int(row[2]), to_int(row[13]))
            pid = pair_to_id.get(pair)
            if pid is None:
                pid = len(pair_to_id) + 1
                pair_to_id[pair] = pid
            wr_output_counts[pid] += cd_count
    return pair_to_id, wr_output_counts


def compute_expected(
    data_dir: Path,
    pair_to_id: dict[tuple[int, int], int],
    wr_output_counts: Counter[int],
    threshold: int,
) -> tuple[int, bool]:
    total = 0
    # web_sales columns: ws_item_sk=3, ws_web_page_sk=12, ws_order_number=17.
    with table_path(data_dir, "web_sales").open("r", encoding="utf-8", errors="ignore") as f:
        for line_no, line in enumerate(f, start=1):
            row = split_dat(line)
            if len(row) <= 17:
                continue
            pid = pair_to_id.get((to_int(row[3]), to_int(row[17])))
            if pid is None:
                continue
            total += wr_output_counts[pid]
            if total > threshold:
                print(f"[stop] count exceeded threshold={threshold} at web_sales line={line_no}", flush=True)
                return total, True
    return total, False


def write_web_sales(data_dir: Path, dst: Path, pair_to_id: dict[tuple[int, int], int]) -> int:
    rows = 0
    with table_path(data_dir, "web_sales").open("r", encoding="utf-8", errors="ignore") as fin, dst.open(
        "w", encoding="utf-8"
    ) as fout:
        for line in fin:
            row = split_dat(line)
            if len(row) <= 17:
                continue
            pair_id = pair_to_id.get((to_int(row[3]), to_int(row[17])))
            if pair_id is None:
                continue
            fout.write(f"{pair_id} {to_int(row[12])}\n")
            rows += 1
    return rows


def write_web_returns(data_dir: Path, dst: Path, pair_to_id: dict[tuple[int, int], int], cd_keys: set[int]) -> int:
    rows = 0
    with table_path(data_dir, "web_returns").open("r", encoding="utf-8", errors="ignore") as fin, dst.open(
        "w", encoding="utf-8"
    ) as fout:
        for line in fin:
            row = split_dat(line)
            if len(row) <= 13:
                continue
            cd_key = to_int(row[4])
            if cd_key not in cd_keys:
                continue
            pair_id = pair_to_id.get((to_int(row[2]), to_int(row[13])))
            if pair_id is None:
                continue
            fout.write(f"{pair_id} {cd_key}\n")
            rows += 1
    return rows


def write_cd1(data_dir: Path, dst: Path) -> int:
    rows = 0
    with table_path(data_dir, "customer_demographics").open("r", encoding="utf-8", errors="ignore") as fin, dst.open(
        "w", encoding="utf-8"
    ) as fout:
        for line in fin:
            row = split_dat(line)
            if not row or not row[0]:
                continue
            key = to_int(row[0])
            fout.write(f"{key} {key}\n")
            rows += 1
    return rows


def export_case(data_dir: Path, out_dir: Path, scale: str, threshold: int) -> None:
    pair_to_id, wr_output_counts = build_return_pair_ids(data_dir)
    expected, stopped = compute_expected(data_dir, pair_to_id, wr_output_counts, threshold)
    if stopped:
        print(
            f"[stop] {out_dir.name}: expected output exceeds threshold={threshold}; please confirm before testing.",
            flush=True,
        )
        raise SystemExit(2)

    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    cd_keys = set(load_key_counts(table_path(data_dir, "customer_demographics"), 0).keys())
    rows = {}
    rows["R1_web_sales"] = write_web_sales(data_dir, out_dir / "R1_web_sales.tbl", pair_to_id)
    rows["R2_web_returns"] = write_web_returns(data_dir, out_dir / "R2_web_returns.tbl", pair_to_id, cd_keys)
    rows["R3_cd1"] = write_cd1(data_dir, out_dir / "R3_cd1.tbl")

    with (out_dir / "expected.txt").open("w", encoding="utf-8") as f:
        f.write(f"{expected}\n")
    with (out_dir / "stats.txt").open("w", encoding="utf-8") as f:
        f.write(f"scale={scale}\n")
        f.write(f"expected={expected}\n")
        f.write(f"return_pair_keys={len(pair_to_id)}\n")
        for name, row_count in rows.items():
            f.write(f"{name}={row_count}\n")

    print(
        f"[export] {out_dir} scale={scale} expected={expected} web_sales_rows={rows['R1_web_sales']} web_returns_rows={rows['R2_web_returns']} pair_keys={len(pair_to_id)}",
        flush=True,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Export TPC-DS Q85 three-table chain benchmark tables.")
    parser.add_argument("--data-dir", default="tpcds/tools", help="Directory containing TPC-DS .dat files.")
    parser.add_argument("--out-root", default="tpcds/sql85_chain3_projected")
    parser.add_argument("--scale", default="1")
    parser.add_argument("--threshold", type=int, default=5_000_000)
    args = parser.parse_args()

    export_case(Path(args.data_dir).resolve(), Path(args.out_root).resolve() / scale_label(args.scale), args.scale, args.threshold)
    return 0


if __name__ == "__main__":
    sys.exit(main())
