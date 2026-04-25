#include "sta_link_binary_sensor.h"

#ifdef USE_ESP_IDF

#include "esphome/core/log.h"
#include "esp_event.h"
#include "esp_wifi.h"

namespace esphome {
namespace esp32_l2_bridge {

static const char *const TAG = "esp32_l2_bridge.binary_sensor";

void StaLinkBinarySensor::setup() {
  // Seed initial state if STA was already up by the time we registered.
  wifi_ap_record_t info;
  this->publish_initial_state(esp_wifi_sta_get_ap_info(&info) == ESP_OK);

  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                             &StaLinkBinarySensor::event_handler_, this);
}

void StaLinkBinarySensor::dump_config() {
  LOG_BINARY_SENSOR("", "STA Link", this);
}

void StaLinkBinarySensor::event_handler_(void *arg, esp_event_base_t event_base,
                                         int32_t event_id, void *event_data) {
  auto *self = static_cast<StaLinkBinarySensor *>(arg);
  if (event_base != WIFI_EVENT) return;
  if (event_id == WIFI_EVENT_STA_CONNECTED) {
    self->publish_state(true);
  } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
    self->publish_state(false);
  }
}

}  // namespace esp32_l2_bridge
}  // namespace esphome

#endif  // USE_ESP_IDF
