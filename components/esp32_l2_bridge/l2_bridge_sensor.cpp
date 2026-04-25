#include "l2_bridge_sensor.h"

#ifdef USE_ESP_IDF

#include "esphome/core/log.h"
#include "esp_wifi.h"

extern "C" {
#include "fdb.h"
#include "dhcp_lease_map.h"
#include "repeater_config.h"
}

namespace esphome {
namespace esp32_l2_bridge {

static const char *const TAG = "esp32_l2_bridge.sensor";

static int count_fdb_entries() {
  fdb_snapshot_entry_t buf[REPEATER_FDB_SIZE];
  return fdb_snapshot(buf, REPEATER_FDB_SIZE);
}

static int count_lease_entries() {
  dhcp_lease_entry_t buf[DHCP_LEASE_MAP_SIZE];
  return dhcp_lease_map_snapshot(buf, DHCP_LEASE_MAP_SIZE);
}

void L2BridgeSensor::update() {
  switch (this->type_) {
    case L2BridgeSensorType::FDB_OCCUPANCY:
      this->publish_state(static_cast<float>(count_fdb_entries()));
      break;
    case L2BridgeSensorType::DHCP_LEASE_COUNT:
      this->publish_state(static_cast<float>(count_lease_entries()));
      break;
    case L2BridgeSensorType::STA_RSSI: {
      wifi_ap_record_t info;
      if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
        this->publish_state(static_cast<float>(info.rssi));
      } else {
        this->publish_state(NAN);
      }
      break;
    }
  }
}

void L2BridgeSensor::dump_config() {
  const char *t = "?";
  switch (this->type_) {
    case L2BridgeSensorType::FDB_OCCUPANCY:    t = "fdb_occupancy"; break;
    case L2BridgeSensorType::DHCP_LEASE_COUNT: t = "dhcp_lease_count"; break;
    case L2BridgeSensorType::STA_RSSI:         t = "sta_rssi"; break;
  }
  LOG_SENSOR("", "L2 Bridge Sensor", this);
  ESP_LOGCONFIG(TAG, "  Type: %s", t);
  LOG_UPDATE_INTERVAL(this);
}

}  // namespace esp32_l2_bridge
}  // namespace esphome

#endif  // USE_ESP_IDF
