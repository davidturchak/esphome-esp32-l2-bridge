"""Button platform for esp32_l2_bridge.

Currently only one type — `fdb_clear`, which empties the FDB (forwarding
database) on press. Useful when MACs move behind upstream NAT and you
don't want to wait for natural FDB expiry.
"""

import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_DIAGNOSTIC

from . import esp32_l2_bridge_ns

DEPENDENCIES = ["esp32_l2_bridge"]

FdbClearButton = esp32_l2_bridge_ns.class_(
    "FdbClearButton", button.Button, cg.Component
)


CONFIG_SCHEMA = cv.typed_schema(
    {
        "fdb_clear": button.button_schema(
            FdbClearButton,
            icon="mdi:database-remove",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ).extend(cv.COMPONENT_SCHEMA),
    }
)


async def to_code(config):
    var = await button.new_button(config)
    await cg.register_component(var, config)
