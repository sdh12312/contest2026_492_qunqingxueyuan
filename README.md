# FlyReflex

**A bio-inspired, connectome-traceable low-latency safety reflex layer for AI-controlled embodied systems.**

FlyReflex 是一个运行在 openvela 上的独立安全反射层：上层 AI 仍可输出 FORWARD，但当本地视觉输入出现快速逼近风险时，轻量反射引擎会在本地生成 ESCAPE，Safety Arbiter 随即输出 REFLEX_OVERRIDE。项目不声称模拟完整果蝇大脑；它把真实 MaleCNS 连接组拓扑压缩成一个透明、确定性、可迁移到 MCU 的工程原型。

## 选题方向

**自定方向：面向具身智能设备的端侧安全反射。** 项目使用 openvela 的 NuttX 应用运行环境、LVGL 图形能力、framebuffer、NSH 命令和单调时钟，在模拟器固件内完成本地感知计算、指令仲裁、实时可视化与延迟测量。它不是 AI 硬件教程中要求依赖 `ai_agent` 的对话型 Agent，而是利用大赛允许的自由创新方向，解决 AI 控制链路缺少独立低延迟安全兜底的问题。

## Problem

AI Agent 擅长任务理解和规划，但推理耗时、网络依赖和偶发停顿使其不适合作为唯一的毫秒级安全闭环。FlyReflex 把安全响应从较慢且时延不确定的认知路径中解耦：

    AI path:       slow cognition / planning (P0 uses an explicitly simulated command)
    FlyReflex:     local sensing / reflex / deterministic arbitration
    Actuator path: always receives the arbiter's final command

## Demonstrated result

危险场景的关键输出为：

    ai=FORWARD
    state=DANGER
    reflex=ESCAPE
    final=ESCAPE
    decision=REFLEX_OVERRIDE

安全与缓慢接近场景保持 final=FORWARD。输入非法时 fail-safe 输出 STOP。

openvela goldfish ARM64 模拟器内 1000 次实测：reflex compute median 1.200 us、P95 2.144 us；event-to-arbiter median 2.144 us、P95 3.904 us。它们是模拟器测量值，不是 MCU latency。

## Biological inspiration

FlyReflex 使用一个最小三节点聚合回路：

    angular-size proxy --------> LPLC2 aggregate --+
                                                   +--> DNp01 / Giant Fiber --> ESCAPE
    angular-velocity proxy ----> LC4 aggregate ----+

MaleCNS v1.0 直接连接数据（2026-09-18 查询）：

| Direct edge | Synapses | Body-pair edges | Runtime normalized weight |
|---|---:|---:|---:|
| LPLC2 → DNp01 | 4,862 | 185 | 433 / 1000 |
| LC4 → DNp01 | 6,362 | 126 | 567 / 1000 |

DNp01 / Giant Fiber 两侧 bodyId 为 10001（R）和 10010（L）。完整的 311 条细胞对边、原始 API 响应、查询语句和再生成脚本均已提交。

功能依据来自 Ache et al.：LC4 提供 angular-velocity 分量，LPLC2 提供 angular-size 分量，两者直接汇入 GF。LPLC2 对径向扩张运动的选择性另有实验研究支持。详细 claim/evidence/usage 表见 docs/SCIENTIFIC_BASIS.md。

## What is real and what is engineered

![Biological to engineering mapping](docs/images/biological-engineering-mapping.svg)

CONNECTOME_DERIVED：

- 数据集 male-cns:v1.0；
- LPLC2、LC4、DNp01 类型与个体 bodyId；
- 311 条 LPLC2/LC4 → DNp01 直接边；
- 4,862 与 6,362 的聚合突触计数；
- 按两类聚合突触数占比计算出的 433:567 相对权重。

PAPER_DERIVED：

- LPLC2 / LC4 是 looming feature channels；
- LPLC2 提供 size-related 分量，LC4 提供 velocity-related 分量；
- Giant Fiber 汇合输入并参与快速逃逸起飞。

ENGINEERING_PARAMETER：

- synthetic looming 序列；
- size/rate 到节点活动的归一化编码；
- 离散泄漏系数、阈值、clamp；
- DANGER → ESCAPE 和非法输入 → STOP；
- P0 的 AI FORWARD 与其延迟都是模拟，不是 LLM benchmark。

全部参数见 data/engineering/model_params.csv。

## Architecture

![FlyReflex system architecture](docs/images/system-architecture.svg)

    deterministic synthetic looming
                 |
                 v
      size/rate engineering encoder
           |                 |
           v                 v
       LPLC2 aggregate    LC4 aggregate
           \                 /
            \               /
             v             v
        fixed-point leaky DNp01/GF node
                 |
          SAFE / CAUTION / DANGER
                 |
                 v
    simulated AI -----> Safety Arbiter -----> final command
       FORWARD            deterministic         FORWARD/ESCAPE/STOP

热路径使用固定大小结构与整数运算，不分配内存。benchmark 的样本数组只在 benchmark 命令启动时分配。

## openvela integration

openvela 不是标签：FlyReflex 作为 NSH 内建应用被交叉编译进现有 goldfish ARM64 固件，并使用：

