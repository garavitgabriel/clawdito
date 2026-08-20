#include "Ui.h"
#include <lvgl.h>

// ----- Palette: dark violet ground, Clawd orange accent -----
#define C_BG      lv_color_hex(0x17121f)
#define C_CARD    lv_color_hex(0x2b2436)
#define C_PILL    lv_color_hex(0x4a4158)
#define C_TRACK   lv_color_hex(0x1c1626)
#define C_OK      lv_color_hex(0x8bd450)
#define C_WARN    lv_color_hex(0xfbbf24)
#define C_HOT     lv_color_hex(0xef4444)
#define C_TEXT    lv_color_hex(0xf2eef7)
#define C_DIM     lv_color_hex(0xb9b0c7)
#define C_CLAWD   lv_color_hex(0xd97757)
#define C_EYE     lv_color_hex(0x14101a)
#define C_ORANGE  lv_color_hex(0xe0784f)
#define C_GREEN   lv_color_hex(0x35d399)

#define FONT_XL   &lv_font_montserrat_32
#define FONT_LG   &lv_font_montserrat_28
#define FONT_TITLE &lv_font_montserrat_22
#define FONT_MD   &lv_font_montserrat_16
#define FONT_SM   &lv_font_montserrat_12

static const uint8_t N_PAGES = 3;
static lv_obj_t* s_pages[N_PAGES];
static uint8_t s_active = 0;
static lv_obj_t* s_splash = nullptr;
static lv_obj_t* s_portal = nullptr;

// page 0 widgets
static lv_obj_t* w_dot0;
static lv_obj_t* w_pct5;
static lv_obj_t* w_bar5;
static lv_obj_t* w_reset5;
static lv_obj_t* w_pct7;
static lv_obj_t* w_bar7;
static lv_obj_t* w_reset7;
static lv_obj_t* w_mood0;
// page 1 widgets
static lv_obj_t* w_eye_l;
static lv_obj_t* w_eye_r;
static lv_obj_t* w_mood1;
static lv_obj_t* w_dot1;
// page 2 widgets
static lv_obj_t* w_today;
static lv_obj_t* w_month;
static lv_obj_t* w_yday;
static lv_obj_t* w_bars[7];
static lv_obj_t* w_days[7];
static lv_obj_t* w_dot2;

// Claude-Code-flavored gerunds for the footer.
static const char* MOODS[] = {
    "Flibbertigibbeting", "Reticulating", "Percolating", "Noodling",
    "Cogitating",         "Marinating",   "Schlepping",  "Wrangling",
    "Puttering",          "Ruminating",   "Simmering",   "Doodling",
};
static uint8_t s_mood = 0;

namespace ui {

// ----- small builders -----

static lv_obj_t* solid(lv_obj_t* parent, int x, int y, int w, int h,
                       lv_color_t col) {
  lv_obj_t* o = lv_obj_create(parent);
  lv_obj_remove_style_all(o);
  lv_obj_set_size(o, w, h);
  lv_obj_set_pos(o, x, y);
  lv_obj_set_style_bg_color(o, col, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN);
  return o;
}

static lv_obj_t* text(lv_obj_t* parent, const char* s, const lv_font_t* f,
                      lv_color_t col) {
  lv_obj_t* l = lv_label_create(parent);
  lv_label_set_text(l, s);
  lv_obj_set_style_text_font(l, f, LV_PART_MAIN);
  lv_obj_set_style_text_color(l, col, LV_PART_MAIN);
  return l;
}

// Clawd, drawn from rectangles; px = pixel size. 15x11 grid.
static lv_obj_t* clawd(lv_obj_t* parent, int px,
                       lv_obj_t** eye_l = nullptr, lv_obj_t** eye_r = nullptr) {
  lv_obj_t* c = lv_obj_create(parent);
  lv_obj_remove_style_all(c);
  lv_obj_set_size(c, 15 * px, 11 * px);
  lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);

  solid(c, 2 * px, 0, 11 * px, 8 * px, C_CLAWD);        // body
  solid(c, 0, 2 * px, 2 * px, 3 * px, C_CLAWD);         // left nub
  solid(c, 13 * px, 2 * px, 2 * px, 3 * px, C_CLAWD);   // right nub
  lv_obj_t* el = solid(c, 4 * px, 2 * px, 2 * px, 2 * px, C_EYE);
  lv_obj_t* er = solid(c, 9 * px, 2 * px, 2 * px, 2 * px, C_EYE);
  solid(c, 3 * px, 8 * px, 2 * px, 3 * px, C_CLAWD);    // legs
  solid(c, 6 * px, 8 * px, 2 * px, 3 * px, C_CLAWD);
  solid(c, 10 * px, 8 * px, 2 * px, 3 * px, C_CLAWD);
  if (eye_l) *eye_l = el;
  if (eye_r) *eye_r = er;
  return c;
}

