#include "connected_clients_text_sensor.h"

#ifdef USE_ESP_IDF

#include <cstdio>
#include <string>
#include "esphome/core/log.h"

extern "C" {
#include "dhcp_lease_map.h"
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

void ConnectedClientsTextSensor::update() {
  dhcp_lease_entry_t leases[DHCP_LEASE_MAP_SIZE];
  int n = dhcp_lease_map_snapshot(leases, DHCP_LEASE_MAP_SIZE);

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
  this->publish_state(out);
}

void ConnectedClientsTextSensor::dump_config() {
  LOG_TEXT_SENSOR("", "Connected Clients", this);
  LOG_UPDATE_INTERVAL(this);
}

}  // namespace esp32_l2_bridge
}  // namespace esphome

#endif  // USE_ESP_IDF
