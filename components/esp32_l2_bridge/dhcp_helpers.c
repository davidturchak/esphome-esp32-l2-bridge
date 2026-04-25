#include "dhcp_helpers.h"


#include <string.h>

/* BOOTP header layout:
 *   0  op(1) htype(1) hlen(1) hops(1)
 *   4  xid(4)
 *   8  secs(2) flags(2)
 *  12  ciaddr(4)
 *  16  yiaddr(4)
 *  20  siaddr(4)
 *  24  giaddr(4)
 *  28  chaddr(16)
 *  44  sname(64)
 * 108  file(128)
 * 236  magic cookie(4) = 0x63825363
 * 240  options...
 */
#define DHCP_MIN_LEN       240
#define DHCP_MAGIC_OFFSET  236
#define DHCP_OPTS_OFFSET   240
#define DHCP_MAGIC_COOKIE  0x63825363u

static bool find_option(const uint8_t *buf, size_t len, uint8_t tag,
                        const uint8_t **val_out, uint8_t *vlen_out) {
    if (len < DHCP_OPTS_OFFSET) return false;
    size_t i = DHCP_OPTS_OFFSET;
    while (i < len) {
        uint8_t t = buf[i++];
        if (t == DHCP_OPT_END) return false;
        if (t == DHCP_OPT_PAD) continue;
        if (i >= len) return false;
        uint8_t l = buf[i++];
        if (i + l > len) return false;
        if (t == tag) {
            if (val_out) *val_out = &buf[i];
            if (vlen_out) *vlen_out = l;
            return true;
        }
        i += l;
    }
    return false;
}

bool dhcp_parse(const uint8_t *buf, size_t len, dhcp_parsed_t *out) {
    if (!buf || !out || len < DHCP_MIN_LEN) return false;
    uint32_t magic = ((uint32_t)buf[DHCP_MAGIC_OFFSET] << 24) |
                     ((uint32_t)buf[DHCP_MAGIC_OFFSET+1] << 16) |
                     ((uint32_t)buf[DHCP_MAGIC_OFFSET+2] << 8)  |
                      (uint32_t)buf[DHCP_MAGIC_OFFSET+3];
    if (magic != DHCP_MAGIC_COOKIE) return false;

    memset(out, 0, sizeof(*out));
    out->op    = buf[0];
    out->htype = buf[1];
    out->hlen  = buf[2];
    out->xid = ((uint32_t)buf[4] << 24) | ((uint32_t)buf[5] << 16) |
               ((uint32_t)buf[6] << 8)  |  (uint32_t)buf[7];
    memcpy(out->chaddr, &buf[28], 6);
    memcpy(&out->yiaddr, &buf[16], 4);

    const uint8_t *v = NULL;
    uint8_t vl = 0;
    if (find_option(buf, len, DHCP_OPT_MSG_TYPE, &v, &vl) && vl >= 1) {
        out->msg_type = v[0];
    }
    out->has_client_id = find_option(buf, len, DHCP_OPT_CLIENT_ID, NULL, NULL);
    if (find_option(buf, len, DHCP_OPT_LEASE_TIME, &v, &vl) && vl >= 4) {
        out->lease_time = ((uint32_t)v[0] << 24) | ((uint32_t)v[1] << 16) |
                          ((uint32_t)v[2] << 8)  |  (uint32_t)v[3];
    }
    if (find_option(buf, len, DHCP_OPT_HOSTNAME, &v, &vl) && vl > 0) {
        uint8_t clen = vl < DHCP_HOSTNAME_MAX - 1 ? vl : DHCP_HOSTNAME_MAX - 1;
        memcpy(out->hostname, v, clen);
        out->hostname[clen] = '\0';
    }
    return true;
}

uint8_t dhcp_get_msg_type(const uint8_t *buf, size_t len) {
    const uint8_t *v = NULL; uint8_t vl = 0;
    if (find_option(buf, len, DHCP_OPT_MSG_TYPE, &v, &vl) && vl >= 1) return v[0];
    return 0;
}

bool dhcp_has_client_id(const uint8_t *buf, size_t len) {
    return find_option(buf, len, DHCP_OPT_CLIENT_ID, NULL, NULL);
}

bool dhcp_force_broadcast(uint8_t *buf, size_t len, uint8_t *udp_hdr) {
    if (!buf || len < DHCP_MIN_LEN) return false;
    if (buf[10] & 0x80) return false;  /* already set */

    uint16_t old_flags = ((uint16_t)buf[10] << 8) | buf[11];
    uint16_t new_flags = old_flags | 0x8000;
    buf[10] = (uint8_t)(new_flags >> 8);
    /* buf[11] is unchanged */

    /* The flags field at BOOTP offset 10 sits at UDP-payload offset 10,
     * which is at UDP-datagram offset 18 — aligned to a 16-bit word, so
     * RFC 1624 incremental update applies cleanly. UDP layout:
     *   0-1 sport  2-3 dport  4-5 length  6-7 checksum */
    if (udp_hdr) {
        uint16_t old_chk = ((uint16_t)udp_hdr[6] << 8) | udp_hdr[7];
        if (old_chk != 0) {
            /* HC' = ~(~HC + ~m + m') */
            uint32_t sum = (uint16_t)~old_chk;
            sum += (uint16_t)~old_flags;
            sum += new_flags;
            while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16);
            uint16_t new_chk = (uint16_t)~sum;
            udp_hdr[6] = (uint8_t)(new_chk >> 8);
            udp_hdr[7] = (uint8_t)(new_chk & 0xff);
        }
    }
    return true;
}