- NuttX/openvela 应用与任务运行环境；
- CLOCK_MONOTONIC 本地时钟；
- NSH 命令与标准日志；
- LVGL + openvela framebuffer 的实时 dashboard；
- openvela simulator 中的真实本地运行时间测量。

## Host build and tests

主机版用于快速回归，不代表 openvela latency：

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j4
    ctest --test-dir build --output-on-failure
    ./build/flyreflex_host demo all
    ./build/flyreflex_host bench 1000

## Integrate into an openvela contest workspace

官方比赛仓通过 `contest2026_492_qunqingxueyuan.xml` 中的 manifest linkfile 将 `app/flyreflex` 映射到：

    packages/demos/contest2026_492_flyreflex

使用组委会提供的标准方式获取完整工作区：

    repo init -u https://github.com/open-vela/contest2026_492_qunqingxueyuan \
      -b dev-ai-contest-2026 -m contest2026_492_qunqingxueyuan.xml
    repo sync -c -j8

启用 Application Configuration → Packages → Demos → FlyReflex，然后构建：

    ./build.sh vendor/openvela/boards/vela/configs/goldfish-arm64-v8a-ap/ --cmake menuconfig
    ./build.sh vendor/openvela/boards/vela/configs/goldfish-arm64-v8a-ap/ --cmake -j2

启动模拟器：

    ./emulator.sh cmake_out/vela_goldfish-arm64-v8a-ap/

## Run and demo

在 NSH 中：

    flyreflex demo safe
    flyreflex demo slow
    flyreflex demo danger
    flyreflex demo recovery
    flyreflex demo all --csv
    flyreflex bench 1000
    flyreflex ui

UI 自动循环 SAFE → SLOW → DANGER → RECOVERY，也可以直接点击顶部四个场景按钮。仪表盘显示 collision risk、LPLC2、LC4、DNp01/GF、AI command、local reflex、robot executes、最终决策和本地端到端延迟。绿色表示 AI 正常控制，黄色表示持续监视，红色表示本地反射已覆盖 AI；终端仅在场景、状态或决策改变时输出一行摘要，不再逐帧刷屏。

最快验收方式：启动 `flyreflex ui` 后点击 `DANGER`，应看到状态区变红、`LOCAL REFLEX = ESCAPE`、`ROBOT EXECUTES = ESCAPE` 和 `REFLEX_OVERRIDE`；再点击 `SAFE`，界面应恢复绿色且最终指令回到 `FORWARD`。

## Reproduce the connectome extraction

    python3 tools/connectome/fetch_malecns_reflex.py
    python3 skills/flyreflex-provenance/scripts/verify_provenance.py

查询使用 Janelia neuPrint 公开 API 与固定数据集 male-cns:v1.0。若未来数据发生变化，验证脚本会失败，要求人工审核权重而不是静默接受新计数。

## Benchmark

指标从 looming event 被接收到 engine 完成，以及 arbiter 完成时分别取 CLOCK_MONOTONIC 时间戳。输出 min、mean、median、P95、P99、max。主机与 openvela simulator 结果严格分开，见 docs/BENCHMARK.md。

## Repository structure

    app/flyreflex/          openvela app, core, arbiter, CLI and LVGL UI
    data/connectome/        raw response, direct edges, aggregate edges and query
    data/engineering/       explicit model parameters and classifications
    docs/                   architecture, science, provenance, benchmark and demo
    logs/                   development log; official AI export still required
    skills/                 reusable FlyReflex provenance audit Skill
    tests/                  host regression tests
    tools/connectome/       deterministic neuPrint extraction script

## Limitations

- Synthetic looming input, not a real camera.
- Simulator-first; no claim of measured MCU latency.
- Three aggregate biological nodes, not a full neural simulation.
- Synapse counts set relative structural weights; they are not physiological synaptic efficacy.
- P0 AI command is simulated and no cloud LLM is required for the safety loop.
- The current local repository is migration-ready but is not yet the organizer-created contest repository.

## Future work

Add camera optical-flow input, validate on a low-power MCU/SoC, connect STOP/ESCAPE to a real robot, test richer parallel descending pathways, and compare against a real AI planning path without making safety dependent on the network.

## AI Coding

AI participated in requirement extraction, official-rule checking, primary-source research, neuPrint queries, architecture, C implementation, tests, openvela integration, benchmark design, debugging and documentation. The manually exported official AI Coding conversation package must still be placed in logs/ before submission; logs/development_log.md is an engineering summary, not a substitute.

## References

- MaleCNS v1.0 project and download documentation: https://male-cns.janelia.org/download/
- Berg et al., Cell 2026, complete male CNS connectome: https://doi.org/10.1016/j.cell.2026.08.015
- Ache et al., Current Biology 2019, size and velocity encoding: https://doi.org/10.1016/j.cub.2019.01.079
- Klapoetke et al., Nature 2017, radial motion opponency in LPLC2: https://doi.org/10.1038/nature24626
- Official contest overview: https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/contest_2026/contest_overview.md
- Official code submission guide: https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/contest_2026/code_submission_guide.md
- Official AI Coding log guide: https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/contest_2026/ai_coding_log_guide.md

## License

FlyReflex source code is provided under Apache-2.0. MaleCNS data artifacts retain the dataset's CC BY attribution requirements.
