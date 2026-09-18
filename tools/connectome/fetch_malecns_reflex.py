#!/usr/bin/env python3
"""Fetch the exact MaleCNS v1.0 edges used by FlyReflex.

Only Python's standard library is required.  The script writes both the raw
neuPrint response and review-friendly CSV files.  It never fabricates or fills
missing body IDs or synapse counts.
"""

from __future__ import annotations

import csv
import datetime as dt
import json
import pathlib
import urllib.request


DATASET = "male-cns:v1.0"
ENDPOINT = "https://neuprint.janelia.org/api/custom/custom"
EDGE_QUERY = """
MATCH (pre:Neuron)-[e:ConnectsTo]->(post:Neuron)
WHERE pre.type IN ["LPLC2", "LC4"] AND post.type = "DNp01"
RETURN pre.bodyId AS pre_body_id,
       pre.instance AS pre_instance,
       pre.type AS pre_cell_type,
       post.bodyId AS post_body_id,
       post.instance AS post_instance,
       post.type AS post_cell_type,
       e.weight AS synapse_count
ORDER BY pre.type, post.bodyId, e.weight DESC
""".strip()


def query_neuprint(cypher: str) -> dict:
    request = urllib.request.Request(
        ENDPOINT,
        data=json.dumps({"dataset": DATASET, "cypher": cypher}).encode(),
        headers={
            "Content-Type": "application/json",
            "User-Agent": "FlyReflex-provenance/1.0",
        },
        method="POST",
    )
    with urllib.request.urlopen(request, timeout=60) as response:
        return json.load(response)


def main() -> None:
    root = pathlib.Path(__file__).resolve().parents[2]
    raw_dir = root / "data" / "connectome" / "raw"
    processed_dir = root / "data" / "connectome" / "processed"
    raw_dir.mkdir(parents=True, exist_ok=True)
    processed_dir.mkdir(parents=True, exist_ok=True)

    fetched_at = dt.datetime.now(dt.timezone.utc).replace(microsecond=0).isoformat()
    response = query_neuprint(EDGE_QUERY)
    artifact = {
        "source": ENDPOINT,
        "dataset": DATASET,
        "fetched_at": fetched_at,
        "query": EDGE_QUERY,
        "response": response,
    }
    (raw_dir / "neuprint_lplc2_lc4_to_dnp01.json").write_text(
        json.dumps(artifact, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )

    columns = response["columns"]
    records = [dict(zip(columns, row, strict=True)) for row in response["data"]]
    fieldnames = [
        "source_type",
        "source_dataset",
        "dataset_version",
        "pre_body_id",
        "pre_instance",
        "pre_cell_type",
        "post_body_id",
        "post_instance",
        "post_cell_type",
        "synapse_count",
        "reference",
        "verification_status",
        "notes",
    ]
    edge_path = processed_dir / "connectome_edges.csv"
    with edge_path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fieldnames)
        writer.writeheader()
        for record in records:
            writer.writerow(
                {
                    "source_type": "CONNECTOME_DERIVED",
                    "source_dataset": "MaleCNS",
                    "dataset_version": DATASET,
                    **record,
                    "reference": ENDPOINT,
                    "verification_status": "DIRECT_CONNECTOME",
                    "notes": "Direct ConnectsTo edge returned by neuPrint",
                }
            )

    aggregate: dict[tuple[str, str], dict[str, int]] = {}
    for record in records:
        key = (record["pre_cell_type"], record["post_cell_type"])
        entry = aggregate.setdefault(key, {"synapses": 0, "body_pairs": 0})
        entry["synapses"] += int(record["synapse_count"])
        entry["body_pairs"] += 1

    aggregate_path = processed_dir / "aggregate_edges.csv"
    with aggregate_path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(
            [
                "source_dataset",
                "dataset_version",
                "pre_cell_type",
                "post_cell_type",
                "synapse_count",
                "body_pair_edges",
                "verification_status",
            ]
        )
        for (pre_type, post_type), values in sorted(aggregate.items()):
            writer.writerow(
                [
                    "MaleCNS",
                    DATASET,
                    pre_type,
                    post_type,
                    values["synapses"],
                    values["body_pairs"],
                    "DIRECT_CONNECTOME",
                ]
            )

    print(f"Wrote {len(records)} direct edges to {edge_path}")
    for (pre_type, post_type), values in sorted(aggregate.items()):
        print(
            f"{pre_type} -> {post_type}: {values['synapses']} synapses "
            f"across {values['body_pairs']} body pairs"
        )


if __name__ == "__main__":
    main()

