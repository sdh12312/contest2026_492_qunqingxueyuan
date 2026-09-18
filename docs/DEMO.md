# Demo guide

## One-command scenarios

In openvela NSH:

    flyreflex demo safe
    flyreflex demo slow
    flyreflex demo danger
    flyreflex demo recovery
    flyreflex ui

Expected results:

| Scenario | Looming | Reflex | Final | Meaning |
|---|---|---|---|---|
| safe | low/static | IDLE | FORWARD | Safety layer does not interfere |
| slow | ordinary approach | IDLE/CAUTION | FORWARD | No nuisance stop |
| danger | rapidly rising | ESCAPE | ESCAPE | AI FORWARD is overridden |
| recovery | danger then recedes | ESCAPE then IDLE | ESCAPE then FORWARD | Control returns after recovery |

## 2 minute 30 second narration

0:00–0:20 — Problem

“AI Agents are excellent planners, but inference and network latency are not a deterministic safety loop. FlyReflex adds an independent local reflex between AI and actuators.”

0:20–0:45 — Biological inspiration

“MaleCNS v1.0 provides a traceable complete male fly CNS connectome. We queried every direct LPLC2 and LC4 input to DNp01, the Giant Fiber: 311 cell-pair edges and 11,224 synapses in total.”

0:45–1:05 — Honest mapping

“Topology and relative weights come from the connectome. The size/velocity encoder, leak and thresholds are explicitly engineering parameters. This is connectome-inspired, not a full biological simulation.”

1:05–1:45 — Live demo

1. Show safe: AI FORWARD, final FORWARD.
2. Show slow: risk increases but no override.
3. Show danger: keep the AI command visibly at FORWARD; point to LPLC2/LC4, DNp01 activity, ESCAPE and REFLEX_OVERRIDE.
4. Show recovery if time permits.

1:45–2:10 — Latency and platform

“The engine and arbiter run as an openvela NSH application, use CLOCK_MONOTONIC, and present a live LVGL dashboard. The shown statistics are measured on the openvela simulator, not claimed as MCU latency.”

2:10–2:30 — Value

“The safety response is decoupled from nondeterministic AI planning and can later move into a camera-equipped MCU or low-power SoC without a neural-network runtime.”

## Recording checklist

- Start with the LVGL dashboard in SAFE.
- Keep AI COMMAND: FORWARD visible.
- Capture CAUTION without override.
- Capture DANGER, REFLEX: ESCAPE, FINAL: ESCAPE and REFLEX_OVERRIDE in one frame.
- Capture benchmark console output.
- Briefly show aggregate_edges.csv and model_params.csv.
- End on architecture and limitations.

Do not call host or simulator results MCU latency. Do not call simulated AI delay measured LLM latency.

