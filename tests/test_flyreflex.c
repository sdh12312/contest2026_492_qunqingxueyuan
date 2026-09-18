/* SPDX-License-Identifier: Apache-2.0 */

#include "flyreflex.h"

#include <stdio.h>
#include <string.h>

static unsigned int g_failures;
static unsigned int g_tests;

#define CHECK(condition) \
  do \
    { \
      g_tests++; \
      if (!(condition)) \
        { \
          fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
          g_failures++; \
        } \
    } \
  while (0)

static size_t collect(enum flyreflex_scenario_e scenario,
                      struct flyreflex_sample_s *samples)
{
  size_t count = 0;
  int status = flyreflex_run_scenario(scenario, true, samples,
                                      FLYREFLEX_MAX_SAMPLES, &count);
  CHECK(status == 0);
  CHECK(count <= FLYREFLEX_MAX_SAMPLES);
  return count;
}

static void test_safe_input(void)
{
  struct flyreflex_sample_s samples[FLYREFLEX_MAX_SAMPLES];
  size_t count = collect(FLYREFLEX_SCENARIO_SAFE, samples);
  size_t index;

  for (index = 0; index < count; index++)
    {
      CHECK(samples[index].reflex.state == FLYREFLEX_STATE_SAFE);
      CHECK(samples[index].arbitration.final_command == FLYREFLEX_CMD_FORWARD);
      CHECK(samples[index].arbitration.decision == FLYREFLEX_DECISION_AI);
    }
}

static void test_slow_approach(void)
{
  struct flyreflex_sample_s samples[FLYREFLEX_MAX_SAMPLES];
  size_t count = collect(FLYREFLEX_SCENARIO_SLOW, samples);
  size_t index;
  bool caution_seen = false;

  for (index = 0; index < count; index++)
    {
      caution_seen |= samples[index].reflex.state == FLYREFLEX_STATE_CAUTION;
      CHECK(samples[index].reflex.state != FLYREFLEX_STATE_DANGER);
      CHECK(samples[index].arbitration.final_command == FLYREFLEX_CMD_FORWARD);
    }

  CHECK(caution_seen);
}

static void test_fast_looming_and_override(void)
{
  struct flyreflex_sample_s samples[FLYREFLEX_MAX_SAMPLES];
  size_t count = collect(FLYREFLEX_SCENARIO_DANGER, samples);
  size_t index;
  bool override_seen = false;

  for (index = 0; index < count; index++)
    {
      if (samples[index].arbitration.decision ==
          FLYREFLEX_DECISION_REFLEX_OVERRIDE)
        {
          override_seen = true;
          CHECK(samples[index].arbitration.ai_command ==
                FLYREFLEX_CMD_FORWARD);
          CHECK(samples[index].arbitration.final_command ==
                FLYREFLEX_CMD_ESCAPE);
        }
    }

  CHECK(override_seen);
}

static void test_threshold_boundary(void)
{
  struct flyreflex_engine_s engine;
  struct flyreflex_input_s input = {0, 0, 0};
  struct flyreflex_output_s output;

  flyreflex_engine_reset(&engine);
  engine.membrane_milli = 857;
  flyreflex_engine_step(&engine, &input, &output);
  CHECK(output.gf_membrane_milli == 299);
  CHECK(output.state == FLYREFLEX_STATE_SAFE);

  flyreflex_engine_reset(&engine);
  engine.membrane_milli = 858;
  flyreflex_engine_step(&engine, &input, &output);
  CHECK(output.gf_membrane_milli == 300);
  CHECK(output.state == FLYREFLEX_STATE_CAUTION);
}

static void test_recovery(void)
{
  struct flyreflex_sample_s samples[FLYREFLEX_MAX_SAMPLES];
  size_t count = collect(FLYREFLEX_SCENARIO_RECOVERY, samples);
  size_t index;
  bool danger_seen = false;
  bool recovered = false;

  for (index = 0; index < count; index++)
    {
      danger_seen |= samples[index].reflex.state == FLYREFLEX_STATE_DANGER;
      if (danger_seen && samples[index].reflex.state == FLYREFLEX_STATE_SAFE)
        {
          recovered = true;
        }
    }

  CHECK(danger_seen);
  CHECK(recovered);
  CHECK(samples[count - 1].arbitration.final_command == FLYREFLEX_CMD_FORWARD);
}

static void test_decay_and_determinism(void)
{
  struct flyreflex_engine_s first;
  struct flyreflex_engine_s second;
  struct flyreflex_input_s input = {600, 700, 0};
  struct flyreflex_input_s zero = {0, 0, 1};
  struct flyreflex_output_s out_first;
  struct flyreflex_output_s out_second;
  int32_t charged;

  flyreflex_engine_reset(&first);
  flyreflex_engine_reset(&second);
  flyreflex_engine_step(&first, &input, &out_first);
  flyreflex_engine_step(&second, &input, &out_second);
  CHECK(memcmp(&out_first, &out_second, sizeof(out_first)) == 0);
  charged = out_first.gf_membrane_milli;
  flyreflex_engine_step(&first, &zero, &out_first);
  CHECK(out_first.gf_membrane_milli < charged);
}

static void test_invalid_input_failsafe(void)
{
  struct flyreflex_engine_s engine;
  struct flyreflex_input_s input = {1001, 0, 0};
  struct flyreflex_output_s output;
  struct flyreflex_arbitration_s arbitration;

  flyreflex_engine_reset(&engine);
  flyreflex_engine_step(&engine, &input, &output);
  flyreflex_arbitrate(FLYREFLEX_CMD_FORWARD, &output, &arbitration);
  CHECK(!output.input_valid);
  CHECK(output.command == FLYREFLEX_CMD_STOP);
  CHECK(arbitration.final_command == FLYREFLEX_CMD_STOP);
  CHECK(arbitration.decision == FLYREFLEX_DECISION_FAILSAFE);
}

static void test_noise_rejection(void)
{
  struct flyreflex_sample_s samples[FLYREFLEX_MAX_SAMPLES];
  size_t count = collect(FLYREFLEX_SCENARIO_NOISE, samples);
  size_t index;

  for (index = 0; index < count; index++)
    {
      CHECK(samples[index].arbitration.decision !=
            FLYREFLEX_DECISION_REFLEX_OVERRIDE);
    }
}

int main(void)
{
  test_safe_input();
  test_slow_approach();
  test_fast_looming_and_override();
  test_threshold_boundary();
  test_recovery();
  test_decay_and_determinism();
  test_invalid_input_failsafe();
  test_noise_rejection();

  printf("FlyReflex tests: %u checks, %u failures\n", g_tests, g_failures);
  return g_failures == 0 ? 0 : 1;
}

