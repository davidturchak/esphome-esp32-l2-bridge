#pragma once

#ifdef USE_ESP_IDF

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace esp32_l2_bridge {

enum class IdentityKind {
  STA_MAC,
  AP_MAC,
};

// Static identity values that publish once at setup. The MAC is set from
// the chip's efuse base at boot and never changes, so there's no point
// polling — one publish_state() in setup() is enough.
class IdentityTextSensor : public text_sensor::TextSensor, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  void set_kind(IdentityKind k) { kind_ = k; }

 protected:
  IdentityKind kind_{IdentityKind::STA_MAC};
};

}  // namespace esp32_l2_bridge
}  // namespace esphome

#endif  // USE_ESP_IDF
