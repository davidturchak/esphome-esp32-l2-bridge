"""ESPHome external component: esp32_l2_bridge.

Vendors the L2-repeater data plane (FDB, DHCP snoop, MAC translation) from
the upstream esp32_nat_router project's esp32_wifi_repeater branch and
exposes it as a native ESPHome component.

Pairs with `wifi_bridge:` (which provides the simultaneous AP+STA WiFi
mode that the bridge plumbs into).
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@dturchak"]
DEPENDENCIES = ["esp32", "wifi_bridge"]

# Visible to platform shims (sensor.py / text_sensor.py / button.py).
CONF_L2_BRIDGE_ID = "l2_bridge_id"

CONF_FDB_SIZE = "fdb_size"
CONF_FDB_DEFAULT_TTL = "fdb_default_ttl"
CONF_XID_MAP_SIZE = "xid_map_size"
CONF_XID_TTL = "xid_ttl"
CONF_DHCP_SNOOP = "dhcp_snoop"

esp32_l2_bridge_ns = cg.esphome_ns.namespace("esp32_l2_bridge")
L2Bridge = esp32_l2_bridge_ns.class_("L2Bridge", cg.Component)

# FDB and XID map sizes back static arrays — they cap RAM cost, so they
# stay compile-time. Bounds match what the source firmware was tested with.
CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(L2Bridge),
        cv.Optional(CONF_FDB_SIZE, default=32): cv.int_range(min=4, max=128),
        cv.Optional(CONF_FDB_DEFAULT_TTL, default="600s"): cv.All(
            cv.positive_time_period_seconds,
            cv.Range(min=cv.TimePeriod(seconds=10), max=cv.TimePeriod(hours=24)),
        ),
        cv.Optional(CONF_XID_MAP_SIZE, default=16): cv.int_range(min=4, max=64),
        cv.Optional(CONF_XID_TTL, default="30s"): cv.All(
            cv.positive_time_period_seconds,
            cv.Range(min=cv.TimePeriod(seconds=5), max=cv.TimePeriod(minutes=10)),
        ),
        cv.Optional(CONF_DHCP_SNOOP, default=True): cv.boolean,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    # Compile-time sizing for the static arrays in fdb.c / dhcp_xid_map.c.
    # cg.add_build_flag (rather than add_define) feeds -D into both C and
    # C++ translation units; add_define only reaches ESPHome C++ via
    # defines.h, which the vendored .c files don't include.
    cg.add_build_flag(f"-DREPEATER_FDB_SIZE={config[CONF_FDB_SIZE]}")
    cg.add_build_flag(f"-DREPEATER_XID_MAP_SIZE={config[CONF_XID_MAP_SIZE]}")
    cg.add_build_flag(
        f"-DREPEATER_XID_TTL_S={int(config[CONF_XID_TTL].total_seconds)}"
    )

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_fdb_default_ttl(int(config[CONF_FDB_DEFAULT_TTL].total_seconds)))
    cg.add(var.set_dhcp_snoop(config[CONF_DHCP_SNOOP]))
