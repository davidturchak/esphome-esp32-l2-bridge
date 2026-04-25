#include "wifi_bridge.h"

#ifdef USE_ESP_IDF

#include <cstring>
#include "esp_netif.h"
#include "esp_wifi.h"

namespace esphome {
namespace wifi_bridge {

static const char *const TAG = "wifi_bridge";

void WiFiBridge::setup() {
  ESP_LOGCONFIG(TAG, "Setting up wifi_bridge AP alongside stock wifi:");
  this->start_ap_();
}

void WiFiBridge::dump_config() {
  ESP_LOGCONFIG(TAG, "WiFi Bridge:");
  ESP_LOGCONFIG(TAG, "  AP SSID: '%s' (max %u clients%s)",
                this->ap_ssid_.c_str(), this->ap_max_clients_,
                this->ap_hidden_ ? ", hidden" : "");
  if (this->ap_channel_ == 0) {
    ESP_LOGCONFIG(TAG, "  AP channel: auto (follows STA)");
  } else {
    ESP_LOGCONFIG(TAG, "  AP channel: %u", this->ap_channel_);
  }
}

void WiFiBridge::start_ap_() {
  // Stock wifi: has already run wifi_pre_setup_ by this point (priority
  // WIFI=250 > AFTER_WIFI=200): esp_netif_init, default event loop,
  // STA netif, AP netif (we forced USE_WIFI_AP from __init__.py),
  // esp_wifi_init. Since the user's `wifi:` block has no `ap:` config,
  // wifi:'s has_ap() returns false — its disable-AP-on-STA-connect path
  // at wifi_component.cpp:1557 stays gated out.

  this->ap_netif_ = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  if (this->ap_netif_ == nullptr) {
    ESP_LOGE(TAG, "AP netif not registered — did wifi: pre_setup run?");
    this->mark_failed();
    return;
  }

  // Two event handlers:
  // - AP_START: IDF auto-starts a local DHCPS; stop it (L2 bridge proxies
  //   upstream DHCP, a local DHCPS races the upstream server).
  // - STA_CONNECTED: defensive re-arm of WIFI_MODE_APSTA. Stock wifi:'s
  //   wifi_component.cpp:1563 calls wifi_mode_({}, false) when STA
  //   connects IF the user accidentally left an `ap:` block under
  //   `wifi:`. That puts the radio back into STA-only and silently
  //   kills our broadcast. Flipping mode back here makes the misconfig
  //   merely noisy in logs instead of breaking the bridge entirely.
  esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_START,
                             &WiFiBridge::event_handler_, this);
  esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED,
                             &WiFiBridge::event_handler_, this);

  // Switch to APSTA — STA stays up, AP comes up. esp_wifi_set_mode is
  // safe at runtime per IDF docs.
  esp_err_t err = esp_wifi_set_mode(WIFI_MODE_APSTA);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_wifi_set_mode(APSTA) failed: %s", esp_err_to_name(err));
    this->mark_failed();
    return;
  }

  wifi_config_t ap_cfg = {};
  std::strncpy(reinterpret_cast<char *>(ap_cfg.ap.ssid),
               this->ap_ssid_.c_str(), sizeof(ap_cfg.ap.ssid) - 1);
  ap_cfg.ap.ssid_len = this->ap_ssid_.length();
  ap_cfg.ap.channel = this->ap_channel_;  // 0 = auto-follow STA
  ap_cfg.ap.max_connection = this->ap_max_clients_;
  ap_cfg.ap.beacon_interval = 100;
  ap_cfg.ap.ssid_hidden = this->ap_hidden_ ? 1 : 0;
  if (this->ap_password_.length() < 8) {
    ap_cfg.ap.authmode = WIFI_AUTH_OPEN;
  } else {
    ap_cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
    std::strncpy(reinterpret_cast<char *>(ap_cfg.ap.password),
                 this->ap_password_.c_str(), sizeof(ap_cfg.ap.password) - 1);
  }
  err = esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_wifi_set_config(AP) failed: %s", esp_err_to_name(err));
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "AP '%s' configured; WIFI_MODE_APSTA active",
           this->ap_ssid_.c_str());
}

void WiFiBridge::event_handler_(void *arg, esp_event_base_t base, int32_t id,
                                void *data) {
  auto *self = static_cast<WiFiBridge *>(arg);
  if (base != WIFI_EVENT)
    return;

  if (id == WIFI_EVENT_AP_START) {
    if (self->ap_netif_ == nullptr)
      return;
    esp_err_t err = esp_netif_dhcps_stop(self->ap_netif_);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "Local AP DHCPS disabled (L2 bridge proxies upstream DHCP)");
    } else if (err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
      ESP_LOGW(TAG, "esp_netif_dhcps_stop returned: %s", esp_err_to_name(err));
    }
    return;
  }

  if (id == WIFI_EVENT_STA_CONNECTED) {
    // Re-arm APSTA mode. Stock wifi: drops the radio to STA-only here
    // (wifi_component.cpp:1563) when its has_ap() is true — usually a
    // user-config mistake (an `ap:` block left under `wifi:`). If
    // we're already in APSTA, esp_wifi_set_mode is a no-op.
    wifi_mode_t mode = WIFI_MODE_NULL;
    if (esp_wifi_get_mode(&mode) == ESP_OK && mode != WIFI_MODE_APSTA) {
      ESP_LOGW(TAG, "Mode is %d after STA connect, forcing APSTA "
                    "(remove `ap:` from your `wifi:` block to silence)",
               static_cast<int>(mode));
      esp_wifi_set_mode(WIFI_MODE_APSTA);
    }
    return;
  }
}

}  // namespace wifi_bridge
}  // namespace esphome

#endif  // USE_ESP_IDF
