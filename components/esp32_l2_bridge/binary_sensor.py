"""Binary sensor platform for esp32_l2_bridge.

Currently only one type — `sta_link`, true while the upstream STA
association is up.
"""

import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import DEVICE_CLASS_CONNECTIVITY, ENTITY_CATEGORY_DIAGNOSTIC

from . import esp32_l2_bridge_ns

DEPENDENCIES = ["esp32_l2_bridge"]

StaLinkBinarySensor = esp32_l2_bridge_ns.class_(
    "StaLinkBinarySensor", binary_sensor.BinarySensor, cg.Component
)


CONFIG_SCHEMA = cv.typed_schema(
    {
        "sta_link": binary_sensor.binary_sensor_schema(
            StaLinkBinarySensor,
            device_class=DEVICE_CLASS_CONNECTIVITY,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ).extend(cv.COMPONENT_SCHEMA),
    }
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)
