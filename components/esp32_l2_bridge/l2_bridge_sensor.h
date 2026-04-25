#pragma once

#ifdef USE_ESP_IDF

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace esp32_l2_bridge {

enum class L2BridgeSensorType {
  FDB_OCCUPANCY,
  DHCP_LEASE_COUNT,
  STA_RSSI,
};

class L2BridgeSensor : public sensor::Sensor, public PollingComponent {
 public:
  void set_type(L2BridgeSensorType t) { type_ = t; }
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

 protected:
  L2BridgeSensorType type_{L2BridgeSensorType::FDB_OCCUPANCY};
};

}  // namespace esp32_l2_bridge
}  // namespace esphome

#endif  // USE_ESP_IDF
