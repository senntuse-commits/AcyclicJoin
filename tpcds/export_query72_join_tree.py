#!/usr/bin/env python3
"""
Export the red-box part of TPC-DS Q72 as integer join-tree inputs.

The exported tree is rooted at catalog_sales:

                 catalog_sales
     /      |       |       |       |        \
  item     cd      hd      d1      d3    promotion  catalog_returns

Q72 has left outer joins to promotion and catalog_returns. To keep the existing
inner-join benchmark interface, missing promotion/return rows are represented
with a synthetic key 0 and one dummy row in the corresponding table.
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


def load_key_set(path: Path, key_col: int) -> set[int]:
    keys: set[int] = set()
    with path.open("r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            row = split_dat(line)
            if len(row) > key_col and row[key_col]:
                keys.add(to_int(row[key_col]))
    return keys


def build_return_pair_ids(data_dir: Path) -> tuple[dict[tuple[int, int], int], Counter[int]]:
    pair_to_id: dict[tuple[int, int], int] = {}
    pair_counts: Counter[int] = Counter()

    # catalog_returns: cr_item_sk=2, cr_order_number=16.
    with table_path(data_dir, "catalog_returns").open("r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            row = split_dat(line)
            if len(row) <= 16:
                continue
            pair = (to_int(row[2]), to_int(row[16]))
            if pair[0] == 0 or pair[1] == 0:
                continue
            pair_id = pair_to_id.get(pair)
            if pair_id is None:
                pair_id = len(pair_to_id) + 1
                pair_to_id[pair] = pair_id
            pair_counts[pair_id] += 1
    return pair_to_id, pair_counts


def compute_expected(
    data_dir: Path,
    pair_to_id: dict[tuple[int, int], int],
    pair_counts: Counter[int],
    threshold: int,
) -> tuple[int, bool]:
    item_keys = load_key_set(table_path(data_dir, "item"), 0)
    cd_keys = load_key_set(table_path(data_dir, "customer_demographics"), 0)
    hd_keys = load_key_set(table_path(data_dir, "household_demographics"), 0)
    date_keys = load_key_set(table_path(data_dir, "date_dim"), 0)
    promo_keys = load_key_set(table_path(data_dir, "promotion"), 0)
    promo_keys.add(0)

    count = 0
    # catalog_sales:
    # cs_sold_date_sk=0, cs_ship_date_sk=2, cs_bill_cdemo_sk=4,
    # cs_bill_hdemo_sk=5, cs_item_sk=15, cs_promo_sk=16,
    # cs_order_number=17.
    with table_path(data_dir, "catalog_sales").open("r", encoding="utf-8", errors="ignore") as f:
        for line_no, line in enumerate(f, start=1):
            row = split_dat(line)
            if len(row) <= 17:
                continue
            if (
                to_int(row[15]) not in item_keys
                or to_int(row[4]) not in cd_keys
                or to_int(row[5]) not in hd_keys
                or to_int(row[0]) not in date_keys
                or to_int(row[2]) not in date_keys
                or to_int(row[16]) not in promo_keys
            ):
                continue

            pair_id = pair_to_id.get((to_int(row[15]), to_int(row[17])), 0)
            count += max(1, pair_counts[pair_id])
            if count > threshold:
                print(f"[stop] count exceeded threshold={threshold} at catalog_sales line={line_no}", flush=True)
                return count, True
    return count, False


def write_simple_projection(src: Path, dst: Path, columns: list[int], prepend_dummy: bool = False) -> int:
    rows = 0
    with src.open("r", encoding="utf-8", errors="ignore") as fin, dst.open("w", encoding="utf-8") as fout:
        if prepend_dummy:
            fout.write("0 0\n")
            rows += 1
        for line in fin:
            row = split_dat(line)
            if len(row) <= max(columns):
                continue
            fout.write(" ".join(str(to_int(row[c])) for c in columns))
            fout.write("\n")
            rows += 1
    return rows


def write_catalog_sales(data_dir: Path, dst: Path, pair_to_id: dict[tuple[int, int], int]) -> int:
    rows = 0
    with table_path(data_dir, "catalog_sales").open("r", encoding="utf-8", errors="ignore") as fin, dst.open(
        "w", encoding="utf-8"
    ) as fout:
        for line in fin:
            row = split_dat(line)
            if len(row) <= 17:
                continue
            pair_id = pair_to_id.get((to_int(row[15]), to_int(row[17])), 0)
            # item, cd, hd, sold date d1, ship date d3, promo, return pair
            fout.write(
                f"{to_int(row[15])} {to_int(row[4])} {to_int(row[5])} "
                f"{to_int(row[0])} {to_int(row[2])} {to_int(row[16])} {pair_id}\n"
            )
            rows += 1
    return rows


def write_catalog_returns(dst: Path, pair_counts: Counter[int]) -> int:
    rows = 0
    with dst.open("w", encoding="utf-8") as fout:
        fout.write("0 0\n")
        rows += 1
        for pair_id, count in sorted(pair_counts.items()):
            for _ in range(count):
                fout.write(f"{pair_id} {pair_id}\n")
                rows += 1
    return rows


def export_case(data_dir: Path, out_dir: Path, scale: str, threshold: int) -> None:
    pair_to_id, pair_counts = build_return_pair_ids(data_dir)
    expected, stopped = compute_expected(data_dir, pair_to_id, pair_counts, threshold)
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
    rows["R1_catalog_sales"] = write_catalog_sales(data_dir, out_dir / "R1_catalog_sales.tbl", pair_to_id)
    rows["R2_item"] = write_simple_projection(table_path(data_dir, "item"), out_dir / "R2_item.tbl", [0, 0])
    rows["R3_customer_demographics"] = write_simple_projection(
        table_path(data_dir, "customer_demographics"), out_dir / "R3_customer_demographics.tbl", [0, 0]
    )
    rows["R4_household_demographics"] = write_simple_projection(
        table_path(data_dir, "household_demographics"), out_dir / "R4_household_demographics.tbl", [0, 0]
    )
    rows["R5_date_dim_d1"] = write_simple_projection(table_path(data_dir, "date_dim"), out_dir / "R5_date_dim_d1.tbl", [0, 0])
    rows["R6_date_dim_d3"] = write_simple_projection(table_path(data_dir, "date_dim"), out_dir / "R6_date_dim_d3.tbl", [0, 0])
    rows["R7_promotion"] = write_simple_projection(
        table_path(data_dir, "promotion"), out_dir / "R7_promotion.tbl", [0, 0], prepend_dummy=True
    )
    rows["R8_catalog_returns"] = write_catalog_returns(out_dir / "R8_catalog_returns.tbl", pair_counts)

    with (out_dir / "expected.txt").open("w", encoding="utf-8") as f:
        f.write(f"{expected}\n")
    with (out_dir / "stats.txt").open("w", encoding="utf-8") as f:
        f.write(f"scale={scale}\n")
        f.write(f"expected={expected}\n")
        f.write(f"return_pair_keys={len(pair_to_id)}\n")
        for name, row_count in rows.items():
            f.write(f"{name}={row_count}\n")

    print(
        f"[export] {out_dir} scale={scale} expected={expected} catalog_sales_rows={rows['R1_catalog_sales']} return_pair_keys={len(pair_to_id)}",
        flush=True,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Export red-box TPC-DS Q72 join-tree benchmark tables.")
    parser.add_argument("--data-dir", default="tpcds/tools", help="Directory containing TPC-DS .dat files.")
    parser.add_argument("--out-root", default="tpcds/sql72_projected")
    parser.add_argument("--scale", default="1")
    parser.add_argument("--threshold", type=int, default=5_000_000)
    args = parser.parse_args()

    export_case(Path(args.data_dir).resolve(), Path(args.out_root).resolve() / scale_label(args.scale), args.scale, args.threshold)
    return 0


if __name__ == "__main__":
    sys.exit(main())
