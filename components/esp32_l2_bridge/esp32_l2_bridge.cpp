#include "esp32_l2_bridge.h"

#ifdef USE_ESP_IDF

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

extern "C" {
#include "fdb.h"
#include "dhcp_xid_map.h"
#include "dhcp_lease_map.h"
#include "netif_hooks.h"
#include "repeater_config.h"
#include "repeater_forward.h"
}

namespace esphome {
namespace esp32_l2_bridge {

static const char *const TAG = "esp32_l2_bridge";

void L2Bridge::setup() {
  ESP_LOGCONFIG(TAG, "Setting up L2 repeater data plane");
  repeater_set_learn_ttl(this->fdb_default_ttl_);
  repeater_set_dhcp_snoop(this->dhcp_snoop_);
  repeater_forward_init();
  // We run BEFORE wifi_bridge (HARDWARE=800 > WIFI=700) so event handlers
  // catch the very first WIFI_EVENT_AP_START / IP_EVENT_STA_GOT_IP. But that
  // means the default event loop hasn't been created yet — wifi_bridge would
  // do that. Create it defensively. Returns ESP_ERR_INVALID_STATE if already
  // exists, which we ignore.
  esp_event_loop_create_default();
  // ANY_ID covers WIFI_EVENT_AP_START (install AP hooks),
  // WIFI_EVENT_AP_STADISCONNECTED (evict by MAC), and others we ignore.
  esp_err_t r1 = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                            &L2Bridge::event_handler_, this);
  esp_err_t r2 = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                            &L2Bridge::event_handler_, this);
  if (r1 != ESP_OK || r2 != ESP_OK) {
    ESP_LOGE(TAG, "event handler register failed: r1=0x%x r2=0x%x", r1, r2);
    this->mark_failed();
  }
}

void L2Bridge::dump_config() {
  ESP_LOGCONFIG(TAG, "L2 Bridge:");
  ESP_LOGCONFIG(TAG, "  FDB capacity: %d entries", REPEATER_FDB_SIZE);
  ESP_LOGCONFIG(TAG, "  FDB learn TTL: %u s", (unsigned)this->fdb_default_ttl_);
  ESP_LOGCONFIG(TAG, "  XID map capacity: %d entries (TTL %d s)",
                REPEATER_XID_MAP_SIZE, REPEATER_XID_TTL_S);
  ESP_LOGCONFIG(TAG, "  DHCP snoop: %s", this->dhcp_snoop_ ? "on" : "off");
}

void L2Bridge::loop() {
  // Age FDB and DHCP XID map roughly once per second.
  uint32_t now = millis();
  if (now - this->last_age_ms_ >= 1000) {
    this->last_age_ms_ = now;
    fdb_age();
    dhcp_xid_map_age();
    if (this->ap_hooked_ && this->sta_hooked_) {
      bridge_netif_hooks_check();
    }
  }
}

void L2Bridge::event_handler_(void *arg, esp_event_base_t base, int32_t id,
                              void *data) {
  auto *self = static_cast<L2Bridge *>(arg);
  if (base == WIFI_EVENT && id == WIFI_EVENT_AP_START) {
    if (!self->ap_hooked_) {
      esp_netif_t *ap = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
      if (ap) {
        bridge_netif_hooks_init_ap(ap);
        self->ap_hooked_ = true;
      }
    }
  } else if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STADISCONNECTED) {
    // Client left the AP — drop its FDB and lease-map entries so the
    // diagnostic entities reflect physical reality. The data plane will
    // re-learn on reassoc via DHCP-ACK or ARP traffic.
    auto *ev = static_cast<wifi_event_ap_stadisconnected_t *>(data);
    bool fdb_hit = fdb_evict_by_mac(ev->mac);
    bool lease_hit = dhcp_lease_map_evict_by_mac(ev->mac);
    if (fdb_hit || lease_hit) {
      ESP_LOGI(TAG,
               "Client %02x:%02x:%02x:%02x:%02x:%02x left — evicted "
               "(fdb=%s, lease=%s)",
               ev->mac[0], ev->mac[1], ev->mac[2], ev->mac[3], ev->mac[4],
               ev->mac[5], fdb_hit ? "yes" : "no", lease_hit ? "yes" : "no");
    }
  } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
    if (!self->sta_hooked_) {
      esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
      if (sta) {
        bridge_netif_hooks_init_sta(sta);
        self->sta_hooked_ = true;
      }
    }
  }
}

}  // namespace esp32_l2_bridge
}  // namespace esphome

#endif  // USE_ESP_IDF
