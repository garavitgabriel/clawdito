#pragma once
#include <Arduino.h>
#include "BridgeClient.h"

// Clawdito UI: swipeable landscape pages (Usage / Clawd / Cost, plus Both
// when two bridge profiles are configured), a boot splash and the
// setup-portal screen.

namespace ui {

void init();                                        // build everything, show splash
void set_profiles(const String labels[], uint8_t count);  // before show_main()
void show_main();                                   // reveal page 0
void show_portal(const String& ap, const String& pass);
void next_page();                                   // BOOT-tap navigation
void switch_profile();                              // BOOT 2s-hold: next profile
void update(const UsageSnapshot snaps[], uint8_t count);

}  // namespace ui
