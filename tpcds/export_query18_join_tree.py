#!/usr/bin/env python3
"""
Export TPC-DS Q18 join-tree inputs as integer tables for the C++ benchmarks.

Only the Q18 join tree is used; SQL18 selection predicates are intentionally
ignored. The exported tree is:

                 catalog_sales
        /             |          |          \
    date_dim        item        cd1       customer
                                           /      \
                                        cd2   customer_address

The dataset identity is the TPC-DS scale factor. A catalog_sales row cap is
available only as an optional debugging shortcut.
"""

from __future__ import annotations

import argparse
import shutil
import sys
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


def write_projection(src: Path, dst: Path, columns: list[int], limit: int | None = None) -> int:
    rows = 0
    with src.open("r", encoding="utf-8", errors="ignore") as fin, dst.open("w", encoding="utf-8") as fout:
        for line in fin:
            row = split_dat(line)
            if len(row) <= max(columns):
                continue
            fout.write(" ".join(str(to_int(row[c])) for c in columns))
            fout.write("\n")
            rows += 1
            if limit is not None and rows >= limit:
                break
    return rows


def compute_expected(data_dir: Path, catalog_limit: int | None, threshold: int) -> tuple[int, bool]:
    date_keys = load_key_set(table_path(data_dir, "date_dim"), 0)
    item_keys = load_key_set(table_path(data_dir, "item"), 0)
    cd_keys = load_key_set(table_path(data_dir, "customer_demographics"), 0)
    addr_keys = load_key_set(table_path(data_dir, "customer_address"), 0)

    customer_ok: set[int] = set()
    with table_path(data_dir, "customer").open("r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            row = split_dat(line)
            if len(row) <= 4:
                continue
            c_key = to_int(row[0])
            c_current_cdemo = to_int(row[2])
            c_current_addr = to_int(row[4])
            if c_key and c_current_cdemo in cd_keys and c_current_addr in addr_keys:
                customer_ok.add(c_key)

    count = 0
    stopped = False
    with table_path(data_dir, "catalog_sales").open("r", encoding="utf-8", errors="ignore") as f:
        for line_no, line in enumerate(f, start=1):
            if catalog_limit is not None and line_no > catalog_limit:
                break
            row = split_dat(line)
            if len(row) <= 15:
                continue
            if (
                to_int(row[0]) in date_keys
                and to_int(row[15]) in item_keys
                and to_int(row[4]) in cd_keys
                and to_int(row[3]) in customer_ok
            ):
                count += 1
                if count > threshold:
                    stopped = True
                    break
    return count, stopped


def scale_label(scale: str) -> str:
    return "sf_" + scale.replace(".", "p")


def export_case(data_dir: Path, out_dir: Path, scale: str, catalog_limit: int | None, threshold: int) -> None:
    expected, stopped = compute_expected(data_dir, catalog_limit, threshold)
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
    # R1: catalog_sales(cs_sold_date_sk, cs_item_sk, cs_bill_cdemo_sk, cs_bill_customer_sk)
    rows["R1_catalog_sales"] = write_projection(
        table_path(data_dir, "catalog_sales"), out_dir / "R1_catalog_sales.tbl", [0, 15, 4, 3], catalog_limit
    )
    # Add a harmless payload column to one-key dimension tables.
    rows["R2_date_dim"] = write_projection(table_path(data_dir, "date_dim"), out_dir / "R2_date_dim.tbl", [0, 0])
    rows["R3_item"] = write_projection(table_path(data_dir, "item"), out_dir / "R3_item.tbl", [0, 0])
    rows["R4_cd1"] = write_projection(
        table_path(data_dir, "customer_demographics"), out_dir / "R4_cd1.tbl", [0, 0]
    )
    # R5: customer(c_customer_sk, c_current_cdemo_sk, c_current_addr_sk)
    rows["R5_customer"] = write_projection(table_path(data_dir, "customer"), out_dir / "R5_customer.tbl", [0, 2, 4])
    rows["R6_cd2"] = write_projection(
        table_path(data_dir, "customer_demographics"), out_dir / "R6_cd2.tbl", [0, 0]
    )
    rows["R7_customer_address"] = write_projection(
        table_path(data_dir, "customer_address"), out_dir / "R7_customer_address.tbl", [0, 0]
    )

    with (out_dir / "expected.txt").open("w", encoding="utf-8") as f:
        f.write(f"{expected}\n")
    with (out_dir / "stats.txt").open("w", encoding="utf-8") as f:
        f.write(f"scale={scale}\n")
        f.write(f"catalog_limit={catalog_limit if catalog_limit is not None else 'ALL'}\n")
        f.write(f"expected={expected}\n")
        for name, row_count in rows.items():
            f.write(f"{name}={row_count}\n")

    print(
        f"[export] {out_dir} scale={scale} expected={expected} catalog_rows={rows['R1_catalog_sales']}",
        flush=True,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Export TPC-DS Q18 join-tree benchmark tables.")
    parser.add_argument("--data-dir", default="tpcds/tools", help="Directory containing TPC-DS .dat files.")
    parser.add_argument("--out-root", default="tpcds/sql18_projected", help="Output root for projected tables.")
    parser.add_argument("--scale", default="1", help="TPC-DS scale factor represented by --data-dir.")
    parser.add_argument(
        "--catalog-limits",
        default="",
        help="Optional comma-separated catalog_sales prefixes for debugging, e.g. 50000,200000.",
    )
    parser.add_argument("--threshold", type=int, default=5_000_000)
    args = parser.parse_args()

    data_dir = Path(args.data_dir).resolve()
    out_root = Path(args.out_root).resolve()
    out_root.mkdir(parents=True, exist_ok=True)

    if args.catalog_limits.strip():
        for raw in args.catalog_limits.split(","):
            limit = int(raw.strip())
            export_case(data_dir, out_root / f"{scale_label(args.scale)}_cs_{limit}", args.scale, limit, args.threshold)
    else:
        export_case(data_dir, out_root / scale_label(args.scale), args.scale, None, args.threshold)
    return 0


if __name__ == "__main__":
    sys.exit(main())
