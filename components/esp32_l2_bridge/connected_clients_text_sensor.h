#pragma once

#ifdef USE_ESP_IDF

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace esp32_l2_bridge {

enum class ConnectedClientsFormat {
  TEXT,  // "David-s-S24-Ultra (192.168.1.123), iPad (192.168.1.130)"
  JSON,  // [{"mac":...,"ip":...,"hostname":...}]
};

// Snapshots the DHCP lease map (which has hostname info) on each poll and
// publishes the entries. Default is a comma-separated, human-readable list;
// `format: json` switches to a JSON array suitable for HA templating.
class ConnectedClientsTextSensor : public text_sensor::TextSensor,
                                   public PollingComponent {
 public:
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  void set_format(ConnectedClientsFormat f) { format_ = f; }

 protected:
  ConnectedClientsFormat format_{ConnectedClientsFormat::TEXT};
};

}  // namespace esp32_l2_bridge
}  // namespace esphome

#endif  // USE_ESP_IDF
