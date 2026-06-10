#!/usr/bin/env python3
"""
Export TPC-DS Q85 join-tree inputs as integer tables for the C++ benchmarks.

Only the Q85 join tree is used; SQL85 selection predicates are intentionally
ignored. The exported tree is:

                 web_sales
          /          |          \
      web_page    date_dim    web_returns
                              /   |   |   \
                            cd1  cd2  ca  reason

The edge web_sales-web_returns is a composite join:
  ws_item_sk = wr_item_sk AND ws_order_number = wr_order_number
It is exported as one synthetic integer key so the existing single-column join
interfaces can test the same join tree.
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


def load_key_set(path: Path, key_col: int) -> set[int]:
    keys: set[int] = set()
    with path.open("r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            row = split_dat(line)
            if len(row) > key_col and row[key_col]:
                keys.add(to_int(row[key_col]))
    return keys


def scale_label(scale: str) -> str:
    return "sf_" + scale.replace(".", "p")


def build_return_pair_ids(data_dir: Path, threshold: int) -> tuple[dict[tuple[int, int], int], Counter[int], bool]:
    cd_keys = load_key_set(table_path(data_dir, "customer_demographics"), 0)
    addr_keys = load_key_set(table_path(data_dir, "customer_address"), 0)
    reason_keys = load_key_set(table_path(data_dir, "reason"), 0)

    pair_to_id: dict[tuple[int, int], int] = {}
    wr_counts: Counter[int] = Counter()
    stopped = False

    # web_returns columns:
    # wr_item_sk=2, wr_refunded_cdemo_sk=4, wr_refunded_addr_sk=6,
    # wr_returning_cdemo_sk=8, wr_reason_sk=12, wr_order_number=13.
    with table_path(data_dir, "web_returns").open("r", encoding="utf-8", errors="ignore") as f:
        for row_no, line in enumerate(f, start=1):
            row = split_dat(line)
            if len(row) <= 13:
                continue
            if (
                to_int(row[4]) not in cd_keys
                or to_int(row[8]) not in cd_keys
                or to_int(row[6]) not in addr_keys
                or to_int(row[12]) not in reason_keys
            ):
                continue
            pair = (to_int(row[2]), to_int(row[13]))
            pid = pair_to_id.get(pair)
            if pid is None:
                pid = len(pair_to_id) + 1
                pair_to_id[pair] = pid
            wr_counts[pid] += 1
            if row_no % 1_000_000 == 0 and sum(wr_counts.values()) > threshold:
                stopped = True
                break
    return pair_to_id, wr_counts, stopped


def compute_expected(data_dir: Path, pair_to_id: dict[tuple[int, int], int], wr_counts: Counter[int], threshold: int) -> tuple[int, bool]:
    page_keys = load_key_set(table_path(data_dir, "web_page"), 0)
    date_keys = load_key_set(table_path(data_dir, "date_dim"), 0)

    count = 0
    stopped = False
    # web_sales columns:
    # ws_sold_date_sk=0, ws_item_sk=3, ws_web_page_sk=12, ws_order_number=17.
    with table_path(data_dir, "web_sales").open("r", encoding="utf-8", errors="ignore") as f:
        for line_no, line in enumerate(f, start=1):
            row = split_dat(line)
            if len(row) <= 17:
                continue
            if to_int(row[12]) not in page_keys or to_int(row[0]) not in date_keys:
                continue
            pid = pair_to_id.get((to_int(row[3]), to_int(row[17])))
            if pid is None:
                continue
            count += wr_counts[pid]
            if count > threshold:
                stopped = True
                print(f"[stop] count exceeded threshold={threshold} at web_sales line={line_no}", flush=True)
                break
    return count, stopped


def write_simple_projection(src: Path, dst: Path, columns: list[int]) -> int:
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


def write_web_sales(data_dir: Path, dst: Path, pair_to_id: dict[tuple[int, int], int]) -> int:
    rows = 0
    with table_path(data_dir, "web_sales").open("r", encoding="utf-8", errors="ignore") as fin, dst.open(
        "w", encoding="utf-8"
    ) as fout:
        for line in fin:
            row = split_dat(line)
            if len(row) <= 17:
                continue
            pair_id = pair_to_id.get((to_int(row[3]), to_int(row[17])), 0)
            fout.write(f"{to_int(row[12])} {to_int(row[0])} {pair_id}\n")
            rows += 1
    return rows


def write_web_returns(data_dir: Path, dst: Path, pair_to_id: dict[tuple[int, int], int]) -> int:
    rows = 0
    with table_path(data_dir, "web_returns").open("r", encoding="utf-8", errors="ignore") as fin, dst.open(
        "w", encoding="utf-8"
    ) as fout:
        for line in fin:
            row = split_dat(line)
            if len(row) <= 13:
                continue
            pair_id = pair_to_id.get((to_int(row[2]), to_int(row[13])), 0)
            fout.write(
                f"{pair_id} {to_int(row[4])} {to_int(row[8])} {to_int(row[6])} {to_int(row[12])}\n"
            )
            rows += 1
    return rows


def export_case(data_dir: Path, out_dir: Path, scale: str, threshold: int) -> None:
    pair_to_id, wr_counts, early = build_return_pair_ids(data_dir, threshold)
    if early:
        print(f"[stop] {out_dir.name}: web_returns side exceeded threshold={threshold}", flush=True)
        raise SystemExit(2)

    expected, stopped = compute_expected(data_dir, pair_to_id, wr_counts, threshold)
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
    # R1 web_sales(ws_web_page_sk, ws_sold_date_sk, return_pair_key)
    rows["R1_web_sales"] = write_web_sales(data_dir, out_dir / "R1_web_sales.tbl", pair_to_id)
    rows["R2_web_page"] = write_simple_projection(table_path(data_dir, "web_page"), out_dir / "R2_web_page.tbl", [0, 0])
    rows["R3_date_dim"] = write_simple_projection(table_path(data_dir, "date_dim"), out_dir / "R3_date_dim.tbl", [0, 0])
    # R4 web_returns(return_pair_key, wr_refunded_cdemo_sk, wr_returning_cdemo_sk, wr_refunded_addr_sk, wr_reason_sk)
    rows["R4_web_returns"] = write_web_returns(data_dir, out_dir / "R4_web_returns.tbl", pair_to_id)
    rows["R5_cd1"] = write_simple_projection(
        table_path(data_dir, "customer_demographics"), out_dir / "R5_cd1.tbl", [0, 0]
    )
    rows["R6_cd2"] = write_simple_projection(
        table_path(data_dir, "customer_demographics"), out_dir / "R6_cd2.tbl", [0, 0]
    )
    rows["R7_customer_address"] = write_simple_projection(
        table_path(data_dir, "customer_address"), out_dir / "R7_customer_address.tbl", [0, 0]
    )
    rows["R8_reason"] = write_simple_projection(table_path(data_dir, "reason"), out_dir / "R8_reason.tbl", [0, 0])

    with (out_dir / "expected.txt").open("w", encoding="utf-8") as f:
        f.write(f"{expected}\n")
    with (out_dir / "stats.txt").open("w", encoding="utf-8") as f:
        f.write(f"scale={scale}\n")
        f.write(f"expected={expected}\n")
        f.write(f"return_pair_keys={len(pair_to_id)}\n")
        for name, row_count in rows.items():
            f.write(f"{name}={row_count}\n")

    print(
        f"[export] {out_dir} scale={scale} expected={expected} web_sales_rows={rows['R1_web_sales']} web_returns_rows={rows['R4_web_returns']} pair_keys={len(pair_to_id)}",
        flush=True,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Export TPC-DS Q85 join-tree benchmark tables.")
    parser.add_argument("--data-dir", default="tpcds/tools", help="Directory containing TPC-DS .dat files.")
    parser.add_argument("--out-root", default="tpcds/sql85_projected")
    parser.add_argument("--scale", default="1")
    parser.add_argument("--threshold", type=int, default=5_000_000)
    args = parser.parse_args()

    export_case(Path(args.data_dir).resolve(), Path(args.out_root).resolve() / scale_label(args.scale), args.scale, args.threshold)
    return 0


if __name__ == "__main__":
    sys.exit(main())
