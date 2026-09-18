/* SPDX-License-Identifier: Apache-2.0 */

#include <nuttx/config.h>

#include "flyreflex.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <lvgl/lvgl.h>

#define UI_FRAME_PERIOD_MS 500
#define UI_CARD_BG         0x17212b
#define UI_PANEL_BG        0x1e2b36
#define UI_BORDER          0x334b5c
#define UI_TEXT            0xf5f7fa
#define UI_MUTED           0x9fb3c8
#define UI_BLUE            0x42a5f5
#define UI_GREEN           0x2e7d32
#define UI_AMBER           0xf9a825
#define UI_RED             0xe53935

struct flyreflex_ui_s
{
  lv_obj_t *root;
  lv_obj_t *scenario_buttons[4];
  lv_obj_t *status_panel;
  lv_obj_t *status_label;
  lv_obj_t *scenario_label;
  lv_obj_t *explain_label;
  lv_obj_t *risk_bar;
  lv_obj_t *risk_label;
  lv_obj_t *lplc2_bar;
  lv_obj_t *lplc2_value;
  lv_obj_t *lc4_bar;
  lv_obj_t *lc4_value;
  lv_obj_t *gf_bar;
  lv_obj_t *gf_value;
  lv_obj_t *ai_panel;
  lv_obj_t *reflex_panel;
  lv_obj_t *final_panel;
  lv_obj_t *ai_label;
  lv_obj_t *reflex_label;
  lv_obj_t *final_label;
  lv_obj_t *decision_label;
  lv_obj_t *latency_label;
  lv_timer_t *timer;
  struct flyreflex_engine_s engine;
  enum flyreflex_scenario_e scenario;
  enum flyreflex_scenario_e last_scenario;
  enum flyreflex_state_e last_state;
  enum flyreflex_decision_e last_decision;
  size_t frame;
  bool has_last_log;
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

static uint32_t state_color_value(enum flyreflex_state_e state)
{
  if (state == FLYREFLEX_STATE_DANGER ||
      state == FLYREFLEX_STATE_INVALID)
    {
      return UI_RED;
    }

  if (state == FLYREFLEX_STATE_CAUTION)
    {
      return UI_AMBER;
    }

  return UI_GREEN;
}

static const char *scenario_caption(enum flyreflex_scenario_e scenario)
{
  switch (scenario)
    {
      case FLYREFLEX_SCENARIO_SAFE:
        return "No threat. AI keeps control.";
      case FLYREFLEX_SCENARIO_SLOW:
        return "Slow approach. Monitor without a false stop.";
      case FLYREFLEX_SCENARIO_DANGER:
        return "Fast collision threat. Reflex must override AI.";
      case FLYREFLEX_SCENARIO_RECOVERY:
        return "Threat clears. Control returns to AI.";
      default:
        return "Input state unavailable.";
    }
}

static const char *state_headline(enum flyreflex_state_e state)
{
  switch (state)
    {
      case FLYREFLEX_STATE_SAFE:
        return "SAFE - AI CONTROL";
      case FLYREFLEX_STATE_CAUTION:
        return "CAUTION - MONITORING";
      case FLYREFLEX_STATE_DANGER:
        return "DANGER - REFLEX OVERRIDE";
      default:
        return "FAILSAFE - STOP";
    }
}

static void set_panel_style(lv_obj_t *panel, uint32_t background)
{
  lv_obj_set_style_bg_color(panel, lv_color_hex(background), 0);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(panel, lv_color_hex(UI_BORDER), 0);
  lv_obj_set_style_border_width(panel, 1, 0);
  lv_obj_set_style_radius(panel, 12, 0);
  lv_obj_set_style_pad_all(panel, 12, 0);
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text,
                            const lv_font_t *font, uint32_t color,
                            lv_text_align_t alignment)
{
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(label, LV_PCT(100));
  lv_obj_set_style_text_font(label, font, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
  lv_obj_set_style_text_align(label, alignment, 0);
  return label;
}

static lv_obj_t *make_activity_card(lv_obj_t *parent, const char *title,
                                    const char *subtitle, int32_t maximum,
                                    lv_obj_t **bar, lv_obj_t **value)
{
  lv_obj_t *card = lv_obj_create(parent);

  lv_obj_set_width(card, LV_PCT(32));
  lv_obj_set_height(card, LV_PCT(100));
  set_panel_style(card, UI_CARD_BG);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(card, 7, 0);

  make_label(card, title, &lv_font_montserrat_20, UI_TEXT,
             LV_TEXT_ALIGN_CENTER);
  make_label(card, subtitle, LV_FONT_DEFAULT, UI_MUTED,
             LV_TEXT_ALIGN_CENTER);

  *bar = lv_bar_create(card);
  lv_obj_set_width(*bar, LV_PCT(100));
  lv_obj_set_height(*bar, 18);
  lv_bar_set_range(*bar, 0, maximum);
  lv_obj_set_style_bg_color(*bar, lv_color_hex(UI_BLUE), LV_PART_INDICATOR);

  *value = make_label(card, "0", &lv_font_montserrat_20, UI_BLUE,
                      LV_TEXT_ALIGN_CENTER);
  return card;
}

static lv_obj_t *make_command_card(lv_obj_t *parent, const char *heading,
                                   lv_obj_t **value)
{
  lv_obj_t *card = lv_obj_create(parent);

  lv_obj_set_width(card, LV_PCT(31));
  lv_obj_set_height(card, LV_PCT(100));
  set_panel_style(card, UI_CARD_BG);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(card, 8, 0);

  make_label(card, heading, LV_FONT_DEFAULT, UI_MUTED, LV_TEXT_ALIGN_CENTER);
  *value = make_label(card, "--", &lv_font_montserrat_20, UI_TEXT,
                      LV_TEXT_ALIGN_CENTER);
  return card;
}

static void update_scenario_buttons(void)
{
  size_t index;

  for (index = 0; index < 4; index++)
    {
      bool selected = (enum flyreflex_scenario_e)index == g_ui.scenario;
      lv_obj_set_style_bg_color(g_ui.scenario_buttons[index],
                                lv_color_hex(selected ? UI_BLUE : UI_CARD_BG),
                                0);
      lv_obj_set_style_border_color(g_ui.scenario_buttons[index],
                                    lv_color_hex(selected ? UI_BLUE :
                                                         UI_BORDER),
                                    0);
    }
}

static void select_scenario(enum flyreflex_scenario_e scenario)
{
  g_ui.scenario = scenario;
  g_ui.frame = 0;
  g_ui.has_last_log = false;
  flyreflex_engine_reset(&g_ui.engine);
  update_scenario_buttons();
}

static void scenario_button_callback(lv_event_t *event)
{
  enum flyreflex_scenario_e scenario =
    (enum flyreflex_scenario_e)(uintptr_t)lv_event_get_user_data(event);

  select_scenario(scenario);
}

static void scenario_advance(void)
{
  enum flyreflex_scenario_e next =
    (enum flyreflex_scenario_e)(g_ui.scenario + 1);

  if (next > FLYREFLEX_SCENARIO_RECOVERY)
    {
      next = FLYREFLEX_SCENARIO_SAFE;
    }

  select_scenario(next);
}

static void ui_timer_callback(lv_timer_t *timer)
{
  const struct flyreflex_scenario_data_s *scenario;
  struct flyreflex_input_s input;
  struct flyreflex_output_s reflex;
  struct flyreflex_arbitration_s arbitration;
  uint64_t start;
  uint64_t end;
  uint64_t latency_ns;
  uint64_t latency_us;
  uint32_t color_value;
  int32_t risk_percent;

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

  latency_ns = end - start;
  latency_us = (latency_ns + 999) / 1000;
  color_value = state_color_value(reflex.state);
  risk_percent = reflex.gf_membrane_milli * 100 / 1200;

  lv_label_set_text(g_ui.status_label, state_headline(reflex.state));
  lv_obj_set_style_bg_color(g_ui.status_panel, lv_color_hex(color_value), 0);
  lv_label_set_text_fmt(g_ui.scenario_label, "SCENE: %s   FRAME: %u",
                        scenario->name, (unsigned int)g_ui.frame);
  lv_label_set_text(g_ui.explain_label, scenario_caption(g_ui.scenario));

  lv_bar_set_value(g_ui.risk_bar, reflex.gf_membrane_milli, LV_ANIM_ON);
  lv_obj_set_style_bg_color(g_ui.risk_bar, lv_color_hex(color_value),
                            LV_PART_INDICATOR);
  lv_label_set_text_fmt(g_ui.risk_label,
                        "COLLISION RISK  %d%%   GF activity %d / 1200",
                        risk_percent, reflex.gf_membrane_milli);

  lv_bar_set_value(g_ui.lplc2_bar, reflex.lplc2_activity_milli, LV_ANIM_ON);
  lv_label_set_text_fmt(g_ui.lplc2_value, "%d / 1000",
                        reflex.lplc2_activity_milli);
  lv_bar_set_value(g_ui.lc4_bar, reflex.lc4_activity_milli, LV_ANIM_ON);
  lv_label_set_text_fmt(g_ui.lc4_value, "%d / 1000",
                        reflex.lc4_activity_milli);
  lv_bar_set_value(g_ui.gf_bar, reflex.gf_membrane_milli, LV_ANIM_ON);
  lv_obj_set_style_bg_color(g_ui.gf_bar, lv_color_hex(color_value),
                            LV_PART_INDICATOR);
  lv_label_set_text_fmt(g_ui.gf_value, "%d / 1200",
                        reflex.gf_membrane_milli);
  lv_obj_set_style_text_color(g_ui.gf_value, lv_color_hex(color_value), 0);

  lv_label_set_text(g_ui.ai_label, "FORWARD");
  lv_label_set_text(g_ui.reflex_label,
                    flyreflex_command_name(reflex.command));
  lv_label_set_text(g_ui.final_label,
                    flyreflex_command_name(arbitration.final_command));
  lv_obj_set_style_bg_color(g_ui.final_panel, lv_color_hex(color_value), 0);
  lv_label_set_text_fmt(g_ui.decision_label,
                        "AI FORWARD  +  REFLEX %s  =>  FINAL %s    [%s]",
                        flyreflex_command_name(reflex.command),
                        flyreflex_command_name(arbitration.final_command),
                        flyreflex_decision_name(arbitration.decision));
  lv_obj_set_style_text_color(g_ui.decision_label, lv_color_hex(color_value),
                              0);
  lv_label_set_text_fmt(g_ui.latency_label,
                        "Measured local reflex + arbiter latency: %" PRIu64
                        " us  (%" PRIu64 " ns)",
                        latency_us, latency_ns);

  update_scenario_buttons();

  if (!g_ui.has_last_log || g_ui.last_scenario != g_ui.scenario ||
      g_ui.last_state != reflex.state ||
      g_ui.last_decision != arbitration.decision)
    {
      printf("[FlyReflex] scene=%s state=%s | AI=FORWARD | REFLEX=%s | "
             "FINAL=%s | %s | latency=%" PRIu64 " us\n",
             scenario->name, flyreflex_state_name(reflex.state),
             flyreflex_command_name(reflex.command),
             flyreflex_command_name(arbitration.final_command),
             flyreflex_decision_name(arbitration.decision), latency_us);
      g_ui.last_scenario = g_ui.scenario;
      g_ui.last_state = reflex.state;
      g_ui.last_decision = arbitration.decision;
      g_ui.has_last_log = true;
    }

  g_ui.frame++;
}

static void create_scenario_buttons(lv_obj_t *parent)
{
  static const char *const names[] =
  {
    "SAFE", "SLOW", "DANGER", "RECOVERY"
  };
  size_t index;

  for (index = 0; index < 4; index++)
    {
      lv_obj_t *button = lv_button_create(parent);
      lv_obj_t *label;

      lv_obj_set_width(button, LV_PCT(24));
      lv_obj_set_height(button, 42);
      lv_obj_set_style_radius(button, 9, 0);
      lv_obj_set_style_border_width(button, 1, 0);
      lv_obj_add_event_cb(button, scenario_button_callback, LV_EVENT_CLICKED,
                          (void *)(uintptr_t)index);
      label = lv_label_create(button);
      lv_label_set_text(label, names[index]);
      lv_obj_center(label);
      g_ui.scenario_buttons[index] = button;
    }
}

static void create_dashboard(void)
{
  lv_obj_t *header;
  lv_obj_t *title_label;
  lv_obj_t *subtitle_label;
  lv_obj_t *button_row;
  lv_obj_t *status_text;
  lv_obj_t *activity_row;
  lv_obj_t *command_row;

  g_ui.root = lv_obj_create(lv_screen_active());
  lv_obj_set_size(g_ui.root, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(g_ui.root, lv_color_hex(0x0d141b), 0);
  lv_obj_set_style_bg_opa(g_ui.root, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(g_ui.root, lv_color_hex(UI_TEXT), 0);
  lv_obj_set_style_border_width(g_ui.root, 0, 0);
  lv_obj_set_style_radius(g_ui.root, 0, 0);
  lv_obj_set_style_pad_all(g_ui.root, 18, 0);
  lv_obj_set_style_pad_row(g_ui.root, 9, 0);
  lv_obj_set_flex_flow(g_ui.root, LV_FLEX_FLOW_COLUMN);
  lv_obj_remove_flag(g_ui.root, LV_OBJ_FLAG_SCROLLABLE);

  header = lv_obj_create(g_ui.root);
  lv_obj_set_width(header, LV_PCT(100));
  lv_obj_set_height(header, 58);
  set_panel_style(header, UI_PANEL_BG);
  lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  title_label = make_label(header, "FlyReflex", &lv_font_montserrat_20,
                           UI_BLUE, LV_TEXT_ALIGN_LEFT);
  lv_obj_set_width(title_label, LV_PCT(40));
  subtitle_label = make_label(header, "BIO-INSPIRED LOCAL SAFETY LAYER",
                              LV_FONT_DEFAULT, UI_MUTED,
                              LV_TEXT_ALIGN_RIGHT);
  lv_obj_set_width(subtitle_label, LV_PCT(58));

  button_row = lv_obj_create(g_ui.root);
  lv_obj_set_width(button_row, LV_PCT(100));
  lv_obj_set_height(button_row, 58);
  lv_obj_set_style_bg_opa(button_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(button_row, 0, 0);
  lv_obj_set_style_pad_all(button_row, 0, 0);
  lv_obj_set_flex_flow(button_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(button_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  create_scenario_buttons(button_row);

  g_ui.status_panel = lv_obj_create(g_ui.root);
  lv_obj_set_width(g_ui.status_panel, LV_PCT(100));
  lv_obj_set_height(g_ui.status_panel, 92);
  set_panel_style(g_ui.status_panel, UI_GREEN);
  lv_obj_set_flex_flow(g_ui.status_panel, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(g_ui.status_panel, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  g_ui.status_label = make_label(g_ui.status_panel, "SAFE - AI CONTROL",
                                 &lv_font_montserrat_20, UI_TEXT,
                                 LV_TEXT_ALIGN_LEFT);
  lv_obj_set_width(g_ui.status_label, LV_PCT(42));
  status_text = lv_obj_create(g_ui.status_panel);
  lv_obj_set_width(status_text, LV_PCT(55));
  lv_obj_set_height(status_text, LV_PCT(100));
  lv_obj_set_style_bg_opa(status_text, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(status_text, 0, 0);
  lv_obj_set_style_pad_all(status_text, 0, 0);
  lv_obj_set_flex_flow(status_text, LV_FLEX_FLOW_COLUMN);
  g_ui.scenario_label = make_label(status_text, "SCENE: safe",
                                   LV_FONT_DEFAULT, UI_TEXT,
                                   LV_TEXT_ALIGN_RIGHT);
  g_ui.explain_label = make_label(status_text,
                                  "No threat. AI keeps control.",
                                  LV_FONT_DEFAULT, UI_TEXT,
                                  LV_TEXT_ALIGN_RIGHT);

  g_ui.risk_label = make_label(g_ui.root, "COLLISION RISK  0%",
                               LV_FONT_DEFAULT, UI_TEXT,
                               LV_TEXT_ALIGN_LEFT);
  g_ui.risk_bar = lv_bar_create(g_ui.root);
  lv_obj_set_width(g_ui.risk_bar, LV_PCT(100));
  lv_obj_set_height(g_ui.risk_bar, 24);
  lv_bar_set_range(g_ui.risk_bar, 0, 1200);

  activity_row = lv_obj_create(g_ui.root);
  lv_obj_set_width(activity_row, LV_PCT(100));
  lv_obj_set_height(activity_row, 160);
  lv_obj_set_style_bg_opa(activity_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(activity_row, 0, 0);
  lv_obj_set_style_pad_all(activity_row, 0, 0);
  lv_obj_set_flex_flow(activity_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(activity_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  make_activity_card(activity_row, "LPLC2", "Object size channel", 1000,
                     &g_ui.lplc2_bar, &g_ui.lplc2_value);
  make_activity_card(activity_row, "LC4", "Expansion speed channel", 1000,
                     &g_ui.lc4_bar, &g_ui.lc4_value);
  make_activity_card(activity_row, "DNp01 / GF", "Escape decision node", 1200,
                     &g_ui.gf_bar, &g_ui.gf_value);

  command_row = lv_obj_create(g_ui.root);
  lv_obj_set_width(command_row, LV_PCT(100));
  lv_obj_set_height(command_row, 132);
  lv_obj_set_style_bg_opa(command_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(command_row, 0, 0);
  lv_obj_set_style_pad_all(command_row, 0, 0);
  lv_obj_set_flex_flow(command_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(command_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  g_ui.ai_panel = make_command_card(command_row, "AI WANTS",
                                    &g_ui.ai_label);
  g_ui.reflex_panel = make_command_card(command_row, "LOCAL REFLEX",
                                        &g_ui.reflex_label);
  g_ui.final_panel = make_command_card(command_row, "ROBOT EXECUTES",
                                       &g_ui.final_label);
  lv_obj_set_style_bg_color(g_ui.ai_panel, lv_color_hex(0x1565c0), 0);

  g_ui.decision_label = make_label(g_ui.root,
                                   "AI FORWARD + REFLEX IDLE => FINAL FORWARD",
                                   &lv_font_montserrat_20, UI_GREEN,
                                   LV_TEXT_ALIGN_CENTER);
  g_ui.latency_label = make_label(g_ui.root,
                                  "Measured local reflex + arbiter latency: --",
                                  LV_FONT_DEFAULT, UI_MUTED,
                                  LV_TEXT_ALIGN_CENTER);

  flyreflex_engine_reset(&g_ui.engine);
  g_ui.scenario = FLYREFLEX_SCENARIO_SAFE;
  g_ui.frame = 0;
  g_ui.has_last_log = false;
  update_scenario_buttons();
  g_ui.timer = lv_timer_create(ui_timer_callback, UI_FRAME_PERIOD_MS, NULL);
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
