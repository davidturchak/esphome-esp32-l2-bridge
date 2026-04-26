#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"

#ifdef USE_ESP_IDF
#include "esp_event.h"
#include "esp_netif.h"
#endif

#include <string>

namespace esphome {
namespace wifi_bridge {

// Brings the soft-AP up alongside ESPHome's stock `wifi:` STA. Runs at
// AFTER_WIFI priority so wifi: has already executed esp_wifi_init /
// esp_wifi_set_mode(STA) / esp_wifi_start. We then create the AP netif,
// flip mode to APSTA, set AP config, and let wifi:'s reconnection logic
// drive the STA without touching our AP (which it can't, because the
// stock wifi: config has no `ap:` block — USE_WIFI_AP is undefined for
// it, so its "disable AP on STA connect" path doesn't compile).
class WiFiBridge : public Component {
 public:
  void setup() override;
  void loop() override {}
  void dump_config() override;

  // AFTER_WIFI = 200 — runs strictly after wifi: (priority WIFI = 250).
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  void set_ap_ssid(const std::string &v) { ap_ssid_ = v; }
  void set_ap_password(const std::string &v) { ap_password_ = v; }
  void set_ap_max_clients(uint8_t v) { ap_max_clients_ = v; }
  void set_ap_hidden(bool v) { ap_hidden_ = v; }

#ifdef USE_ESP_IDF
  esp_netif_t *get_ap_netif() const { return ap_netif_; }
#endif

 protected:
#ifdef USE_ESP_IDF
  void start_ap_();
  static void event_handler_(void *arg, esp_event_base_t base, int32_t id,
                             void *data);
  esp_netif_t *ap_netif_{nullptr};
#endif

  std::string ap_ssid_;
  std::string ap_password_;
  uint8_t ap_max_clients_{8};
  bool ap_hidden_{false};
};

}  // namespace wifi_bridge
}  // namespace esphome
