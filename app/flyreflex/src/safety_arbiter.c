/* SPDX-License-Identifier: Apache-2.0 */

#include "flyreflex.h"

static bool command_valid(enum flyreflex_command_e command)
{
  return command >= FLYREFLEX_CMD_IDLE && command <= FLYREFLEX_CMD_ESCAPE;
}

void flyreflex_arbitrate(enum flyreflex_command_e ai_command,
                         const struct flyreflex_output_s *reflex,
                         struct flyreflex_arbitration_s *result)
{
  if (reflex == NULL || result == NULL)
    {
      return;
    }

  result->ai_command = ai_command;
  result->reflex_command = reflex->command;

  if (!command_valid(ai_command) || !reflex->input_valid ||
      reflex->state == FLYREFLEX_STATE_INVALID)
    {
      result->final_command = FLYREFLEX_CMD_STOP;
      result->decision = FLYREFLEX_DECISION_FAILSAFE;
    }
  else if (reflex->state == FLYREFLEX_STATE_DANGER)
    {
      result->final_command = reflex->command == FLYREFLEX_CMD_ESCAPE ?
                              FLYREFLEX_CMD_ESCAPE : FLYREFLEX_CMD_STOP;
      result->decision = FLYREFLEX_DECISION_REFLEX_OVERRIDE;
    }
  else
    {
      result->final_command = ai_command;
      result->decision = FLYREFLEX_DECISION_AI;
    }
}

