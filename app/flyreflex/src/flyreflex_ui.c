/* SPDX-License-Identifier: Apache-2.0 */

#include <nuttx/config.h>

#include "flyreflex.h"

#include <inttypes.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <lvgl/lvgl.h>

struct flyreflex_ui_s
{
  lv_obj_t *root;
  lv_obj_t *scenario_label;
  lv_obj_t *risk_bar;
  lv_obj_t *activity_label;
  lv_obj_t *ai_label;
  lv_obj_t *reflex_label;
  lv_obj_t *final_label;
  lv_obj_t *decision_label;
  lv_obj_t *latency_label;
  lv_obj_t *state_label;
  lv_timer_t *timer;
  struct flyreflex_engine_s engine;
  enum flyreflex_scenario_e scenario;
  size_t frame;
};

static struct flyreflex_ui_s g_ui;

static uint64_t ui_monotonic_ns(void)
{
  struct timespec timestamp;

  if (clock_gettime(CLOCK_MONOTONIC, &timestamp) != 0)
    {
      return 0;
    }

  return (uint64_t)timestamp.tv_sec * UINT64_C(1000000000) +
         (uint64_t)timestamp.tv_nsec;
}

static lv_color_t state_color(enum flyreflex_state_e state)
{
  if (state == FLYREFLEX_STATE_DANGER ||
      state == FLYREFLEX_STATE_INVALID)
    {
      return lv_color_hex(0xe53935);
    }

  if (state == FLYREFLEX_STATE_CAUTION)
    {
      return lv_color_hex(0xf9a825);
    }

  return lv_color_hex(0x2e7d32);
}

static void scenario_advance(void)
{
  g_ui.scenario = (enum flyreflex_scenario_e)(g_ui.scenario + 1);
  if (g_ui.scenario > FLYREFLEX_SCENARIO_RECOVERY)
    {
      g_ui.scenario = FLYREFLEX_SCENARIO_SAFE;
    }

  g_ui.frame = 0;
  flyreflex_engine_reset(&g_ui.engine);
}

static void ui_timer_callback(lv_timer_t *timer)
{
  const struct flyreflex_scenario_data_s *scenario;
  struct flyreflex_input_s input;
  struct flyreflex_output_s reflex;
  struct flyreflex_arbitration_s arbitration;
  uint64_t start;
  uint64_t end;
  lv_color_t color;

  (void)timer;
  scenario = flyreflex_get_scenario(g_ui.scenario);
  if (scenario == NULL)
    {
      return;
    }

  if (g_ui.frame >= scenario->count)
    {
      scenario_advance();
      scenario = flyreflex_get_scenario(g_ui.scenario);
    }

  flyreflex_make_input(scenario, g_ui.frame, &input);
  start = ui_monotonic_ns();
  flyreflex_engine_step(&g_ui.engine, &input, &reflex);
  flyreflex_arbitrate(FLYREFLEX_CMD_FORWARD, &reflex, &arbitration);
  end = ui_monotonic_ns();

  color = state_color(reflex.state);
  lv_label_set_text_fmt(g_ui.scenario_label, "SCENARIO: %s  FRAME: %u",
                        scenario->name, (unsigned int)g_ui.frame);
  lv_bar_set_value(g_ui.risk_bar, reflex.gf_membrane_milli, LV_ANIM_ON);
  lv_obj_set_style_bg_color(g_ui.risk_bar, color, LV_PART_INDICATOR);
  lv_label_set_text_fmt(g_ui.activity_label,
                        "Loom size %d   rate %d\n"
                        "LPLC2(size) %d   LC4(velocity) %d\n"
                        "DNp01 / GF membrane %d",
                        input.angular_size_milli, input.angular_rate_milli,
                        reflex.lplc2_activity_milli,
                        reflex.lc4_activity_milli,
                        reflex.gf_membrane_milli);
  lv_label_set_text(g_ui.ai_label, "AI COMMAND: FORWARD (SIMULATED)");
  lv_label_set_text_fmt(g_ui.reflex_label, "REFLEX: %s",
                        flyreflex_command_name(reflex.command));
  lv_label_set_text_fmt(g_ui.final_label, "FINAL: %s",
                        flyreflex_command_name(arbitration.final_command));
  lv_label_set_text_fmt(g_ui.decision_label, "DECISION: %s",
                        flyreflex_decision_name(arbitration.decision));
  lv_label_set_text_fmt(g_ui.latency_label, "LOCAL END-TO-END: %" PRIu64 " ns",
                        end - start);
  lv_label_set_text_fmt(g_ui.state_label, "STATE: %s",
                        flyreflex_state_name(reflex.state));
  lv_obj_set_style_text_color(g_ui.state_label, color, 0);
  lv_obj_set_style_text_color(g_ui.final_label, color, 0);
  lv_obj_set_style_text_color(g_ui.decision_label, color, 0);

  printf("[FlyReflex UI] scenario=%s frame=%u state=%s ai=FORWARD "
         "reflex=%s final=%s decision=%s end_to_end_ns=%" PRIu64 "\n",
         scenario->name, (unsigned int)g_ui.frame,
         flyreflex_state_name(reflex.state),
         flyreflex_command_name(reflex.command),
         flyreflex_command_name(arbitration.final_command),
         flyreflex_decision_name(arbitration.decision), end - start);

  g_ui.frame++;
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text, int32_t size)
{
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_set_width(label, LV_PCT(100));
  lv_obj_set_style_text_font(label,
                             size >= 24 ? &lv_font_montserrat_24 :
                             &lv_font_montserrat_16, 0);
  return label;
}

