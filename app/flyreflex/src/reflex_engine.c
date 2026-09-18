/* SPDX-License-Identifier: Apache-2.0 */

#include "flyreflex.h"

#include <limits.h>

/* MaleCNS v1.0 aggregate synapse counts queried 2026-09-18:
 *   LPLC2 -> DNp01: 4862
 *   LC4   -> DNp01: 6362
 * The integer weights below are the transparent normalization
 * count / (4862 + 6362), rounded to a 1000-point scale.
 * The leak, thresholds, activity encoders, clamp and command mapping are
 * ENGINEERING_PARAMETER values, not measured fruit-fly dynamics.
 */

#define LPLC2_WEIGHT_MILLI 433
#define LC4_WEIGHT_MILLI   567
#define LEAK_MILLI         350
#define CAUTION_THRESHOLD  300
#define DANGER_THRESHOLD   850
#define MEMBRANE_MAX       1200

static int32_t clamp_milli(int32_t value)
{
  if (value < 0)
    {
      return 0;
    }

  if (value > FLYREFLEX_SCALE)
    {
      return FLYREFLEX_SCALE;
    }

  return value;
}

void flyreflex_engine_reset(struct flyreflex_engine_s *engine)
{
  if (engine == NULL)
    {
      return;
    }

  engine->membrane_milli = 0;
  engine->state = FLYREFLEX_STATE_SAFE;
  engine->danger_events = 0;
}

void flyreflex_engine_step(struct flyreflex_engine_s *engine,
                           const struct flyreflex_input_s *input,
                           struct flyreflex_output_s *output)
{
  int32_t drive;
  int32_t membrane;

  if (engine == NULL || input == NULL || output == NULL)
    {
      return;
    }

  output->input_valid = input->angular_size_milli >= 0 &&
                        input->angular_size_milli <= FLYREFLEX_SCALE &&
                        input->angular_rate_milli >= 0 &&
                        input->angular_rate_milli <= FLYREFLEX_SCALE;

  if (!output->input_valid)
    {
      output->lplc2_activity_milli = 0;
      output->lc4_activity_milli = 0;
      output->gf_membrane_milli = engine->membrane_milli;
      output->state = FLYREFLEX_STATE_INVALID;
      output->command = FLYREFLEX_CMD_STOP;
      engine->state = FLYREFLEX_STATE_INVALID;
      return;
    }

  /* Connectome-inspired engineering encoder: angular-size proxy maps to
   * LPLC2 activity and expansion-rate proxy maps to LC4 activity.
   */

  output->lplc2_activity_milli = clamp_milli(input->angular_size_milli);
  output->lc4_activity_milli = clamp_milli(input->angular_rate_milli);

  drive = (output->lplc2_activity_milli * LPLC2_WEIGHT_MILLI +
           output->lc4_activity_milli * LC4_WEIGHT_MILLI) /
          FLYREFLEX_SCALE;
  membrane = (engine->membrane_milli * LEAK_MILLI) / FLYREFLEX_SCALE + drive;

  if (membrane > MEMBRANE_MAX)
    {
      membrane = MEMBRANE_MAX;
    }

  engine->membrane_milli = membrane;

  if (membrane >= DANGER_THRESHOLD)
    {
      engine->state = FLYREFLEX_STATE_DANGER;
      engine->danger_events++;
      output->command = FLYREFLEX_CMD_ESCAPE;
    }
  else if (membrane >= CAUTION_THRESHOLD)
    {
      engine->state = FLYREFLEX_STATE_CAUTION;
      output->command = FLYREFLEX_CMD_IDLE;
    }
  else
    {
      engine->state = FLYREFLEX_STATE_SAFE;
      output->command = FLYREFLEX_CMD_IDLE;
    }

  output->gf_membrane_milli = membrane;
  output->state = engine->state;
}