static lv_obj_t* status_dot(lv_obj_t* page) {
  lv_obj_t* d = solid(page, 0, 0, 9, 9, C_HOT);
  lv_obj_set_style_radius(d, 5, LV_PART_MAIN);
  lv_obj_align(d, LV_ALIGN_TOP_RIGHT, -4, 8);
  return d;
}

static lv_obj_t* page(lv_obj_t* screen) {
  lv_obj_t* p = lv_obj_create(screen);
  lv_obj_remove_style_all(p);
  lv_obj_set_size(p, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_color(p, C_BG, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(p, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_pad_all(p, 6, LV_PART_MAIN);
  lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(p, LV_OBJ_FLAG_HIDDEN);
  return p;
}

// Rate-limit card: big %, pill tag, bar, reset countdown.
static void limit_card(lv_obj_t* parent, int y, const char* tag,
                       lv_obj_t** pct, lv_obj_t** bar, lv_obj_t** reset) {
  lv_obj_t* card = lv_obj_create(parent);
  lv_obj_remove_style_all(card);
  lv_obj_set_size(card, 308, 56);
  lv_obj_set_pos(card, 0, y);
  lv_obj_set_style_bg_color(card, C_CARD, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(card, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_hor(card, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_top(card, 2, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(card, 0, LV_PART_MAIN);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  *pct = text(card, "--%", FONT_LG, C_TEXT);
  lv_obj_align(*pct, LV_ALIGN_TOP_LEFT, 0, -2);

  lv_obj_t* pill = solid(card, 0, 0, 76, 19, C_PILL);
  lv_obj_set_style_radius(pill, 9, LV_PART_MAIN);
  lv_obj_align(pill, LV_ALIGN_TOP_RIGHT, 0, 1);
  lv_obj_t* pt = text(pill, tag, FONT_SM, C_TEXT);
  lv_obj_center(pt);

  *bar = lv_bar_create(card);
  lv_obj_set_size(*bar, 292, 7);
  lv_obj_align(*bar, LV_ALIGN_TOP_LEFT, 0, 30);
  lv_obj_set_style_bg_color(*bar, C_TRACK, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(*bar, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(*bar, 3, LV_PART_MAIN);
  lv_obj_set_style_bg_color(*bar, C_OK, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(*bar, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_radius(*bar, 3, LV_PART_INDICATOR);
  lv_bar_set_range(*bar, 0, 100);

  *reset = text(card, "waiting for data...", FONT_SM, C_DIM);
  lv_obj_align(*reset, LV_ALIGN_TOP_LEFT, 0, 40);
}

// ----- page builders -----

static void build_usage(lv_obj_t* p) {
  lv_obj_t* mascot = clawd(p, 2);
  lv_obj_set_pos(mascot, 2, 1);

  lv_obj_t* title = text(p, "Usage", FONT_TITLE, C_TEXT);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

  w_dot0 = status_dot(p);
  limit_card(p, 27, "Current", &w_pct5, &w_bar5, &w_reset5);
  limit_card(p, 88, "Weekly", &w_pct7, &w_bar7, &w_reset7);

  w_mood0 = text(p, "* Flibbertigibbeting...", FONT_SM, C_ORANGE);
  lv_obj_align(w_mood0, LV_ALIGN_BOTTOM_LEFT, 2, 1);
}

static void blink_open(lv_timer_t*) {
  lv_obj_clear_flag(w_eye_l, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(w_eye_r, LV_OBJ_FLAG_HIDDEN);
}

static void blink_close(lv_timer_t*) {
  lv_obj_add_flag(w_eye_l, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(w_eye_r, LV_OBJ_FLAG_HIDDEN);
  lv_timer_t* t = lv_timer_create(blink_open, 150, nullptr);
  lv_timer_set_repeat_count(t, 1);
}

static void build_mascot(lv_obj_t* p) {
  lv_obj_t* big = clawd(p, 8, &w_eye_l, &w_eye_r);
  lv_obj_align(big, LV_ALIGN_CENTER, 0, -14);

  w_dot1 = status_dot(p);

  w_mood1 = text(p, "* Flibbertigibbeting...", FONT_MD, C_ORANGE);
  lv_obj_align(w_mood1, LV_ALIGN_BOTTOM_MID, 0, -6);

  lv_timer_create(blink_close, 3700, nullptr);
}

static void build_cost(lv_obj_t* p) {
  lv_obj_t* title = text(p, "Cost", FONT_TITLE, C_TEXT);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
  w_dot2 = status_dot(p);

  lv_obj_t* lt = text(p, "TODAY", FONT_SM, C_DIM);
  lv_obj_set_pos(lt, 4, 30);
  w_today = text(p, "$0.00", FONT_XL, C_GREEN);
  lv_obj_set_pos(w_today, 4, 44);
  w_yday = text(p, "yday $0.00", FONT_SM, C_DIM);
  lv_obj_set_pos(w_yday, 4, 80);

  lv_obj_t* lm = text(p, "MONTH", FONT_SM, C_DIM);
  lv_obj_set_pos(lm, 190, 30);
  w_month = text(p, "$0", FONT_TITLE, C_TEXT);
  lv_obj_set_pos(w_month, 190, 46);

  for (int i = 0; i < 7; i++) {
    w_bars[i] = solid(p, 6 + i * 43, 140, 30, 2, C_GREEN);
    lv_obj_set_style_radius(w_bars[i], 2, LV_PART_MAIN);
    w_days[i] = text(p, "", FONT_SM, C_DIM);
    lv_obj_set_pos(w_days[i], 12 + i * 43, 144);
  }
}

// ----- navigation -----

static void switch_to(uint8_t idx) {
  uint8_t target = idx % N_PAGES;
  if (target == s_active) return;
  uint8_t prev = s_active;
  s_active = target;
  lv_obj_clear_flag(s_pages[target], LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_pages[prev], LV_OBJ_FLAG_HIDDEN);
}

static void on_gesture(lv_event_t*) {
  lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
  if (dir == LV_DIR_LEFT)  switch_to(s_active + 1);
  if (dir == LV_DIR_RIGHT) switch_to(s_active + N_PAGES - 1);
  lv_indev_wait_release(lv_indev_get_act());
}

static void mood_tick(lv_timer_t*) {
  s_mood = (s_mood + 1) % (sizeof(MOODS) / sizeof(MOODS[0]));
  lv_label_set_text_fmt(w_mood0, "* %s...", MOODS[s_mood]);
  lv_label_set_text_fmt(w_mood1, "* %s...", MOODS[s_mood]);
}

// ----- public API -----

void init() {
  lv_obj_t* scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, C_BG, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

  for (uint8_t i = 0; i < N_PAGES; i++) s_pages[i] = page(scr);
  build_usage(s_pages[0]);
  build_mascot(s_pages[1]);
  build_cost(s_pages[2]);

  lv_obj_add_event_cb(scr, on_gesture, LV_EVENT_GESTURE, nullptr);
  lv_timer_create(mood_tick, 7000, nullptr);

  // splash overlay
  s_splash = page(scr);
  lv_obj_clear_flag(s_splash, LV_OBJ_FLAG_HIDDEN);
  lv_obj_t* m = clawd(s_splash, 4);
  lv_obj_align(m, LV_ALIGN_CENTER, 0, -24);
  lv_obj_t* t = text(s_splash, "Clawdito", FONT_TITLE, C_ORANGE);
  lv_obj_align(t, LV_ALIGN_CENTER, 0, 22);
  lv_obj_t* v = text(s_splash, "v1.0", FONT_SM, C_DIM);
  lv_obj_align(v, LV_ALIGN_CENTER, 0, 44);
}

void show_main() {
  if (s_splash) lv_obj_add_flag(s_splash, LV_OBJ_FLAG_HIDDEN);
  if (s_portal) lv_obj_add_flag(s_portal, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s_pages[s_active], LV_OBJ_FLAG_HIDDEN);
}

void show_portal(const String& ap, const String& pass) {
  if (s_splash) lv_obj_add_flag(s_splash, LV_OBJ_FLAG_HIDDEN);
  if (!s_portal) {
    s_portal = page(lv_scr_act());
    lv_obj_t* m = clawd(s_portal, 3);
    lv_obj_set_pos(m, 6, 4);
    lv_obj_t* t = text(s_portal, "Setup Mode", FONT_TITLE, C_ORANGE);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 20, 6);

    char buf[96];
    snprintf(buf, sizeof(buf), "1. Join WiFi:  %s", ap.c_str());
    lv_obj_t* l1 = text(s_portal, buf, FONT_MD, C_TEXT);
    lv_obj_set_pos(l1, 8, 52);
    snprintf(buf, sizeof(buf), "2. Password:  %s", pass.c_str());
    lv_obj_t* l2 = text(s_portal, buf, FONT_MD, C_TEXT);
    lv_obj_set_pos(l2, 8, 80);
    lv_obj_t* l3 = text(s_portal, "3. Open:  http://192.168.4.1", FONT_MD, C_TEXT);
    lv_obj_set_pos(l3, 8, 108);
    lv_obj_t* l4 = text(s_portal, "hold BOOT 5s anytime to return here", FONT_SM, C_DIM);
    lv_obj_align(l4, LV_ALIGN_BOTTOM_MID, 0, -2);
  }
  lv_obj_clear_flag(s_portal, LV_OBJ_FLAG_HIDDEN);
}

void next_page() { switch_to(s_active + 1); }

static void fmt_reset(char* out, size_t cap, uint32_t secs) {
  uint32_t m = secs / 60;
  if (m < 60)        snprintf(out, cap, "Resets in %um", (unsigned)m);
  else if (m < 1440) snprintf(out, cap, "Resets in %uh %um",
                              (unsigned)(m / 60), (unsigned)(m % 60));
  else               snprintf(out, cap, "Resets in %ud %uh",
                              (unsigned)(m / 1440), (unsigned)((m % 1440) / 60));
}

static lv_color_t heat(uint8_t pct) {
  if (pct >= 90) return C_HOT;
  if (pct >= 70) return C_WARN;
  return C_OK;
}

void update(const UsageSnapshot& s) {
  lv_color_t dc = s.online ? C_OK : C_HOT;
  lv_obj_set_style_bg_color(w_dot0, dc, LV_PART_MAIN);
  lv_obj_set_style_bg_color(w_dot1, dc, LV_PART_MAIN);
  lv_obj_set_style_bg_color(w_dot2, dc, LV_PART_MAIN);

  char buf[40];
  if (s.limits_ok) {
    lv_label_set_text_fmt(w_pct5, "%u%%", (unsigned)s.pct_5h);
    lv_bar_set_value(w_bar5, s.pct_5h, LV_ANIM_ON);
    lv_obj_set_style_bg_color(w_bar5, heat(s.pct_5h), LV_PART_INDICATOR);
    fmt_reset(buf, sizeof(buf), s.reset_5h_s);
    lv_label_set_text(w_reset5, buf);

    lv_label_set_text_fmt(w_pct7, "%u%%", (unsigned)s.pct_7d);
    lv_bar_set_value(w_bar7, s.pct_7d, LV_ANIM_ON);
    lv_obj_set_style_bg_color(w_bar7, heat(s.pct_7d), LV_PART_INDICATOR);
    fmt_reset(buf, sizeof(buf), s.reset_7d_s);
    lv_label_set_text(w_reset7, buf);
  } else if (!s.online) {
    lv_label_set_text(w_reset5, "bridge offline");
    lv_label_set_text(w_reset7, "bridge offline");
  }

  // Cost page (LVGL's formatter has no float support: use libc snprintf)
  snprintf(buf, sizeof(buf), "$%.2f", s.today_usd);
  lv_label_set_text(w_today, buf);
  snprintf(buf, sizeof(buf), "$%.0f", s.month_usd);
  lv_label_set_text(w_month, buf);
  snprintf(buf, sizeof(buf), "yday $%.2f", s.last7_usd[5]);
  lv_label_set_text(w_yday, buf);

  float top = 0.01f;
  for (int i = 0; i < 7; i++) top = max(top, s.last7_usd[i]);
  for (int i = 0; i < 7; i++) {
    int h = (int)(s.last7_usd[i] / top * 36.0f);
    if (h < 2) h = 2;
    lv_obj_set_size(w_bars[i], 30, h);
    lv_obj_set_pos(w_bars[i], 6 + i * 43, 142 - h);
    lv_label_set_text(w_days[i], s.last7_day[i]);
  }
}

}  // namespace ui
