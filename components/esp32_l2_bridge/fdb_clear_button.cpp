#include "fdb_clear_button.h"

#ifdef USE_ESP_IDF

#include "esphome/core/log.h"

extern "C" {
#include "fdb.h"
}

namespace esphome {
namespace esp32_l2_bridge {

static const char *const TAG = "esp32_l2_bridge.button";

void FdbClearButton::press_action() {
  ESP_LOGW(TAG, "Clearing FDB on user request");
  fdb_clear();
}

void FdbClearButton::dump_config() {
  LOG_BUTTON("", "FDB Clear", this);
}

}  // namespace esp32_l2_bridge
}  // namespace esphome

#endif  // USE_ESP_IDF
