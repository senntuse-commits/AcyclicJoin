#!/usr/bin/env python3
import argparse
import os
import shutil
import subprocess
from collections import defaultdict
from pathlib import Path
from typing import List


TPCH_TABLES = [
    "nation",
    "part",
    "partsupp",
    "customer",
    "supplier",
    "lineitem",
    "orders",
]

def scale_name(scale: str) -> str:
    return f"sf_{scale}"


def dbgen_scale_arg(scale: str) -> str:
    # The bundled TPC-H dbgen can segfault on fractional scales written as
    # "0.01"; the canonical small-scale spelling for dbgen is ".01".
    if scale.startswith("0."):
        return scale[1:]
    return scale


def table_path(data_dir: Path, table: str) -> Path:
    return data_dir / f"{table}.tbl"


def ensure_dbgen(repo_root: Path) -> None:
    dbgen_dir = repo_root / "tpch" / "dbgen"
    marker = dbgen_dir / ".dbgen_no_rng_test"
    patched_sources = [dbgen_dir / "bm_utils.c"]
    if ((dbgen_dir / "dbgen").exists() and marker.exists() and
            all(src.stat().st_mtime <= marker.stat().st_mtime for src in patched_sources)):
        return

    cflags = '-w -g -DDBNAME=\\"dss\\" -DLINUX -DPOSTGRESQL -DTPCH -D_FILE_OFFSET_BITS=64'
    print("[build] rebuilding tpch/dbgen without -DRNG_TEST")
    subprocess.run(["make", "clean"], cwd=dbgen_dir, check=True)
    subprocess.run(["make", "dbgen", f"CFLAGS={cflags}"], cwd=dbgen_dir, check=True)
    marker.write_text("dbgen built without -DRNG_TEST\n", encoding="utf-8")


def generate_raw_tpch(repo_root: Path, scale: str) -> Path:
    data_dir = repo_root / "tpch" / "generated" / scale_name(scale)
    if all(table_path(data_dir, table).exists() for table in TPCH_TABLES):
        print(f"[reuse] scale={scale} data_dir={data_dir}")
        return data_dir

    ensure_dbgen(repo_root)
    data_dir.mkdir(parents=True, exist_ok=True)
    dbgen_dir = repo_root / "tpch" / "dbgen"
    scale_arg = dbgen_scale_arg(scale)
    print(f"[generate] scale={scale} dbgen_scale={scale_arg}")
    try:
        env = os.environ.copy()
        env["DSS_PATH"] = str(dbgen_dir)
        env["DSS_DIST"] = str(dbgen_dir / "dists.dss")
        subprocess.run(["./dbgen", "-f", "-s", scale_arg, "-b", str(dbgen_dir / "dists.dss")],
                       cwd=dbgen_dir, env=env, check=True)
    except subprocess.CalledProcessError:
        for table in TPCH_TABLES:
            leftover = table_path(dbgen_dir, table)
            if leftover.exists():
                leftover.unlink()
        raise RuntimeError(
            "TPC-H dbgen failed. This script no longer fabricates fallback data; "
            "fix dbgen first so the generated dataset is real."
        )

    for table in TPCH_TABLES:
        src = dbgen_dir / f"{table}.tbl"
        if not src.exists():
            raise RuntimeError(f"dbgen did not create {src}")
        dst = table_path(data_dir, table)
        if dst.exists():
            dst.unlink()
        shutil.move(str(src), dst)
    return data_dir


def to_int(value: str) -> int:
    if value == "":
        return 0
    return int(value)


def project_table(data_dir: Path, table: str, columns: List[int], out_path: Path) -> List[List[int]]:
    result = []
    with table_path(data_dir, table).open("r", encoding="utf-8", errors="ignore") as src, \
            out_path.open("w", encoding="utf-8") as dst:
        for line in src:
            fields = line.rstrip("\n").split("|")
            row = [to_int(fields[col]) for col in columns]
            result.append(row)
            dst.write(" ".join(str(v) for v in row))
            dst.write("\n")
    return result


def children_of(parent: List[int]) -> List[List[int]]:
    children = [[] for _ in parent]
    for node, p in enumerate(parent):
        if p >= 0:
            children[p].append(node)
    return children


