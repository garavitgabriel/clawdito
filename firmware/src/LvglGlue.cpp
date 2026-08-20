#include "LvglGlue.h"
#include "DisplayJD9853.h"
#include "TouchAXS5106.h"
#include <esp_timer.h>

static const uint32_t TICK_MS = 5;
static const size_t BUF_PIXELS = SCREEN_WIDTH * SCREEN_HEIGHT / 10;

static lv_disp_draw_buf_t s_draw_buf;
static lv_color_t s_buf_a[BUF_PIXELS];
static lv_color_t s_buf_b[BUF_PIXELS];

namespace lvgl_glue {

static void flush_cb(lv_disp_drv_t* drv, const lv_area_t* area,
                     lv_color_t* pixels) {
  display::blit(area->x1, area->y1, area->x2, area->y2,
                (const uint16_t*)&pixels->full);
  lv_disp_flush_ready(drv);
}

static void input_cb(lv_indev_drv_t*, lv_indev_data_t* data) {
  uint16_t x, y;
  if (touch::read(&x, &y)) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

static void tick_cb(void*) { lv_tick_inc(TICK_MS); }

void init() {
  lv_init();
  lv_disp_draw_buf_init(&s_draw_buf, s_buf_a, s_buf_b, BUF_PIXELS);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = SCREEN_WIDTH;
  disp_drv.ver_res = SCREEN_HEIGHT;
  disp_drv.flush_cb = flush_cb;
  disp_drv.draw_buf = &s_draw_buf;
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = input_cb;
  lv_indev_drv_register(&indev_drv);

  const esp_timer_create_args_t tick_args = {
      .callback = tick_cb, .name = "lv_tick"};
  esp_timer_handle_t th = nullptr;
  esp_timer_create(&tick_args, &th);
  esp_timer_start_periodic(th, TICK_MS * 1000);
}

void loop() { lv_timer_handler(); }

}  // namespace lvgl_glue
