# esphome-esp32-l2-bridge

ESPHome external component port of the L2 WiFi repeater from
[`esp32_nat_router` (esp32_wifi_repeater branch)](../esp32_nat_router).

The original firmware turns an ESP32 into an L2 WiFi repeater: AP+STA on
a single radio, software MAC translation at the lwIP `netif` layer, an
IP→MAC FDB populated by data-plane learning + DHCP-ACK snooping. This
repository delivers the same data plane wrapped as a native ESPHome
component, with first-class Home Assistant integration.

> **Status: M1 spike (AP+STA only).** No bridge logic is wired up yet —
> this milestone only validates that ESPHome's lifecycle accommodates
> simultaneous AP+STA. M2 vendors the bridge data-plane.

## Repository layout

```
components/
  wifi_bridge/      # Replaces ESPHome's stock wifi: component
  esp32_l2_bridge/  # (M2+) data-plane bridge: FDB, DHCP snoop, MAC translation
examples/
  esp32-spike.yaml  # M1 verification YAML
```

## Why a custom WiFi component

ESPHome's stock `wifi:` component does **not** support simultaneous AP+STA:
when the STA connects, it tears the AP down (`AP only enabled when no
connection to the WiFi router can be made`). A repeater needs both up
concurrently with the AP channel pinned to the STA channel. `wifi_bridge`
takes over WiFi initialization wholesale rather than fighting the stock
component.

`wifi_bridge` is `CONFLICTS_WITH = ["wifi"]` — having both in one config
is a build-time error.

## Quickstart (M1 spike)

```bash
cd /root/esp/esphome-esp32-l2-bridge

# Create secrets:
cat > examples/secrets.yaml <<'EOF'
upstream_ssid: "your-home-wifi"
upstream_pass: "..."
ap_pass: "repeater-pass"
EOF

esphome run examples/esp32-spike.yaml
```

Acceptance:
1. Serial logs show `WiFi started in APSTA mode` followed by `STA got IP: ...`.
2. A phone associates to the `Repeater-Spike` AP **while** the STA remains
   connected (no `STA disconnected` in logs when the AP client joins).
3. The phone gets an IP from the local AP DHCPS (this is M1 only — in M2 it
   will instead be served by the upstream DHCP server through the bridge).

If (1) and (2) pass, the architecture works and we proceed to M2.

## Roadmap

- **M1 (this commit)** — `wifi_bridge` skeleton, AP+STA up, no bridging.
- **M2** — Vendor `fdb.c` / `repeater_forward.c` / `netif_hooks.c` (stripped of
  ACL/PCAP/LED dependencies) into `components/esp32_l2_bridge/src/`. Hooks
  installed on `WIFI_EVENT_AP_START` and `IP_EVENT_STA_GOT_IP`.
- **M3** — Full YAML schema for `esp32_l2_bridge:` (FDB size, DHCP snoop,
  TTL/MSS clamp, proxy ARP).
- **M4** — Home Assistant entities (`text_sensor` for FDB, `sensor` for
  byte counters, `binary_sensor` for STA link).
- **M5** — ACL port: stateless 4-direction firewall, NVS storage replaced
  by ESPHome `globals`, rules exposed as HA `switch` entities.
- **M6** — Per-target example YAMLs for ESP32 / S3 / C3 / C5 / C6.
  ESP32-C5 requires ESPHome ≥ 2026.4.0.
- **M7** — Polish: OTA rollback parity, README, optional encrypted backup.

## Hardware support

| Chip          | Status     | Notes                                             |
|---------------|------------|---------------------------------------------------|
| ESP32         | M1 target  | Original board, `esp32dev`                        |
| ESP32-S3      | M6         | `esp32-s3-devkitc-1`                              |
| ESP32-C3      | M6         | 5 AP-client cap (RAM-bound)                       |
| ESP32-C5      | M6         | Requires ESPHome ≥ 2026.4.0 (ESP-IDF v6.0+)       |
| ESP32-C6      | M6         |                                                   |
| WT32-ETH01    | Stretch    | Ethernet↔WiFi-AP bridge — different netif plumbing |

## Reference

- [Layer2Bridging.md](../esp32_nat_router/Layer2Bridging.md) — the design
  rationale for the data-plane bridge.
- [ESPHome WiFi Component](https://esphome.io/components/wifi/) — what we replace.
- [ESPHome External Components](https://esphome.io/components/external_components/) —
  packaging convention.