def exact_join_rows(tables: List[List[List[int]]],
                    parent: List[int],
                    join_parent: List[int],
                    join_child: List[int],
                    root: int = 0) -> int:
    children = children_of(parent)
    order = []
    stack = [root]
    while stack:
        node = stack.pop()
        order.append(node)
        stack.extend(children[node])

    mult = [[1] * len(table) for table in tables]
    for p in reversed(order):
        for c in children[p]:
            child_sum = defaultdict(int)
            child_col = join_child[c]
            for row, m in zip(tables[c], mult[c]):
                child_sum[row[child_col]] += m

            parent_col = join_parent[c]
            for i, row in enumerate(tables[p]):
                mult[p][i] *= child_sum.get(row[parent_col], 0)
    return sum(mult[root])


def export_ternary_l3(data_dir: Path, out_dir: Path, scale: str) -> int:
    out_dir.mkdir(parents=True, exist_ok=True)
    specs = [
        ("lineitem", [0, 1, 2], "R1_lineitem.tbl"),
        ("orders", [0, 1], "R2_orders.tbl"),
        ("part", [0, 5], "R3_part.tbl"),
        ("supplier", [0, 3], "R4_supplier.tbl"),
        ("customer", [0, 3], "R5_customer.tbl"),
        ("orders", [1, 0], "R6_orders2.tbl"),
        ("customer", [0, 3], "R7_customer2.tbl"),
        ("part", [0, 5], "R8_part2.tbl"),
        ("partsupp", [0, 1], "R9_partsupp.tbl"),
        ("part", [0, 5], "R10_part3.tbl"),
        ("nation", [0, 2], "R11_nation.tbl"),
        ("supplier", [3, 0], "R12_supplier2.tbl"),
        ("nation", [0, 2], "R13_nation2.tbl"),
    ]
    tables = [project_table(data_dir, table, cols, out_dir / name) for table, cols, name in specs]
    parent = [-1, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3]
    join_parent = [-1, 0, 1, 2, 1, 1, 1, 0, 0, 0, 1, 1, 1]
    join_child = [-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    return exact_join_rows(tables, parent, join_parent, join_child)


def export_binary_l3(data_dir: Path, out_dir: Path, scale: str) -> int:
    out_dir.mkdir(parents=True, exist_ok=True)
    specs = [
        ("lineitem", [0, 1], "R1_lineitem.tbl"),
        ("orders", [0, 1], "R2_orders.tbl"),
        ("partsupp", [0, 1], "R3_partsupp.tbl"),
        ("customer", [0, 3], "R4_customer.tbl"),
        ("orders", [1, 0], "R5_orders2.tbl"),
        ("part", [0, 5], "R6_part.tbl"),
        ("supplier", [0, 3], "R7_supplier.tbl"),
    ]
    tables = [project_table(data_dir, table, cols, out_dir / name) for table, cols, name in specs]
    parent = [-1, 0, 0, 1, 1, 2, 2]
    join_parent = [-1, 0, 1, 1, 1, 0, 1]
    join_child = [-1, 0, 0, 0, 0, 0, 0]
    return exact_join_rows(tables, parent, join_parent, join_child)


def write_expected(out_dir: Path, expected: int) -> None:
    (out_dir / "expected.txt").write_text(f"{expected}\n", encoding="utf-8")


def parse_scales(values: List[str]) -> List[str]:
    scales = []
    for value in values:
        for raw in value.split(","):
            scale = raw.strip()
            if scale:
                scales.append(scale)
    return scales


def main() -> None:
    parser = argparse.ArgumentParser(description="Prepare projected TPC-H L3 join-tree datasets.")
    parser.add_argument("--scales", nargs="+", default=["0.01", "0.02", "0.03"],
                        help="TPC-H scale factors, for example: 0.01,0.02,0.03")
    parser.add_argument("--query", choices=["ternary-l3", "binary-l3", "all"], default="all")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    scales = parse_scales(args.scales)
    if not scales:
        raise ValueError("--scales must contain at least one scale factor")

    for scale in scales:
        data_dir = generate_raw_tpch(repo_root, scale)
        if args.query in ("ternary-l3", "all"):
            out_dir = repo_root / "tpch" / "tpch_ternary_l3_projected" / scale_name(scale)
            expected = export_ternary_l3(data_dir, out_dir, scale)
            write_expected(out_dir, expected)
            print(f"[export] {out_dir} scale={scale} expected={expected}")
        if args.query in ("binary-l3", "all"):
            out_dir = repo_root / "tpch" / "tpch_binary_l3_projected" / scale_name(scale)
            expected = export_binary_l3(data_dir, out_dir, scale)
            write_expected(out_dir, expected)
            print(f"[export] {out_dir} scale={scale} expected={expected}")


if __name__ == "__main__":
    main()
