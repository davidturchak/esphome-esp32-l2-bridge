#pragma once

#ifdef USE_ESP_IDF

#include "esphome/core/component.h"
#include "esphome/components/button/button.h"

namespace esphome {
namespace esp32_l2_bridge {

// Pressing this clears the FDB (and DHCP XID map). Mostly a debug tool —
// expected behavior is FDB self-heals via DHCP-ACK + data-plane learning,
// but a hard reset is occasionally useful when MACs change behind a NAT.
class FdbClearButton : public button::Button, public Component {
 public:
  void dump_config() override;

 protected:
  void press_action() override;
};

}  // namespace esp32_l2_bridge
}  // namespace esphome

#endif  // USE_ESP_IDF
