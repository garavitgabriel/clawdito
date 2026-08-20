#pragma once
#include <lvgl.h>

// LVGL <-> hardware glue: draw buffers, flush callback into the JD9853
// blitter, pointer input from the AXS5106L, and the tick source.

namespace lvgl_glue {

void init();     // call after display::init() and touch::init()
void loop();     // lv_timer_handler wrapper

}  // namespace lvgl_glue
