/* SPDX-License-Identifier: Apache-2.0 */

#include "flyreflex.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint64_t monotonic_ns(void)
{
  struct timespec timestamp;

  if (clock_gettime(CLOCK_MONOTONIC, &timestamp) != 0)
    {
      return 0;
    }

  return (uint64_t)timestamp.tv_sec * UINT64_C(1000000000) +
         (uint64_t)timestamp.tv_nsec;
}

static int compare_u64(const void *left, const void *right)
{
  uint64_t a = *(const uint64_t *)left;
  uint64_t b = *(const uint64_t *)right;
  return (a > b) - (a < b);
}

static void calculate_stats(uint64_t *values, size_t count,
                            struct flyreflex_stats_s *stats)
{
  uint64_t sum = 0;
  size_t index;

  qsort(values, count, sizeof(values[0]), compare_u64);
  for (index = 0; index < count; index++)
    {
      sum += values[index];
    }

  stats->minimum_ns = values[0];
  stats->mean_ns = sum / count;
  stats->median_ns = values[(count - 1) / 2];
  stats->p95_ns = values[((count - 1) * 95) / 100];
  stats->p99_ns = values[((count - 1) * 99) / 100];
  stats->maximum_ns = values[count - 1];
}

const char *flyreflex_command_name(enum flyreflex_command_e command)
{
  static const char *const names[] =
  {
    "IDLE", "FORWARD", "LEFT", "RIGHT", "STOP", "ESCAPE"
  };

  if (command < FLYREFLEX_CMD_IDLE || command > FLYREFLEX_CMD_ESCAPE)
    {
      return "INVALID";
    }

  return names[command];
}

const char *flyreflex_state_name(enum flyreflex_state_e state)
{
  static const char *const names[] =
  {
    "SAFE", "CAUTION", "DANGER", "INVALID"
  };

  if (state < FLYREFLEX_STATE_SAFE || state > FLYREFLEX_STATE_INVALID)
    {
      return "INVALID";
    }

  return names[state];
}

const char *flyreflex_decision_name(enum flyreflex_decision_e decision)
{
  static const char *const names[] =
  {
    "AI", "REFLEX_OVERRIDE", "FAILSAFE"
  };

  if (decision < FLYREFLEX_DECISION_AI ||
      decision > FLYREFLEX_DECISION_FAILSAFE)
    {
      return "INVALID";
    }

  return names[decision];
}

void flyreflex_print_sample(const char *scenario,
                            const struct flyreflex_sample_s *sample, bool csv,
                            bool print_header)
{
  uint64_t reflex_ns;
  uint64_t end_to_end_ns;

  if (sample == NULL)
    {
      return;
    }

  reflex_ns = sample->latency.reflex_end_ns - sample->latency.reflex_start_ns;
  end_to_end_ns = sample->latency.arbiter_end_ns - sample->latency.event_ns;

  if (csv)
    {
      if (print_header)
        {
          printf("scenario,frame,size_milli,rate_milli,lplc2_milli,lc4_milli,"
                 "gf_milli,state,ai_cmd,reflex_cmd,final_cmd,decision,"
                 "reflex_ns,end_to_end_ns\n");
        }

      printf("%s,%" PRIu32 ",%" PRId32 ",%" PRId32 ",%" PRId32
             ",%" PRId32 ",%" PRId32 ",%s,%s,%s,%s,%s,%" PRIu64
             ",%" PRIu64 "\n",
             scenario, sample->input.frame, sample->input.angular_size_milli,
             sample->input.angular_rate_milli,
             sample->reflex.lplc2_activity_milli,
             sample->reflex.lc4_activity_milli,
             sample->reflex.gf_membrane_milli,
             flyreflex_state_name(sample->reflex.state),
             flyreflex_command_name(sample->arbitration.ai_command),
             flyreflex_command_name(sample->arbitration.reflex_command),
             flyreflex_command_name(sample->arbitration.final_command),
             flyreflex_decision_name(sample->arbitration.decision), reflex_ns,
             end_to_end_ns);
      return;
    }

  printf("[FlyReflex] scenario=%s frame=%" PRIu32
         " size=%" PRId32 " rate=%" PRId32
         " LPLC2=%" PRId32 " LC4=%" PRId32 " GF=%" PRId32
         " state=%s ai=%s reflex=%s final=%s decision=%s"
         " reflex_ns=%" PRIu64 " end_to_end_ns=%" PRIu64 "\n",
         scenario, sample->input.frame, sample->input.angular_size_milli,
         sample->input.angular_rate_milli,
         sample->reflex.lplc2_activity_milli,
         sample->reflex.lc4_activity_milli,
         sample->reflex.gf_membrane_milli,
         flyreflex_state_name(sample->reflex.state),
         flyreflex_command_name(sample->arbitration.ai_command),
         flyreflex_command_name(sample->arbitration.reflex_command),
         flyreflex_command_name(sample->arbitration.final_command),
         flyreflex_decision_name(sample->arbitration.decision), reflex_ns,
         end_to_end_ns);
}

