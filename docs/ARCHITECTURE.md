# Architecture

## Product boundary

FlyReflex sits between a slow AI command path and an actuator command path. It does not reinterpret the AI's intent. It only applies a small deterministic safety rule when local evidence reaches DANGER or becomes invalid.

## Runtime data flow

    Scenario generator / future camera
                 |
                 v
      Input {size_milli, rate_milli}
                 |
                 v
    Connectome-inspired engineering encoder
          |                    |
          v                    v
       LPLC2                 LC4
      size proxy          velocity proxy
          \                    /
           +---- 433 : 567 ---+
                    |
                    v
           DNp01/GF leaky node
                    |
      SAFE / CAUTION / DANGER / INVALID
                    |
          +---------+---------+
          |                   |
          v                   v
    Reflex command        AI command
          \                   /
           +-- Safety Arbiter +
                    |
                    v
             Final command

## Deterministic arbitration

| Condition | Final command | Decision source |
|---|---|---|
| Invalid input or command | STOP | FAILSAFE |
| DANGER | ESCAPE, with STOP fallback | REFLEX_OVERRIDE |
| SAFE or CAUTION | AI command | AI |

There is no probabilistic arbitration and no cloud dependency.

## Model equation

All values use a 1000-point integer scale.

    drive = (433 * LPLC2 + 567 * LC4) / 1000
    membrane_next = (350 * membrane / 1000) + drive
    membrane_next = min(membrane_next, 1200)

The two input weights are normalized from MaleCNS aggregate direct synapse counts. The leak, thresholds, scale and clamp are engineering parameters.

## Timing boundary

    event_ns         input accepted
    reflex_start_ns  immediately before encoder/engine
    reflex_end_ns    immediately after engine output
    arbiter_end_ns   immediately after final command

The runtime uses CLOCK_MONOTONIC. The UI reports one event; the benchmark sorts repeated measurements and reports min/mean/median/P95/P99/max.

## Memory and portability

- Core hot path uses fixed-size structs and integer arithmetic.
- No neural-network runtime and no floating-point dependency.
- No allocation in looming, engine or arbiter calls.
- Benchmark allocation occurs outside the measured loop.
- Core builds unchanged on a POSIX host and openvela/NuttX.

