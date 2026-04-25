#pragma once

#include "esp_netif.h"

#ifdef __cplusplus
extern "C" {
#endif

// Install lwIP-netif input/linkoutput hooks on the AP netif.
// Idempotent: safe to call multiple times. Must be called only after
// WIFI_EVENT_AP_START so the underlying lwIP netif exists.
void bridge_netif_hooks_init_ap(esp_netif_t *ap);

// Install lwIP-netif input/linkoutput hooks plus the STA->AP output redirect
// on the STA netif. Idempotent. Must be called only after IP_EVENT_STA_GOT_IP
// so the netif's hwaddr / ip4_addr are populated.
void bridge_netif_hooks_init_sta(esp_netif_t *sta);

// Returns true if hooks are still installed; logs a warning otherwise.
#include <stdbool.h>
bool bridge_netif_hooks_check(void);

#ifdef __cplusplus
}
#endif
