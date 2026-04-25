"""Text sensor platform for esp32_l2_bridge.

Currently one type — `connected_clients`. Source of truth is the AP's
associated-station list, enriched with hostname + IP from the DHCP lease
map (so DHCP clients show their hostname) and with the IP from the FDB
(so static-IP clients still show an IP once they've sent any traffic).
Default format is human-readable ("hostname (ip), …"); set `format: json`
for a JSON array suitable for HA templating.
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
