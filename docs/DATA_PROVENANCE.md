# Data provenance

## Source

- Project: Janelia MaleCNS connectome
- Dataset: male-cns:v1.0
- API: https://neuprint.janelia.org/api/custom/custom
- Download documentation: https://male-cns.janelia.org/download/
- Dataset license: CC BY
- Retrieval date: 2026-09-18 UTC

## Query

The exact Cypher text is stored in data/connectome/provenance/neuprint_query.cypher. It selects direct ConnectsTo edges from LPLC2 or LC4 neurons to DNp01 and returns pre/post bodyId, instance, type and edge weight.

## Reproduction

    python3 tools/connectome/fetch_malecns_reflex.py

The script uses only the Python standard library and writes:

- data/connectome/raw/neuprint_lplc2_lc4_to_dnp01.json: request metadata and untouched API response;
- data/connectome/processed/connectome_edges.csv: 311 direct cell-pair edges;
- data/connectome/processed/aggregate_edges.csv: review-friendly type aggregates.

Then validate:

    python3 skills/flyreflex-provenance/scripts/verify_provenance.py

The verifier intentionally fails if the dataset identifier, aggregate counts or direct-edge count changes.

## Transformation

    raw ConnectsTo edges
      -> group by pre type and DNp01 post type
      -> sum edge weight
      -> LPLC2 4862, LC4 6362
      -> divide each by total 11224
      -> round to integer scale 1000
      -> LPLC2 433, LC4 567

No optimization, learned fit or hidden parameter search is used.

## Body IDs

Both DNp01/GF bodies are preserved in the edge table:

- 10001: DNp01(GF)_R
- 10010: DNp01(GF)_L

All upstream LPLC2 and LC4 body IDs remain row-level data in connectome_edges.csv.

## Verification classes

- DIRECT_CONNECTOME: exact MaleCNS edge returned by neuPrint.
- PAPER_SUPPORTED: functional relationship established in an experimental publication.
- ENGINEERING_ASSUMPTION: runtime mapping used for the prototype without a claim of direct biological measurement.

Runtime parameter classifications are stored separately in data/engineering/model_params.csv.

## Update policy

Never silently update the runtime model after a data refresh. If counts change, first document the dataset/version change, review raw edges, recompute the transparent weights, rerun tests and regenerate benchmark results.

