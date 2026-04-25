#pragma once

#ifdef USE_ESP_IDF

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esp_event.h"

namespace esphome {
namespace esp32_l2_bridge {

// Tracks WIFI_EVENT_STA_CONNECTED / DISCONNECTED. Mirrors STA-link state to
// HA via a binary_sensor.
class StaLinkBinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

 protected:
  static void event_handler_(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data);
};

}  // namespace esp32_l2_bridge
}  // namespace esphome

#endif  // USE_ESP_IDF
