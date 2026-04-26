#include "connected_clients_text_sensor.h"

#ifdef USE_ESP_IDF

#include <cstdio>
#include <cstring>
#include <string>
#include "esphome/core/log.h"
#include "esp_wifi.h"

extern "C" {
#include "dhcp_lease_map.h"
#include "fdb.h"
}

namespace esphome {
namespace esp32_l2_bridge {

static const char *const TAG = "esp32_l2_bridge.text_sensor";

static void escape_json_str(std::string &out, const char *s) {
  for (; *s; s++) {
    char c = *s;
    if (c == '"' || c == '\\') {
      out.push_back('\\');
      out.push_back(c);
    } else if (static_cast<unsigned char>(c) < 0x20) {
      char buf[8];
      std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(c));
      out.append(buf);
    } else {
      out.push_back(c);
    }
  }
}

static std::string format_json(const dhcp_lease_entry_t *leases, int n) {
  std::string out;
  out.reserve(64 * (n + 1));
  out.push_back('[');
  for (int i = 0; i < n; i++) {
    if (i) out.push_back(',');
    char tmp[80];
    const uint8_t *m = leases[i].mac;
    uint32_t ip = leases[i].ip;
    std::snprintf(tmp, sizeof(tmp),
                  "{\"mac\":\"%02x:%02x:%02x:%02x:%02x:%02x\","
                  "\"ip\":\"%u.%u.%u.%u\",\"hostname\":\"",
                  m[0], m[1], m[2], m[3], m[4], m[5],
                  static_cast<unsigned>(ip & 0xff),
                  static_cast<unsigned>((ip >> 8) & 0xff),
                  static_cast<unsigned>((ip >> 16) & 0xff),
                  static_cast<unsigned>((ip >> 24) & 0xff));
    out.append(tmp);
    escape_json_str(out, leases[i].hostname);
    out.append("\"}");
  }
  out.push_back(']');
  return out;
}

// Format: "hostname (ip), hostname (ip), ..." — falls back to MAC when the
// client never sent a hostname in its DHCP request.
static std::string format_text(const dhcp_lease_entry_t *leases, int n) {
  if (n == 0) return std::string();
  std::string out;
  out.reserve(48 * n);
  for (int i = 0; i < n; i++) {
    if (i) out.append(", ");
    char tmp[80];
    const uint8_t *m = leases[i].mac;
    uint32_t ip = leases[i].ip;
    if (leases[i].hostname[0]) {
      out.append(leases[i].hostname);
    } else {
      std::snprintf(tmp, sizeof(tmp),
                    "%02x:%02x:%02x:%02x:%02x:%02x",
                    m[0], m[1], m[2], m[3], m[4], m[5]);
      out.append(tmp);
    }
    if (ip != 0) {
      std::snprintf(tmp, sizeof(tmp), " (%u.%u.%u.%u)",
                    static_cast<unsigned>(ip & 0xff),
                    static_cast<unsigned>((ip >> 8) & 0xff),
                    static_cast<unsigned>((ip >> 16) & 0xff),
                    static_cast<unsigned>((ip >> 24) & 0xff));
      out.append(tmp);
    }
  }
  return out;
}

// We merge two sources so a client shows up if either knows about it:
//   1. esp_wifi_ap_get_sta_list() — currently associated to our AP
//   2. dhcp_lease_map snapshot     — DHCPed through the bridge
// Each MAC appears once. The ESP32's own STA MAC is filtered out so a
// snooped self-renew on the upstream side doesn't masquerade as a client.
void ConnectedClientsTextSensor::update() {
  uint8_t self_sta_mac[6] = {0};
  esp_wifi_get_mac(WIFI_IF_STA, self_sta_mac);

  dhcp_lease_entry_t entries[DHCP_LEASE_MAP_SIZE];
  int n = 0;

  wifi_sta_list_t sta_list;
  int sta_n = 0;
  if (esp_wifi_ap_get_sta_list(&sta_list) == ESP_OK) {
    sta_n = sta_list.num;
    for (int i = 0;
         i < sta_list.num && n < static_cast<int>(DHCP_LEASE_MAP_SIZE); i++) {
      const uint8_t *mac = sta_list.sta[i].mac;
      if (std::memcmp(mac, self_sta_mac, 6) == 0) continue;
      std::memset(&entries[n], 0, sizeof(entries[n]));
      std::memcpy(entries[n].mac, mac, 6);
      uint32_t ip = 0;
      char hostname[DHCP_LEASE_HOSTNAME_MAX] = {0};
      if (dhcp_lease_map_lookup(entries[n].mac, &ip, hostname,
                                DHCP_LEASE_HOSTNAME_MAX)) {
        entries[n].ip = ip;
        std::strncpy(entries[n].hostname, hostname,
                     DHCP_LEASE_HOSTNAME_MAX - 1);
      } else {
        entries[n].ip = fdb_lookup_by_mac(entries[n].mac);
      }
      n++;
    }
  }

  dhcp_lease_entry_t leases[DHCP_LEASE_MAP_SIZE];
  int lease_n = dhcp_lease_map_snapshot(leases, DHCP_LEASE_MAP_SIZE);
  for (int i = 0;
       i < lease_n && n < static_cast<int>(DHCP_LEASE_MAP_SIZE); i++) {
    if (leases[i].ip == 0) continue;
    if (std::memcmp(leases[i].mac, self_sta_mac, 6) == 0) continue;
    bool dup = false;
    for (int j = 0; j < n; j++) {
      if (std::memcmp(entries[j].mac, leases[i].mac, 6) == 0) {
        dup = true;
        break;
      }
    }
    if (dup) continue;
    entries[n++] = leases[i];
  }

  ESP_LOGD(TAG, "update: sta_list=%d lease_map=%d merged=%d", sta_n, lease_n,
           n);

  std::string out = (this->format_ == ConnectedClientsFormat::JSON)
                        ? format_json(entries, n)
                        : format_text(entries, n);
  this->publish_state(out);
}

void ConnectedClientsTextSensor::dump_config() {
  LOG_TEXT_SENSOR("", "Connected Clients", this);
  ESP_LOGCONFIG(TAG, "  Format: %s",
                this->format_ == ConnectedClientsFormat::JSON ? "json" : "text");
  LOG_UPDATE_INTERVAL(this);
}

}  // namespace esp32_l2_bridge
}  // namespace esphome

#endif  // USE_ESP_IDF
