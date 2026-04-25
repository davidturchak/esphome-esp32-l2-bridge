#pragma once

#include <stdint.h>
#include <stdbool.h>

/* These are sizes for static arrays — RAM-cost predictable, so they stay
 * compile-time. YAML can override via cg.add_define() in __init__.py. */
#ifndef REPEATER_FDB_SIZE
#define REPEATER_FDB_SIZE           32
#endif

#ifndef REPEATER_FDB_DEFAULT_TTL_S
#define REPEATER_FDB_DEFAULT_TTL_S  600
#endif

#ifndef REPEATER_XID_MAP_SIZE
#define REPEATER_XID_MAP_SIZE       16
#endif

#ifndef REPEATER_XID_TTL_S
#define REPEATER_XID_TTL_S          30
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Runtime override for the data-plane learning fallback TTL (seconds).
 * DHCP-ACK snooping always supplies its own TTL from the lease; this
 * controls the TTL applied when learning from generic IPv4 traffic.
 * Default = REPEATER_FDB_DEFAULT_TTL_S; 0 leaves the default in place. */
void repeater_set_learn_ttl(uint32_t ttl_seconds);
uint32_t repeater_get_learn_ttl(void);

/* Runtime toggle for DHCP snooping. When false, snoop_dhcp_client and
 * snoop_dhcp_server_reply become no-ops — useful as a kill-switch for
 * environments where the bridge should be transparent. Default true. */
void repeater_set_dhcp_snoop(bool enabled);
bool repeater_get_dhcp_snoop(void);

#ifdef __cplusplus
}
#endif
