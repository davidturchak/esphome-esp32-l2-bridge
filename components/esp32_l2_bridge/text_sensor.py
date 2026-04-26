"""Text sensor platform for esp32_l2_bridge.

Types:
  - connected_clients — merged source (AP sta_list + DHCP lease map),
    minus the ESP32's own STA MAC. Default format is human-readable
    ("hostname (ip), …"); set `format: json` for a JSON array suitable
    for HA templating.
  - sta_mac / ap_mac — diagnostic identity values, published once at
    setup. Static for the life of the device.
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
IdentityTextSensor = esp32_l2_bridge_ns.class_(
    "IdentityTextSensor", text_sensor.TextSensor, cg.Component
)
IdentityKind = esp32_l2_bridge_ns.enum("IdentityKind", is_class=True)

CONF_FORMAT = "format"
FORMATS = {
    "text": ConnectedClientsFormat.TEXT,
    "json": ConnectedClientsFormat.JSON,
}

IDENTITY_KINDS = {
    "sta_mac": IdentityKind.STA_MAC,
    "ap_mac": IdentityKind.AP_MAC,
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
        "sta_mac": text_sensor.text_sensor_schema(
            IdentityTextSensor,
            icon="mdi:network-outline",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        "ap_mac": text_sensor.text_sensor_schema(
            IdentityTextSensor,
            icon="mdi:access-point-network",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)


async def to_code(config):
    var = await text_sensor.new_text_sensor(config)
    await cg.register_component(var, config)
    type_ = config["type"]
    if type_ == "connected_clients":
        cg.add(var.set_format(FORMATS[config[CONF_FORMAT]]))
    else:
        cg.add(var.set_kind(IDENTITY_KINDS[type_]))
