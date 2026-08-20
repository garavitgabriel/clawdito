#pragma once
#include <Arduino.h>
#include "BridgeClient.h"

// Clawdito UI: three swipeable landscape pages (Usage / Clawd / Cost),
// plus a boot splash and the setup-portal screen.

namespace ui {

void init();                                        // build everything, show splash
void show_main();                                   // reveal page 0
void show_portal(const String& ap, const String& pass);
void next_page();                                   // BOOT-tap navigation
void update(const UsageSnapshot& s);

}  // namespace ui