int flyreflex_run_scenario(enum flyreflex_scenario_e scenario, bool csv,
                           struct flyreflex_sample_s *samples,
                           size_t sample_capacity, size_t *sample_count)
{
  const struct flyreflex_scenario_data_s *data;
  struct flyreflex_engine_s engine;
  struct flyreflex_sample_s sample;
  size_t index;

  data = flyreflex_get_scenario(scenario);
  if (data == NULL)
    {
      return -1;
    }

  flyreflex_engine_reset(&engine);
  for (index = 0; index < data->count; index++)
    {
      memset(&sample, 0, sizeof(sample));
      flyreflex_make_input(data, index, &sample.input);
      sample.latency.event_ns = monotonic_ns();
      sample.latency.reflex_start_ns = monotonic_ns();
      flyreflex_engine_step(&engine, &sample.input, &sample.reflex);
      sample.latency.reflex_end_ns = monotonic_ns();
      flyreflex_arbitrate(FLYREFLEX_CMD_FORWARD, &sample.reflex,
                          &sample.arbitration);
      sample.latency.arbiter_end_ns = monotonic_ns();

      if (samples != NULL && index < sample_capacity)
        {
          samples[index] = sample;
        }

      flyreflex_print_sample(data->name, &sample, csv, csv && index == 0);
    }

  if (sample_count != NULL)
    {
      *sample_count = data->count;
    }

  return 0;
}

int flyreflex_run_benchmark(size_t iterations,
                            struct flyreflex_stats_s *reflex_stats,
                            struct flyreflex_stats_s *end_to_end_stats)
{
  struct flyreflex_input_s input = {960, 1000, 0};
  struct flyreflex_engine_s engine;
  struct flyreflex_output_s output;
  struct flyreflex_arbitration_s arbitration;
  uint64_t *reflex_values;
  uint64_t *end_to_end_values;
  uint64_t start;
  uint64_t reflex_end;
  uint64_t arbiter_end;
  size_t index;

  if (iterations == 0 || reflex_stats == NULL || end_to_end_stats == NULL)
    {
      return -1;
    }

  reflex_values = (uint64_t *)malloc(iterations * sizeof(uint64_t));
  end_to_end_values = (uint64_t *)malloc(iterations * sizeof(uint64_t));
  if (reflex_values == NULL || end_to_end_values == NULL)
    {
      free(reflex_values);
      free(end_to_end_values);
      return -1;
    }

  for (index = 0; index < iterations; index++)
    {
      flyreflex_engine_reset(&engine);
      start = monotonic_ns();
      flyreflex_engine_step(&engine, &input, &output);
      reflex_end = monotonic_ns();
      flyreflex_arbitrate(FLYREFLEX_CMD_FORWARD, &output, &arbitration);
      arbiter_end = monotonic_ns();
      reflex_values[index] = reflex_end - start;
      end_to_end_values[index] = arbiter_end - start;
    }

  calculate_stats(reflex_values, iterations, reflex_stats);
  calculate_stats(end_to_end_values, iterations, end_to_end_stats);
  free(reflex_values);
  free(end_to_end_values);
  return 0;
}
