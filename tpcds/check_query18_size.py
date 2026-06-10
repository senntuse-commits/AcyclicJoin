#!/usr/bin/env python3
"""
Count the materialized join-tree size of TPC-DS Query 18 without building the result.

The SQL18 join tree is:

    catalog_sales
    /     |      |        \
 date   item   cd1     customer
                       /        \
                     cd2   customer_address

This script reads TPC-DS .dat files directly and checks only the join tree:
  catalog_sales.cs_sold_date_sk = date_dim.d_date_sk
  catalog_sales.cs_item_sk = item.i_item_sk
  catalog_sales.cs_bill_cdemo_sk = cd1.cd_demo_sk
  catalog_sales.cs_bill_customer_sk = customer.c_customer_sk
  customer.c_current_cdemo_sk = cd2.cd_demo_sk
  customer.c_current_addr_sk = customer_address.ca_address_sk

It intentionally does not apply SQL18 selection predicates, group by, rollup, or limit.
It reports the number of catalog_sales rows that survive the join tree.
If the count exceeds --threshold, it stops early and exits with code 2.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path


def split_dat(line: str) -> list[str]:
    line = line.rstrip("\n")
    if line.endswith("|"):
        line = line[:-1]
    return line.split("|")


def table_path(data_dir: Path, table: str) -> Path:
    path = data_dir / f"{table}.dat"
    if not path.exists():
        raise FileNotFoundError(f"missing {path}")
    return path


def load_key_set(path: Path, key_col: int, pred=None) -> set[str]:
    keys: set[str] = set()
    with path.open("r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            row = split_dat(line)
            if len(row) <= key_col:
                continue
            if pred is None or pred(row):
                key = row[key_col]
                if key:
                    keys.add(key)
    return keys


def generate_data(tools_dir: Path, scale: str, data_dir: Path) -> None:
    candidates = [tools_dir / "dsdgen.exe", tools_dir / "dsdgen"] if os.name == "nt" else [tools_dir / "dsdgen"]
    dsdgen = next((p for p in candidates if p.exists()), None)
    if dsdgen is None:
        raise FileNotFoundError(
            f"dsdgen not found in {tools_dir}. Build TPC-DS tools first, then rerun with --generate."
        )
    data_dir.mkdir(parents=True, exist_ok=True)
    cmd = [str(dsdgen), "-sc", str(scale), "-DIR", str(data_dir)]
    print("[generate]", " ".join(cmd), flush=True)
    subprocess.run(cmd, cwd=str(tools_dir), check=True)


def compute_query18_size(data_dir: Path, threshold: int) -> tuple[int, dict[str, int], bool]:
    # date_dim: d_date_sk at col 0.
    date_keys = load_key_set(table_path(data_dir, "date_dim"), 0)

    # item: i_item_sk at col 0. No SQL18 filter on item.
    item_keys = load_key_set(table_path(data_dir, "item"), 0)

    # customer_demographics: cd_demo_sk at col 0. Used twice as cd1 and cd2.
    cd_all = load_key_set(table_path(data_dir, "customer_demographics"), 0)

    # customer_address: ca_address_sk at col 0.
    addr_keys = load_key_set(table_path(data_dir, "customer_address"), 0)

    # customer:
    # c_customer_sk=0, c_current_cdemo_sk=2, c_current_addr_sk=4.
    # Keep customers that can join to both cd2 and customer_address.
    customer_ok: set[str] = set()
    with table_path(data_dir, "customer").open("r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            r = split_dat(line)
            if len(r) <= 4:
                continue
            c_key = r[0]
            c_current_cdemo = r[2]
            c_current_addr = r[4]
            if c_key and c_current_cdemo in cd_all and c_current_addr in addr_keys:
                customer_ok.add(c_key)

    stats = {
        "date_dim": len(date_keys),
        "item": len(item_keys),
        "customer_demographics_cd1_cd2": len(cd_all),
        "customer_address": len(addr_keys),
        "customer_joinable_to_cd2_address": len(customer_ok),
    }

    # catalog_sales columns:
    # cs_sold_date_sk=0, cs_bill_customer_sk=3,
    # cs_bill_cdemo_sk=4, cs_item_sk=15.
    count = 0
    stopped_early = False
    with table_path(data_dir, "catalog_sales").open("r", encoding="utf-8", errors="ignore") as f:
        for line_no, line in enumerate(f, start=1):
            r = split_dat(line)
            if len(r) <= 15:
                continue
            if (
                r[0] in date_keys
                and r[15] in item_keys
                and r[4] in cd_all
                and r[3] in customer_ok
            ):
                count += 1
                if count > threshold:
                    stopped_early = True
                    print(
                        f"[stop] count exceeded threshold={threshold} at catalog_sales line={line_no}",
                        flush=True,
                    )
                    break

    return count, stats, stopped_early


def main() -> int:
    parser = argparse.ArgumentParser(description="Check TPC-DS SQL18 join-tree output size.")
    parser.add_argument(
        "--data-dir",
        default="tpcds/tools/data",
        help="Directory containing TPC-DS .dat files.",
    )
    parser.add_argument(
        "--threshold",
        type=int,
        default=5_000_000,
        help="Ask/stop threshold. Default: 5,000,000 rows.",
    )
    parser.add_argument(
        "--generate",
        action="store_true",
        help="Generate TPC-DS data before counting. Requires built dsdgen.",
    )
    parser.add_argument("--scale", default="1", help="TPC-DS dsdgen scale factor.")
    parser.add_argument(
        "--tools-dir",
        default="tpcds/tools",
        help="TPC-DS tools directory containing dsdgen.",
    )
    args = parser.parse_args()

    data_dir = Path(args.data_dir).resolve()
    tools_dir = Path(args.tools_dir).resolve()

    if args.generate:
        generate_data(tools_dir, args.scale, data_dir)

    print(f"[input] data_dir={data_dir}", flush=True)
    count, stats, stopped = compute_query18_size(data_dir, args.threshold)

    print("[stats]")
    for k, v in stats.items():
        print(f"  {k}: {v}")
    print(f"[result] sql18_join_tree_rows={count}")

    if stopped:
        print(
            f"[decision] output is larger than {args.threshold}. Please confirm before materializing/testing.",
            flush=True,
        )
        return 2
    print("[decision] output is within threshold; safe to continue to C++ test generation.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
