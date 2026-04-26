"""ESPHome external component: wifi_bridge.

Adjunct to ESPHome's stock `wifi:` component. wifi: handles STA
(upstream connection + the `network:` framework integration that api/
ota/web_server/mqtt depend on). wifi_bridge brings the AP up alongside,
in WIFI_MODE_APSTA, AFTER wifi: has finished its STA setup.

The user's YAML must NOT include `ap:` under the stock `wifi:` block —
that triggers wifi:'s "disable AP on STA connect" path. Putting AP
config here keeps that path gated out (USE_WIFI_AP undefined for wifi:).
"""

import esphome.codegen as cg
from esphome.components.esp32 import add_idf_sdkconfig_option
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_SSID,
    CONF_PASSWORD,
    CONF_AP,
    CONF_HIDDEN,
)

CODEOWNERS = ["@dturchak"]
DEPENDENCIES = ["esp32", "wifi"]

CONF_MAX_CLIENTS = "max_clients"

wifi_bridge_ns = cg.esphome_ns.namespace("wifi_bridge")
WiFiBridge = wifi_bridge_ns.class_("WiFiBridge", cg.Component)

AP_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_SSID): cv.ssid,
        cv.Optional(CONF_PASSWORD, default=""): cv.string,
        cv.Optional(CONF_MAX_CLIENTS, default=8): cv.int_range(min=1, max=10),
        cv.Optional(CONF_HIDDEN, default=False): cv.boolean,
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(WiFiBridge),
        cv.Required(CONF_AP): AP_SCHEMA,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    # Stock wifi: gates the IDF SOFTAP capability and the AP netif creation
    # behind two separate switches:
    #   - CONFIG_ESP_WIFI_SOFTAP_SUPPORT (sdkconfig) — IDF wifi driver
    #     compiles in AP code paths; needed for esp_wifi_set_mode(APSTA)
    #     and esp_wifi_set_config(WIFI_IF_AP, ...).
    #   - USE_WIFI_AP (preprocessor) — wifi:'s own ESP-IDF C++ glue calls
    #     esp_netif_create_default_wifi_ap() during wifi_pre_setup_,
    #     which wires the AP netif into lwIP and into the wifi driver.
    # Both go off when the user's wifi: config has no `ap:` block.
    # We re-enable both so the AP netif exists by the time wifi_bridge
    # runs — and since the YAML still has no ap: under wifi:, `has_ap()`
    # stays false at runtime, so wifi:'s "disable AP on STA connect"
    # path (gated by has_ap()) doesn't fire.
    add_idf_sdkconfig_option("CONFIG_ESP_WIFI_SOFTAP_SUPPORT", True)
    # Defining USE_WIFI_AP makes wifi:'s ESP-IDF C++ glue compile in
    # wifi_ap_ip_config_, which references esp_netif_dhcps_{start,stop,
    # option}. Those live in lwIP behind CONFIG_LWIP_DHCPS — re-enable
    # so the symbols link. Runtime: wifi_ap_ip_config_ is only called
    # from setup_ap_config_ which is only called when has_ap() is true,
    # and has_ap() stays false because the YAML's wifi: has no ap: block.
    # We do still call esp_netif_dhcps_stop ourselves on the AP netif
    # before forwarding starts — IDF auto-starts the local DHCPS when
    # the AP comes up, and we need it OFF so our L2 bridge can proxy
    # upstream DHCP without competing.
    add_idf_sdkconfig_option("CONFIG_LWIP_DHCPS", True)
    cg.add_define("USE_WIFI_AP")

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    ap = config[CONF_AP]
    cg.add(var.set_ap_ssid(ap[CONF_SSID]))
    cg.add(var.set_ap_password(ap[CONF_PASSWORD]))
    cg.add(var.set_ap_max_clients(ap[CONF_MAX_CLIENTS]))
    cg.add(var.set_ap_hidden(ap[CONF_HIDDEN]))
