#!/usr/bin/env python3
"""Generate and export TPC-DS Q85 web_returns-star datasets by scale factor."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

from export_query85_returns_star import export_case, scale_label


REQUIRED_TABLES = [
    "web_returns",
    "customer_demographics",
    "customer_address",
    "reason",
]

TABLE_GENERATORS = {
    "web_returns": "web_sales",
}


def has_required_tables(data_dir: Path) -> bool:
    return all((data_dir / f"{table}.dat").exists() for table in REQUIRED_TABLES)


def ensure_dsdgen(tools_dir: Path) -> Path:
    dsdgen = tools_dir / ("dsdgen.exe" if sys.platform.startswith("win") else "dsdgen")
    if not dsdgen.exists():
        if sys.platform.startswith("win"):
            raise FileNotFoundError(f"missing {dsdgen}")
        for pattern in ("*.o", "*.exe"):
            for path in tools_dir.glob(pattern):
                path.unlink(missing_ok=True)
        for name in ("dsdgen", "qgen", "mkheader", "distcomp"):
            (tools_dir / name).unlink(missing_ok=True)
        print(f"[build] missing {dsdgen}; running make -f Makefile.suite OS=LINUX LINUX_CFLAGS='-g -Wall -fcommon'", flush=True)
        subprocess.run(
            ["make", "-f", "Makefile.suite", "OS=LINUX", "LINUX_CFLAGS=-g -Wall -fcommon"],
            cwd=str(tools_dir),
            check=True,
        )
    if not dsdgen.exists():
        raise FileNotFoundError(f"missing {dsdgen}")
    return dsdgen


def generate_table(tools_dir: Path, data_dir: Path, scale: str, table: str) -> None:
    dsdgen = ensure_dsdgen(tools_dir)
    data_dir.mkdir(parents=True, exist_ok=True)
    if sys.platform.startswith("win"):
        cmd = [str(dsdgen), "/f", "/dir", str(data_dir), "/sc", str(scale), "/table", table, "/quiet", "Y"]
    else:
        cmd = [str(dsdgen), "-force", "-dir", str(data_dir), "-scale", str(scale), "-table", table, "-quiet", "Y"]
    print(f"[generate] scale={scale} table={table}", flush=True)
    subprocess.run(cmd, cwd=str(tools_dir), check=True)


def ensure_scale_data(tools_dir: Path, data_root: Path, scale: str, source_sf1: Path | None) -> Path:
    if scale == "1" and source_sf1 is not None and has_required_tables(source_sf1):
        print(f"[reuse] scale=1 data_dir={source_sf1}", flush=True)
        return source_sf1

    data_dir = data_root / scale_label(scale)
    if has_required_tables(data_dir):
        print(f"[reuse] scale={scale} data_dir={data_dir}", flush=True)
        return data_dir

    for table in REQUIRED_TABLES:
        if not (data_dir / f"{table}.dat").exists():
            generator = TABLE_GENERATORS.get(table, table)
            generate_table(tools_dir, data_dir, scale, generator)
            if not (data_dir / f"{table}.dat").exists():
                raise FileNotFoundError(f"missing {data_dir / f'{table}.dat'} after generating table {generator}")
    return data_dir


def main() -> int:
    parser = argparse.ArgumentParser(description="Prepare TPC-DS Q85 web_returns-star projected datasets.")
    parser.add_argument("--scales", default="1", help="Comma-separated scale factors.")
    parser.add_argument("--tools-dir", default="tpcds/tools")
    parser.add_argument("--data-root", default="tpcds/generated")
    parser.add_argument("--out-root", default="tpcds/sql85_returns_star_projected")
    parser.add_argument("--source-sf1", default="tpcds/tools")
    parser.add_argument("--threshold", type=int, default=5_000_000)
    args = parser.parse_args()

    tools_dir = Path(args.tools_dir).resolve()
    data_root = Path(args.data_root).resolve()
    out_root = Path(args.out_root).resolve()
    source_sf1 = Path(args.source_sf1).resolve() if args.source_sf1 else None
    out_root.mkdir(parents=True, exist_ok=True)

    for raw in args.scales.split(","):
        scale = raw.strip()
        if not scale:
            continue
        data_dir = ensure_scale_data(tools_dir, data_root, scale, source_sf1)
        export_case(data_dir, out_root / scale_label(scale), scale, args.threshold)
    return 0


if __name__ == "__main__":
    sys.exit(main())
