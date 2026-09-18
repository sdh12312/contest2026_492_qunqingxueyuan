# Benchmark

## What is measured

The danger input is already available before timing begins.

- reflex_compute: immediately before encoder/engine through engine output;
- end_to_end: event receipt through Safety Arbiter final command.

CLOCK_MONOTONIC is used. Each iteration resets engine state. The input is deterministic and forces the danger path. Sorting and reporting happen after all measurements.

## Host reference result

Measured on the local WSL host reference build on 2026-09-18. This is not openvela and not an MCU result.

| Path | Iterations | Min ns | Mean ns | Median ns | P95 ns | P99 ns | Max ns |
|---|---:|---:|---:|---:|---:|---:|---:|
| reflex_compute | 1000 | 10 | 18 | 20 | 20 | 21 | 60 |
| end_to_end | 1000 | 30 | 36 | 40 | 40 | 41 | 90 |

These sub-microsecond host numbers mainly show that the core is tiny; timer-call overhead and host optimization dominate.

## openvela simulator result

Measured on the openvela goldfish ARM64 simulator on 2026-09-18. This is simulator latency, not MCU latency.

| Path | Iterations | Min ns | Mean ns | Median ns | P95 ns | P99 ns | Max ns |
|---|---:|---:|---:|---:|---:|---:|---:|
| reflex_compute | 1000 | 1,152 | 1,400 | 1,200 | 2,144 | 9,776 | 17,552 |
| end_to_end | 1000 | 2,080 | 2,565 | 2,144 | 3,904 | 12,848 | 57,872 |

Exact NSH output:

    FlyReflex benchmark (measured locally with CLOCK_MONOTONIC)
    environment=openvela simulator/target iterations=1000 units=ns
    path,min,mean,median,p95,p99,max
    reflex_compute,1152,1400,1200,2144,9776,17552
    end_to_end,2080,2565,2144,3904,12848,57872

## AI latency

P0 uses a simulated AI FORWARD command. No LLM round-trip measurement is included. Any future simulated delay must be labeled SIMULATED AI DELAY.
