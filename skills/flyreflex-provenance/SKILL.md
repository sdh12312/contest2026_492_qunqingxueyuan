---
name: flyreflex-provenance
description: Refresh and audit FlyReflex MaleCNS evidence, connectome-to-model mapping, and reproducibility artifacts. Use for scientific provenance checks, MaleCNS dataset refreshes, or pre-submission claim audits in the FlyReflex repository.
---

# FlyReflex provenance

Keep biological evidence and engineering choices separate.

1. Read docs/DATA_PROVENANCE.md and data/engineering/model_params.csv.
2. For a live refresh, run python3 tools/connectome/fetch_malecns_reflex.py. This is allowed to replace only the generated files under data/connectome/raw/ and data/connectome/processed/.
3. Run python3 skills/flyreflex-provenance/scripts/verify_provenance.py to verify the checked-in artifacts. Add --refresh only when the user asks for current data or a pre-submission refresh.
4. If live counts change, do not silently keep the runtime weights. Report the dataset version and changed counts, update the transparent normalization in code and model_params.csv, then rerun host tests and openvela integration tests.
5. Describe topology and direct edge counts as CONNECTOME_DERIVED; describe functional feature roles supported by experiments as PAPER_DERIVED; describe encoders, thresholds, leak, clamp, and action mapping as ENGINEERING_PARAMETER.

Never infer a body ID or synapse count from a neuron name, and never call this a faithful simulation of fly neural dynamics.
