#!/usr/bin/env python3
"""Verify FlyReflex's checked-in connectome evidence and optional live refresh."""

from __future__ import annotations

import argparse
import csv
import json
import pathlib
import subprocess
import sys


EXPECTED = {
    ("LC4", "DNp01"): (6362, 126),
    ("LPLC2", "DNp01"): (4862, 185),
}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--refresh",
        action="store_true",
        help="query MaleCNS v1.0 before validating generated artifacts",
    )
    args = parser.parse_args()

    root = pathlib.Path(__file__).resolve().parents[3]
    if args.refresh:
        subprocess.run(
            [sys.executable, str(root / "tools/connectome/fetch_malecns_reflex.py")],
            check=True,
            cwd=root,
        )

    raw_path = (
        root / "data/connectome/raw/neuprint_lplc2_lc4_to_dnp01.json"
    )
    aggregate_path = root / "data/connectome/processed/aggregate_edges.csv"
    edge_path = root / "data/connectome/processed/connectome_edges.csv"

    raw = json.loads(raw_path.read_text(encoding="utf-8"))
    if raw["dataset"] != "male-cns:v1.0":
        raise SystemExit(f"unexpected dataset: {raw['dataset']}")

    with aggregate_path.open(encoding="utf-8", newline="") as stream:
        rows = list(csv.DictReader(stream))
    actual = {
        (row["pre_cell_type"], row["post_cell_type"]): (
            int(row["synapse_count"]),
            int(row["body_pair_edges"]),
        )
        for row in rows
    }
    if actual != EXPECTED:
        raise SystemExit(
            "MaleCNS aggregate changed; audit runtime weights before accepting:\n"
            f"expected={EXPECTED}\nactual={actual}"
        )

    with edge_path.open(encoding="utf-8", newline="") as stream:
        edges = list(csv.DictReader(stream))
    if len(edges) != sum(pair_count for _, pair_count in EXPECTED.values()):
        raise SystemExit(f"expected 311 direct edges, got {len(edges)}")
    if any(row["verification_status"] != "DIRECT_CONNECTOME" for row in edges):
        raise SystemExit("edge table contains a non-direct connectivity claim")

    print("FlyReflex provenance verified")
    print("dataset=male-cns:v1.0 direct_edges=311")
    for key, value in sorted(actual.items()):
        print(f"{key[0]} -> {key[1]}: synapses={value[0]} body_pairs={value[1]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

