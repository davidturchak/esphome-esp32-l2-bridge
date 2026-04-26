"""Text sensor platform for esp32_l2_bridge.

Currently one type — `connected_clients`. Sources are merged so a client
shows up if either knows about it:
  - esp_wifi_ap_get_sta_list() — currently associated to our AP
  - DHCP lease map              — DHCPed through the bridge
Static-IP clients show an IP once they've sent any traffic (via FDB
lookup). The ESP32's own STA MAC is filtered out. Default format is
human-readable ("hostname (ip), …"); set `format: json` for a JSON array
suitable for HA templating.
"""

import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_DIAGNOSTIC

from . import esp32_l2_bridge_ns

DEPENDENCIES = ["esp32_l2_bridge"]

ConnectedClientsTextSensor = esp32_l2_bridge_ns.class_(
    "ConnectedClientsTextSensor", text_sensor.TextSensor, cg.PollingComponent
)
ConnectedClientsFormat = esp32_l2_bridge_ns.enum(
    "ConnectedClientsFormat", is_class=True
)

CONF_FORMAT = "format"
FORMATS = {
    "text": ConnectedClientsFormat.TEXT,
    "json": ConnectedClientsFormat.JSON,
}


CONFIG_SCHEMA = cv.typed_schema(
    {
        "connected_clients": text_sensor.text_sensor_schema(
            ConnectedClientsTextSensor,
            icon="mdi:account-multiple",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        )
        .extend(cv.polling_component_schema("15s"))
        .extend(
            {
                cv.Optional(CONF_FORMAT, default="text"): cv.enum(
                    FORMATS, lower=True
                ),
            }
        ),
    }
)


async def to_code(config):
    var = await text_sensor.new_text_sensor(config)
    await cg.register_component(var, config)
    cg.add(var.set_format(FORMATS[config[CONF_FORMAT]]))
