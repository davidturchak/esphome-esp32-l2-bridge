/* Stripped lwIP-netif hook installer for the ESPHome L2-bridge port.
 *
 * Only the bridge plumbing — no ACL, PCAP, LED, client_stats, MSS clamp,
 * TTL override, or PMTU. Those features will land later as separate
 * ESPHome components / sub-features and may layer their own hooks.
 *
 * The mechanism: ESP-IDF exposes esp_netif_t handles, but the lwIP
 * pointers we need (input, linkoutput, output) live on the underlying
 * struct netif*. esp_netif_get_netif_impl() is internal API but is the
 * only way to reach them. Originals are saved and chained.
 */

#include "netif_hooks.h"

#include <string.h>
#include "esp_log.h"
#include "esp_attr.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"
#include "fdb.h"
#include "repeater_forward.h"

static const char *TAG = "bridge_hooks";

// esp_netif internal: returns the underlying struct netif*. Declared here
// because the public esp_netif.h does not expose it.
extern struct netif *esp_netif_get_netif_impl(esp_netif_t *esp_netif);

static netif_input_fn       orig_ap_input        = NULL;
static netif_linkoutput_fn  orig_ap_linkoutput   = NULL;
static struct netif        *ap_netif             = NULL;

static netif_input_fn       orig_sta_input       = NULL;
static netif_linkoutput_fn  orig_sta_linkoutput  = NULL;
static netif_output_fn      orig_sta_output      = NULL;
static struct netif        *sta_netif            = NULL;

/* ---- AP-side hooks ---- */

static IRAM_ATTR err_t ap_input_hook(struct pbuf *p, struct netif *netif) {
    if (repeater_ap_rx_handle(p, netif)) {
        return ERR_OK;
    }
    return orig_ap_input ? orig_ap_input(p, netif) : ERR_VAL;
}

static IRAM_ATTR err_t ap_linkoutput_hook(struct netif *netif, struct pbuf *p) {
    return orig_ap_linkoutput ? orig_ap_linkoutput(netif, p) : ERR_IF;
}

/* ---- STA-side hooks ---- */

static IRAM_ATTR err_t sta_input_hook(struct pbuf *p, struct netif *netif) {
    if (repeater_sta_rx_handle(p, netif)) {
        return ERR_OK;
    }
    return orig_sta_input ? orig_sta_input(p, netif) : ERR_VAL;
}

static IRAM_ATTR err_t sta_linkoutput_hook(struct netif *netif, struct pbuf *p) {
    return orig_sta_linkoutput ? orig_sta_linkoutput(netif, p) : ERR_IF;
}

/* STA TX redirect: when lwIP wants to send out STA toward an IP that
 * actually belongs to an AP-side client, bypass ARP and emit the frame
 * directly on the AP netif. Without this, services running on the ESP32
 * itself (e.g. ESPHome API, web_server) cannot reach AP clients. */
static IRAM_ATTR err_t sta_output_redirect(struct netif *netif,
                                           struct pbuf *p,
                                           const ip4_addr_t *ipaddr) {
    if (ap_netif && ipaddr) {
        uint8_t client_mac[6];
        if (fdb_lookup_by_ip(ipaddr->addr, client_mac)) {
            if (pbuf_add_header(p, 14) == ERR_OK) {
                uint8_t *eth = (uint8_t *)p->payload;
                memcpy(eth,     client_mac,       6);
                memcpy(eth + 6, ap_netif->hwaddr, 6);
                eth[12] = 0x08; eth[13] = 0x00;
                err_t r = ap_netif->linkoutput(ap_netif, p);
                pbuf_remove_header(p, 14);
                return r;
            }
        }
    }
    return orig_sta_output ? orig_sta_output(netif, p, ipaddr) : ERR_IF;
}

/* ---- Public installers ---- */

void bridge_netif_hooks_init_ap(esp_netif_t *ap) {
    if (!ap || orig_ap_input != NULL) return;
    struct netif *nif = esp_netif_get_netif_impl(ap);
    if (!nif) {
        ESP_LOGW(TAG, "AP netif impl not yet available");
        return;
    }
    ap_netif = nif;
    orig_ap_input        = nif->input;
    orig_ap_linkoutput   = nif->linkoutput;
    nif->input        = ap_input_hook;
    nif->linkoutput   = ap_linkoutput_hook;
    repeater_forward_set_netifs(nif, NULL);
    ESP_LOGI(TAG, "AP hooks installed (hwaddr %02x:%02x:%02x:%02x:%02x:%02x)",
             nif->hwaddr[0], nif->hwaddr[1], nif->hwaddr[2],
             nif->hwaddr[3], nif->hwaddr[4], nif->hwaddr[5]);
}

void bridge_netif_hooks_init_sta(esp_netif_t *sta) {
    if (!sta || orig_sta_input != NULL) return;
    struct netif *nif = esp_netif_get_netif_impl(sta);
    if (!nif) {
        ESP_LOGW(TAG, "STA netif impl not yet available");
        return;
    }
    sta_netif = nif;
    orig_sta_input       = nif->input;
    orig_sta_linkoutput  = nif->linkoutput;
    orig_sta_output      = nif->output;
    nif->input       = sta_input_hook;
    nif->linkoutput  = sta_linkoutput_hook;
    nif->output      = sta_output_redirect;
    repeater_forward_set_netifs(NULL, nif);
    ESP_LOGI(TAG, "STA hooks installed (hwaddr %02x:%02x:%02x:%02x:%02x:%02x)",
             nif->hwaddr[0], nif->hwaddr[1], nif->hwaddr[2],
             nif->hwaddr[3], nif->hwaddr[4], nif->hwaddr[5]);
}

// Periodic check: are our hooks still installed on the netifs?
// If something inside ESP-IDF re-attaches the netif (observed on the AP side
// in IDF v5.5: input pointer rotates after STA connects), reinstall and
// rebind the chain to the new "original" pointer. Linkoutput is sticky and
// rarely needs reinstall, but we cover it for symmetry.
bool bridge_netif_hooks_check(void) {
    bool ap_ok  = true, sta_ok = true;
    if (ap_netif) {
        if (ap_netif->input != ap_input_hook) {
            ESP_LOGW(TAG, "AP input clobbered (was %p) — reinstalling",
                     (void *)ap_netif->input);
            orig_ap_input = ap_netif->input;
            ap_netif->input = ap_input_hook;
            ap_ok = false;
        }
        if (ap_netif->linkoutput != ap_linkoutput_hook) {
            orig_ap_linkoutput = ap_netif->linkoutput;
            ap_netif->linkoutput = ap_linkoutput_hook;
            ap_ok = false;
        }
    }
    if (sta_netif) {
        if (sta_netif->input != sta_input_hook) {
            ESP_LOGW(TAG, "STA input clobbered (was %p) — reinstalling",
                     (void *)sta_netif->input);
            orig_sta_input = sta_netif->input;
            sta_netif->input = sta_input_hook;
            sta_ok = false;
        }
        if (sta_netif->linkoutput != sta_linkoutput_hook) {
            orig_sta_linkoutput = sta_netif->linkoutput;
            sta_netif->linkoutput = sta_linkoutput_hook;
            sta_ok = false;
        }
        if (sta_netif->output != sta_output_redirect) {
            orig_sta_output = sta_netif->output;
            sta_netif->output = sta_output_redirect;
            sta_ok = false;
        }
    }
    return ap_ok && sta_ok;
}
