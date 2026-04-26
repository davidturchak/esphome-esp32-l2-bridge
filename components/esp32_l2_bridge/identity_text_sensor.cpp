#include "identity_text_sensor.h"

#ifdef USE_ESP_IDF

#include <cstdio>
#include "esphome/core/log.h"
#include "esp_wifi.h"

namespace esphome {
namespace esp32_l2_bridge {

static const char *const TAG = "esp32_l2_bridge.identity";

static const char *kind_name(IdentityKind k) {
  return k == IdentityKind::AP_MAC ? "AP MAC" : "STA MAC";
}

void IdentityTextSensor::setup() {
  uint8_t mac[6] = {0};
  wifi_interface_t iface =
      (this->kind_ == IdentityKind::AP_MAC) ? WIFI_IF_AP : WIFI_IF_STA;
  esp_err_t err = esp_wifi_get_mac(iface, mac);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "esp_wifi_get_mac(%s) failed: 0x%x", kind_name(this->kind_),
             err);
    this->publish_state("");
    return;
  }
  char buf[18];
  std::snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],
                mac[1], mac[2], mac[3], mac[4], mac[5]);
  this->publish_state(buf);
}

void IdentityTextSensor::dump_config() {
  LOG_TEXT_SENSOR("", kind_name(this->kind_), this);
}

}  // namespace esp32_l2_bridge
}  // namespace esphome

#endif  // USE_ESP_IDF
