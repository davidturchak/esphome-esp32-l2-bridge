#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"

#ifdef USE_ESP_IDF
#include "esp_event.h"
#include "esp_netif.h"
#endif

#include <cstdint>

namespace esphome {
namespace esp32_l2_bridge {

class L2Bridge : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  // Sit between stock wifi: (priority WIFI=250) and wifi_bridge
  // (AFTER_WIFI=200). At WIFI, wifi:'s wifi_pre_setup_ runs and creates
  // the default event loop and STA netif. At BEFORE_CONNECTION=220 we
  // register our WIFI_EVENT/IP_EVENT handlers — guaranteed to be in
  // place before wifi_bridge posts WIFI_EVENT_AP_START.
  // We must NOT run before wifi_pre_setup_, otherwise a defensive
  // esp_event_loop_create_default() here would make wifi:'s pre_setup
  // bail out with ESP_ERR_INVALID_STATE (it treats that as fatal).
  float get_setup_priority() const override { return setup_priority::BEFORE_CONNECTION; }

  // Runtime tunables wired up by codegen. FDB / XID *sizes* are compile-time
  // (cg.add_define) since they back static arrays.
  void set_fdb_default_ttl(uint32_t v) { fdb_default_ttl_ = v; }
  void set_dhcp_snoop(bool v) { dhcp_snoop_ = v; }

 protected:
#ifdef USE_ESP_IDF
  static void event_handler_(void *arg, esp_event_base_t base, int32_t id,
                             void *data);
#endif

  uint32_t fdb_default_ttl_{600};
  bool dhcp_snoop_{true};

  uint32_t last_age_ms_{0};
  bool ap_hooked_{false};
  bool sta_hooked_{false};
};

}  // namespace esp32_l2_bridge
}  // namespace esphome
