#pragma once

#ifdef USE_ESP_IDF

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace esp32_l2_bridge {

// Snapshots the DHCP lease map (which has hostname info) on each poll and
// publishes a JSON-encoded array. HA can template per-client cards from it.
class ConnectedClientsTextSensor : public text_sensor::TextSensor,
                                   public PollingComponent {
 public:
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }
};

}  // namespace esp32_l2_bridge
}  // namespace esphome

#endif  // USE_ESP_IDF
