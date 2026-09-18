# Scientific basis

FlyReflex is connectome-inspired engineering, not a faithful fruit-fly simulation.

## Claim / evidence / usage

| Claim | Evidence class and source | FlyReflex usage |
|---|---|---|
| MaleCNS v1.0 is a whole male Drosophila CNS connectome with neuron annotations and synaptic connectivity | CONNECTOME_DERIVED; MaleCNS project and Berg et al., Cell 2026 | Fixed dataset identifier and provenance source |
| DNp01 is annotated as Giant Fiber and has bodyId 10001 (R) and 10010 (L) | CONNECTOME_DERIVED; neuPrint male-cns:v1.0 query | Aggregated descending/reflex node |
| LPLC2 directly connects to DNp01 in MaleCNS v1.0 | CONNECTOME_DERIVED; 185 body-pair edges and 4,862 synapses in checked-in query result | Topology and normalized relative input weight |
| LC4 directly connects to DNp01 in MaleCNS v1.0 | CONNECTOME_DERIVED; 126 body-pair edges and 6,362 synapses in checked-in query result | Topology and normalized relative input weight |
| LPLC2 and LC4 directly synapse onto GF | PAPER_DERIVED; Ache et al. 2019 | Supports choosing the three-node circuit |
| LC4 supplies an angular-velocity component while LPLC2 supplies an angular-size component to GF looming response | PAPER_DERIVED; Ache et al. 2019 | Guides the two engineering encoder channels |
| LPLC2 neurons respond selectively to outward radial motion and project to Giant Fiber | PAPER_DERIVED; Klapoetke et al. 2017 | Supports using LPLC2 as a looming feature channel |
| A single fixed-point leaky node approximates DNp01/GF dynamics | ENGINEERING_PARAMETER; no biological source claimed | Small deterministic embedded model |
| Threshold crossing should map to ESCAPE | ENGINEERING_PARAMETER product decision | Demonstrates independent safety override |

## Direct MaleCNS result

Dataset: male-cns:v1.0

Query date: 2026-09-18 UTC

Endpoint: https://neuprint.janelia.org/api/custom/custom

| Pre type | Post type | Direct synapses | Body-pair edges |
|---|---|---:|---:|
| LPLC2 | DNp01 | 4,862 | 185 |
| LC4 | DNp01 | 6,362 | 126 |

The runtime weights are:

    LPLC2 weight = round(1000 * 4862 / 11224) = 433
    LC4 weight   = round(1000 * 6362 / 11224) = 567

This preserves only a relative structural ratio. Synapse count is not treated as physiological efficacy.

## Why DNp01 is acceptable

The project brief warned not to force a preselected descending neuron. Here DNp01 is used only after two independent checks:

1. MaleCNS v1.0 directly annotates DNp01(GF) and returns direct LPLC2/LC4 connections.
2. The experimental literature identifies GF as the common postsynaptic target combining these looming feature channels and participating in escape takeoff.

## Claim ceiling

Safe statements:

- connectome-traceable;
- topology derived from MaleCNS v1.0;
- connectome-inspired engineering dynamics;
- local low-latency safety-layer prototype.

Statements this project must not make:

- faithful fruit-fly brain simulation;
- biologically measured membrane threshold or time constant;
- MCU latency when measured in the simulator;
- real LLM latency when using the simulated AI command.

## Primary references

1. Berg S. et al. Sexual dimorphism in the complete Drosophila male central nervous system connectome. Cell 189, 5504–5526.e15 (2026). https://doi.org/10.1016/j.cell.2026.08.015
2. MaleCNS download and data documentation. https://male-cns.janelia.org/download/
3. Ache J. M. et al. Neural basis for looming size and velocity encoding in the Drosophila giant fiber escape pathway. Current Biology 29 (2019). https://doi.org/10.1016/j.cub.2019.01.079
4. Klapoetke N. C. et al. Ultra-selective looming detection from radial motion opponency. Nature 551 (2017). https://doi.org/10.1038/nature24626

