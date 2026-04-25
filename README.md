# esphome-esp32-l2-bridge

ESPHome external component that turns an ESP32 into a transparent L2 WiFi
repeater: AP and STA up at the same time on a single radio, software MAC
translation at the lwIP `netif` layer, and DHCP-ACK snooping that puts
AP-side clients on the **upstream subnet** instead of an isolated
`192.168.4.x` network.

Native Home Assistant integration comes for free: the component coexists
with ESPHome's built-in `wifi:`, `api:`, `ota:`, `web_server:`, and
`mqtt:` so the chip is reachable on its STA IP just like any other
ESPHome node.

> **Status: M1–M5 done, hardware-verified** on ESP32 (`esp32doit-devkit-v1`).
> AP clients receive DHCP leases from the upstream subnet, phone
> associates without tearing the STA down, native API listens on port
> 6053, web UI on port 80. Multi-chip example YAMLs (S3/C3/C5/C6) and
> ACL-rule entities land in M6.

## How it works

Two cooperating components:

- **`wifi_bridge`** runs at `setup_priority::AFTER_WIFI` (200), *after*
  ESPHome's stock `wifi:` (250) has finished bringing STA up. It flips
  the radio to `WIFI_MODE_APSTA`, configures the AP, and stops IDF's
  auto-started local DHCPS — the L2 bridge proxies upstream DHCP, so a
  local DHCPS would race the upstream server.
- **`esp32_l2_bridge`** vendors the data plane (FDB, DHCP XID map, DHCP
  lease map, repeater forwarding, lwIP netif hooks) and installs hooks
  on `WIFI_EVENT_AP_START` and `IP_EVENT_STA_GOT_IP`. A periodic
  self-heal in `loop()` reinstalls the hooks when IDF rotates the AP
  netif's input pointer (observed on IDF v5.5 a few seconds after STA
  associates).

Stock `wifi:` keeps STA management (reconnect, roaming, power save) and
is the network-stack provider that `network::is_connected()` consults —
this is why `api:` / `ota:` / `web_server:` work out of the box.

The user's `wifi:` block must **not** include an `ap:` block. The
component compiles in `USE_WIFI_AP` itself so that wifi:'s pre-setup
creates the AP netif, but leaves wifi:'s runtime `has_ap()` false, which
gates out the "disable AP on STA connect" path in
`wifi_component.cpp:1557`.