static void create_dashboard(void)
{
  lv_obj_t *title;

  g_ui.root = lv_obj_create(lv_screen_active());
  lv_obj_set_size(g_ui.root, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(g_ui.root, lv_color_hex(0x101820), 0);
  lv_obj_set_style_text_color(g_ui.root, lv_color_hex(0xf5f7fa), 0);
  lv_obj_set_style_pad_all(g_ui.root, 20, 0);
  lv_obj_set_style_pad_row(g_ui.root, 10, 0);
  lv_obj_set_flex_flow(g_ui.root, LV_FLEX_FLOW_COLUMN);
  lv_obj_remove_flag(g_ui.root, LV_OBJ_FLAG_SCROLLABLE);

  title = make_label(g_ui.root, "FlyReflex", 24);
  lv_obj_set_style_text_color(title, lv_color_hex(0x4fc3f7), 0);
  g_ui.scenario_label = make_label(g_ui.root, "SCENARIO", 16);

  g_ui.risk_bar = lv_bar_create(g_ui.root);
  lv_obj_set_width(g_ui.risk_bar, LV_PCT(100));
  lv_obj_set_height(g_ui.risk_bar, 24);
  lv_bar_set_range(g_ui.risk_bar, 0, 1200);

  g_ui.activity_label = make_label(g_ui.root, "Activity", 16);
  g_ui.state_label = make_label(g_ui.root, "STATE: SAFE", 24);
  g_ui.ai_label = make_label(g_ui.root, "AI COMMAND: FORWARD (SIMULATED)", 16);
  g_ui.reflex_label = make_label(g_ui.root, "REFLEX: IDLE", 16);
  g_ui.final_label = make_label(g_ui.root, "FINAL: FORWARD", 24);
  g_ui.decision_label = make_label(g_ui.root, "DECISION: AI", 24);
  g_ui.latency_label = make_label(g_ui.root, "LOCAL END-TO-END: -- ns", 16);

  flyreflex_engine_reset(&g_ui.engine);
  g_ui.scenario = FLYREFLEX_SCENARIO_SAFE;
  g_ui.frame = 0;
  g_ui.timer = lv_timer_create(ui_timer_callback, 350, NULL);
  ui_timer_callback(g_ui.timer);
}

int flyreflex_ui_run(void)
{
  lv_nuttx_dsc_t descriptor;
  lv_nuttx_result_t result;

  if (lv_is_initialized())
    {
      LV_LOG_ERROR("LVGL is already initialized");
      return 1;
    }

  lv_init();
  lv_nuttx_dsc_init(&descriptor);
#ifdef CONFIG_LV_USE_NUTTX_LCD
  descriptor.fb_path = "/dev/lcd0";
#endif
  lv_nuttx_init(&descriptor, &result);
  if (result.disp == NULL)
    {
      LV_LOG_ERROR("FlyReflex display initialization failed");
      return 1;
    }

  create_dashboard();
  while (1)
    {
      uint32_t idle = lv_timer_handler();
      usleep((idle ? idle : 1) * 1000);
    }

  return 0;
}

