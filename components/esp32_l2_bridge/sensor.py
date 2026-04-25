"""Sensor platform for esp32_l2_bridge.

Each entry produces one Sensor for a single metric, selected by `type:`.
The metric is read from the C data plane on every poll interval.
"""

import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_SIGNAL_STRENGTH,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_DECIBEL_MILLIWATT,
    UNIT_EMPTY,
)

from . import esp32_l2_bridge_ns

DEPENDENCIES = ["esp32_l2_bridge"]

L2BridgeSensor = esp32_l2_bridge_ns.class_(
    "L2BridgeSensor", sensor.Sensor, cg.PollingComponent
)
L2BridgeSensorType = esp32_l2_bridge_ns.enum("L2BridgeSensorType", is_class=True)

TYPE_ENUM = {
    "fdb_occupancy": L2BridgeSensorType.FDB_OCCUPANCY,
    "dhcp_lease_count": L2BridgeSensorType.DHCP_LEASE_COUNT,
    "sta_rssi": L2BridgeSensorType.STA_RSSI,
}


def _schema(**kwargs):
    return (
        sensor.sensor_schema(L2BridgeSensor, **kwargs)
        .extend(cv.polling_component_schema("15s"))
    )


CONFIG_SCHEMA = cv.typed_schema(
    {
        "fdb_occupancy": _schema(
            unit_of_measurement=UNIT_EMPTY,
            icon="mdi:lan",
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        "dhcp_lease_count": _schema(
            unit_of_measurement=UNIT_EMPTY,
            icon="mdi:ip-network",
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        "sta_rssi": _schema(
            unit_of_measurement=UNIT_DECIBEL_MILLIWATT,
            device_class=DEVICE_CLASS_SIGNAL_STRENGTH,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    cg.add(var.set_type(TYPE_ENUM[config["type"]]))