See [Layer2Bridging.md](https://github.com/davidturchak/esp32_nat_router/blob/esp32_wifi_repeater/Layer2Bridging.md)
in the source firmware for the bridge design rationale.

## Quickstart

```yaml
esphome:
  name: l2-bridge
  friendly_name: L2 Bridge

esp32:
  board: esp32doit-devkit-v1
  framework:
    type: esp-idf
    log_level: INFO
    sdkconfig_options:
      CONFIG_LWIP_IP_FORWARD: "y"
      CONFIG_LWIP_L2_TO_L3_COPY: "y"
      CONFIG_ESP_WIFI_IRAM_OPT: "y"
      CONFIG_ESP_WIFI_RX_IRAM_OPT: "y"
      CONFIG_LWIP_IRAM_OPTIMIZATION: "y"
      CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240: "y"

external_components:
  - source:
      type: git
      url: https://github.com/davidturchak/esphome-esp32-l2-bridge

# Stock wifi: handles STA + provides network: for api/ota/web_server.
# IMPORTANT: do NOT add an `ap:` block here — wifi_bridge brings AP up.
wifi:
  ssid: !secret upstream_ssid
  password: !secret upstream_pass

wifi_bridge:
  ap:
    ssid: "Repeater"
    password: !secret ap_pass
    max_clients: 8

esp32_l2_bridge:
  # All keys optional; defaults shown.
  fdb_size: 32
  fdb_default_ttl: 600s
  xid_map_size: 16
  xid_ttl: 30s
  dhcp_snoop: true

# Diagnostic entities — all auto-discovered by Home Assistant.
sensor:
  - platform: esp32_l2_bridge
    type: fdb_occupancy
    name: "FDB Entries"
  - platform: esp32_l2_bridge
    type: dhcp_lease_count
    name: "DHCP Leases"
  - platform: esp32_l2_bridge
    type: sta_rssi
    name: "Upstream RSSI"

text_sensor:
  - platform: esp32_l2_bridge
    type: connected_clients
    name: "Connected Clients"

binary_sensor:
  - platform: esp32_l2_bridge
    type: sta_link
    name: "STA Link"

button:
  - platform: esp32_l2_bridge
    type: fdb_clear
    name: "Clear FDB"

logger:
  level: INFO
api:
ota:
  - platform: esphome
web_server:
  port: 80
```

Flash with `esphome run l2-bridge.yaml`. The chip joins your upstream
WiFi as a station and exposes the AP. Phones, IoT devices, etc. that
join the AP get DHCP leases on the upstream subnet — they're
indistinguishable on the network from devices on the upstream router.

## Configuration reference

### `wifi_bridge:`

| Key                    | Type    | Default | Notes                                       |
|------------------------|---------|---------|---------------------------------------------|
| `ap.ssid`              | string  | —       | Required.                                   |
| `ap.password`          | string  | `""`    | < 8 chars opens an open AP.                 |
| `ap.channel`           | 0–14    | `0`     | `0` = follow STA channel automatically.     |
| `ap.max_clients`       | 1–10    | `8`     | RAM-bound; ESP32-C3 typically caps at 5.    |
| `ap.hidden`            | bool    | `false` |                                             |

### `esp32_l2_bridge:`

| Key                | Type        | Default | Notes                                                 |
|--------------------|-------------|---------|-------------------------------------------------------|
| `fdb_size`         | 4–128       | `32`    | Compile-time; backs a static array.                   |
| `fdb_default_ttl`  | 10s–24h     | `600s`  | Runtime fallback when DHCP doesn't supply a lease.    |
| `xid_map_size`     | 4–64        | `16`    | DHCP transaction-ID map for snoop matching.           |
| `xid_ttl`          | 5s–10m      | `30s`   |                                                       |
| `dhcp_snoop`       | bool        | `true`  | Off: bridge passes DHCP through without learning.     |

### Home Assistant entities

| Platform        | `type:`              | What it reports                                          |
|-----------------|----------------------|----------------------------------------------------------|
| `sensor`        | `fdb_occupancy`      | Count of valid IP→MAC FDB entries.                       |
| `sensor`        | `dhcp_lease_count`   | Count of non-expired entries in the DHCP lease map.      |
| `sensor`        | `sta_rssi`           | Upstream signal strength (dBm) via `esp_wifi_sta_get_ap_info`. |
| `text_sensor`   | `connected_clients`  | JSON array of `{mac, ip, hostname}` per AP-side client.  |
| `binary_sensor` | `sta_link`           | True while the upstream STA association is up.           |
| `button`        | `fdb_clear`          | Press to flush the FDB.                                  |

All entities accept the standard ESPHome sensor schema (`name:`, `id:`,
`icon:`, `update_interval:` for polled types, etc.).

## Hardware support

| Chip       | Status        | Notes                                            |
|------------|---------------|--------------------------------------------------|
| ESP32      | Verified      | `esp32doit-devkit-v1`, `esp32dev`                |
| ESP32-S3   | M6 (untested) | `esp32-s3-devkitc-1`                             |
| ESP32-C3   | M6 (untested) | 5 AP-client cap (RAM-bound)                      |
| ESP32-C5   | M6 (untested) | Requires ESPHome ≥ 2026.4.0 (ESP-IDF v6.0+)      |
| ESP32-C6   | M6 (untested) |                                                  |
| WT32-ETH01 | Out of scope  | Ethernet↔WiFi-AP bridging needs different plumbing |

Footprint on ESP32 (M5 build): RAM 12.1%, Flash 44.6%.

## Roadmap

- **M1 — AP+STA spike** ✓ Bring AP and STA up at the same time.
- **M2 — Vendor data plane** ✓ FDB, DHCP snoop, MAC translation, lwIP hooks.
- **M3 — YAML schema + codegen** ✓ All tunables exposed.
- **M4 — Home Assistant entities** ✓ Sensors, text_sensor, binary_sensor, button.
- **M5 — Native api/ota/web_server** ✓ Coexist with stock `wifi:`.
- **M6** — Per-target example YAMLs for S3 / C3 / C5 / C6; ACL rules
  exposed as HA `switch` entities.
- **M7** — Polish: OTA rollback parity, hostname configuration on AP.

## Acknowledgements

The vendored data plane (`fdb.{c,h}`, `repeater_forward.{c,h}`,
`netif_hooks.{c,h}`, `dhcp_*.{c,h}`, `repeater_config.h`) is lifted with
minor adjustments from the
[`esp32_wifi_repeater` branch](https://github.com/davidturchak/esp32_nat_router/tree/esp32_wifi_repeater)
of `esp32_nat_router`, originally by Martin Ger.

## License

The upstream `esp32_nat_router` does not currently declare a license.
A license for this repository will be added once the upstream's
licensing intent is settled. Until then, treat the code as
"all rights reserved" and use it for personal / evaluation purposes.
