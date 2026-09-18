/* SPDX-License-Identifier: Apache-2.0 */

#include "flyreflex.h"

#include <stddef.h>

/* Values are normalized angular-size proxies in [0, 1000].  They are
 * deterministic engineering stimuli, not camera measurements.
 */

static const int16_t g_safe_sizes[] =
{
  120, 120, 121, 120, 119, 120, 121, 120, 120, 119, 120, 120
};

static const int16_t g_slow_sizes[] =
{
  100, 125, 150, 180, 215, 250, 290, 330, 370, 410, 450, 490
};

static const int16_t g_danger_sizes[] =
{
  100, 115, 145, 210, 330, 510, 740, 960
};

static const int16_t g_recovery_sizes[] =
{
  100, 120, 165, 260, 430, 680, 940, 700, 460, 280, 170, 120,
  100, 90, 90, 90
};

static const int16_t g_noise_sizes[] =
{
  120, 121, 119, 120, 330, 121, 120, 119, 120, 121, 120, 120
};

static const struct flyreflex_scenario_data_s g_scenarios[] =
{
  {FLYREFLEX_SCENARIO_SAFE, "safe", g_safe_sizes,
   sizeof(g_safe_sizes) / sizeof(g_safe_sizes[0])},
  {FLYREFLEX_SCENARIO_SLOW, "slow", g_slow_sizes,
   sizeof(g_slow_sizes) / sizeof(g_slow_sizes[0])},
  {FLYREFLEX_SCENARIO_DANGER, "danger", g_danger_sizes,
   sizeof(g_danger_sizes) / sizeof(g_danger_sizes[0])},
  {FLYREFLEX_SCENARIO_RECOVERY, "recovery", g_recovery_sizes,
   sizeof(g_recovery_sizes) / sizeof(g_recovery_sizes[0])},
  {FLYREFLEX_SCENARIO_NOISE, "noise", g_noise_sizes,
   sizeof(g_noise_sizes) / sizeof(g_noise_sizes[0])}
};

const struct flyreflex_scenario_data_s *
flyreflex_get_scenario(enum flyreflex_scenario_e scenario)
{
  size_t index;

  for (index = 0; index < sizeof(g_scenarios) / sizeof(g_scenarios[0]); index++)
    {
      if (g_scenarios[index].kind == scenario)
        {
          return &g_scenarios[index];
        }
    }

  return NULL;
}

void flyreflex_make_input(const struct flyreflex_scenario_data_s *scenario,
                          size_t index, struct flyreflex_input_s *input)
{
  int32_t delta;
  int32_t rate;

  if (scenario == NULL || input == NULL || index >= scenario->count)
    {
      return;
    }

  input->angular_size_milli = scenario->sizes[index];
  input->frame = (uint32_t)index;

  if (index == 0)
    {
      input->angular_rate_milli = 0;
      return;
    }

  delta = (int32_t)scenario->sizes[index] - scenario->sizes[index - 1];
  rate = delta > 0 ? delta * 4 : 0;
  input->angular_rate_milli = rate > FLYREFLEX_SCALE ? FLYREFLEX_SCALE : rate;
}

