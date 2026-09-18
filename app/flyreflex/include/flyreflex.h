/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * FlyReflex: a deterministic connectome-traceable safety reflex prototype.
 */

#ifndef FLYREFLEX_H
#define FLYREFLEX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define FLYREFLEX_SCALE 1000
#define FLYREFLEX_MAX_SAMPLES 32

enum flyreflex_command_e
{
  FLYREFLEX_CMD_IDLE = 0,
  FLYREFLEX_CMD_FORWARD,
  FLYREFLEX_CMD_LEFT,
  FLYREFLEX_CMD_RIGHT,
  FLYREFLEX_CMD_STOP,
  FLYREFLEX_CMD_ESCAPE
};

enum flyreflex_state_e
{
  FLYREFLEX_STATE_SAFE = 0,
  FLYREFLEX_STATE_CAUTION,
  FLYREFLEX_STATE_DANGER,
  FLYREFLEX_STATE_INVALID
};

enum flyreflex_decision_e
{
  FLYREFLEX_DECISION_AI = 0,
  FLYREFLEX_DECISION_REFLEX_OVERRIDE,
  FLYREFLEX_DECISION_FAILSAFE
};

enum flyreflex_scenario_e
{
  FLYREFLEX_SCENARIO_SAFE = 0,
  FLYREFLEX_SCENARIO_SLOW,
  FLYREFLEX_SCENARIO_DANGER,
  FLYREFLEX_SCENARIO_RECOVERY,
  FLYREFLEX_SCENARIO_NOISE
};

struct flyreflex_input_s
{
  int32_t angular_size_milli;
  int32_t angular_rate_milli;
  uint32_t frame;
};

struct flyreflex_engine_s
{
  int32_t membrane_milli;
  enum flyreflex_state_e state;
  uint32_t danger_events;
};

struct flyreflex_output_s
{
  int32_t lplc2_activity_milli;
  int32_t lc4_activity_milli;
  int32_t gf_membrane_milli;
  enum flyreflex_state_e state;
  enum flyreflex_command_e command;
  bool input_valid;
};

struct flyreflex_arbitration_s
{
  enum flyreflex_command_e ai_command;
  enum flyreflex_command_e reflex_command;
  enum flyreflex_command_e final_command;
  enum flyreflex_decision_e decision;
};

struct flyreflex_latency_s
{
  uint64_t event_ns;
  uint64_t reflex_start_ns;
  uint64_t reflex_end_ns;
  uint64_t arbiter_end_ns;
};

struct flyreflex_sample_s
{
  struct flyreflex_input_s input;
  struct flyreflex_output_s reflex;
  struct flyreflex_arbitration_s arbitration;
  struct flyreflex_latency_s latency;
};

struct flyreflex_stats_s
{
  uint64_t minimum_ns;
  uint64_t mean_ns;
  uint64_t median_ns;
  uint64_t p95_ns;
  uint64_t p99_ns;
  uint64_t maximum_ns;
};

struct flyreflex_scenario_data_s
{
  enum flyreflex_scenario_e kind;
  const char *name;
  const int16_t *sizes;
  size_t count;
};

void flyreflex_engine_reset(struct flyreflex_engine_s *engine);
void flyreflex_engine_step(struct flyreflex_engine_s *engine,
                           const struct flyreflex_input_s *input,
                           struct flyreflex_output_s *output);
void flyreflex_arbitrate(enum flyreflex_command_e ai_command,
                         const struct flyreflex_output_s *reflex,
                         struct flyreflex_arbitration_s *result);

const struct flyreflex_scenario_data_s *
flyreflex_get_scenario(enum flyreflex_scenario_e scenario);
void flyreflex_make_input(const struct flyreflex_scenario_data_s *scenario,
                          size_t index, struct flyreflex_input_s *input);

int flyreflex_run_scenario(enum flyreflex_scenario_e scenario, bool csv,
                           struct flyreflex_sample_s *samples,
                           size_t sample_capacity, size_t *sample_count);
int flyreflex_run_benchmark(size_t iterations,
                            struct flyreflex_stats_s *reflex_stats,
                            struct flyreflex_stats_s *end_to_end_stats);
void flyreflex_print_sample(const char *scenario,
                            const struct flyreflex_sample_s *sample, bool csv,
                            bool print_header);

const char *flyreflex_command_name(enum flyreflex_command_e command);
const char *flyreflex_state_name(enum flyreflex_state_e state);
const char *flyreflex_decision_name(enum flyreflex_decision_e decision);

#ifdef __NuttX__
int flyreflex_ui_run(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* FLYREFLEX_H */

