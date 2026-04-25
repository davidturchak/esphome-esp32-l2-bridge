"""Text sensor platform for esp32_l2_bridge.

Currently only one type — `connected_clients`, a JSON-encoded snapshot of
the DHCP lease map (mac/ip/hostname per AP-side client).
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


CONFIG_SCHEMA = cv.typed_schema(
    {
        "connected_clients": text_sensor.text_sensor_schema(
            ConnectedClientsTextSensor,
            icon="mdi:account-multiple",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ).extend(cv.polling_component_schema("15s")),
    }
)


async def to_code(config):
    var = await text_sensor.new_text_sensor(config)
    await cg.register_component(var, config)
