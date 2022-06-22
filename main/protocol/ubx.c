/*
 * This file is part of the ESP32-XBee distribution (https://github.com/nebkat/esp32-xbee).
 * Copyright (c) 2022 Damjan Marion.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <esp_log.h>

#include "protocol/ubx.h"

static const char *TAG = "UBX";

static ubx_msg_index_t ubx_msg_index_from_class_and_id(uint8_t class, uint8_t id)
{
    for (int i = 0; i < UBX_N_MSG_INDICES; i++) {
        if ((class == ubx_msg_class_by_index[i]) && (id == ubx_msg_id_by_index[i])) {
            return i;
        }
    }
    return UBX_UNKNOWN_MSG;
}

static void ubx_msg_handler(ubx_ctx_t *ctx, ubx_msg_frame_t *m)
{
    ubx_msg_cksum_t ce, *cp = (ubx_msg_cksum_t *) &m->payload[m->hdr.len];
    ubx_msg_calc_checksum (m, &ce);
    ubx_msg_index_t msg_index;

    if (cp->ok_a != ce.ok_a || cp->ok_b != ce.ok_b) {
        ESP_LOGI(TAG, "bad checksum - expected %02x %02x, provided %02x %02x",
                 ce.ok_a, ce.ok_b, cp->ok_a, cp->ok_b);
        return;
    }

    msg_index = ubx_msg_index_from_class_and_id (m->hdr.class, m->hdr.id);

    if (msg_index == UBX_UNKNOWN_MSG) {
        ESP_LOGD(TAG, "unknown message received (Class 0x%x ID 0x%x Len %u)",
                 m->hdr.class, m->hdr.id, m->hdr.len);
        return;
    }

    uint16_t len = m->hdr.len;
    void *p = ctx->msg[msg_index];

    if (ubx_mem_size (p) < len || ubx_mem_size (p) > 2 * len) {
        ubx_mem_free (p);
        ctx->msg[msg_index] = p = ubx_mem_alloc(len);
    }

    memcpy (p, m->payload, len);

    if (msg_index == UBX_MON_VER) {
        uint8_t n = (m->hdr.len - sizeof (ubx_mon_ver_data_t)) / sizeof (ubx_mon_ver_extension_data_t);
        ubx_mon_ver_data_t *d = (ubx_mon_ver_data_t *) ctx->msg[msg_index];
        ESP_LOGI(TAG, "swVersion %s hwVersion %s", d->swVersion, d->hwVersion);
        for (uint32_t i = 0; i < n; i++) {
            char *s = d->extensions[i].extension;
            if (strncmp (s, "PROTVER=", 8) == 0) {
                sscanf (s + 8, "%hhu.%hhu", &ctx->protVerMajor, &ctx->protVerMinor);
            } else if (strncmp (s, "MOD=", 4) == 0) {
                ctx->mod = s + 4;
            } else if (strncmp (s, "FWVER=", 6) == 0) {
                ctx->fwVer = s + 6;
            }
        }
    }
}

void ubx_parse_data(ubx_ctx_t *ctx, uint8_t *data, uint32_t length)
{
    if (ctx->frame_bytes > 1 || (ctx->frame_bytes == 1 && data[0] == UBX_SYNC2)) {
        while (ctx->frame_bytes < 6) {
            ctx->frame.bytes[ctx->frame_bytes++] = data++[0];
            if  (--length == 0) {
                return;
            }
        }

        if (ctx->frame.hdr.len + 8 <= sizeof (ubx_msg_frame_t)) {
            while (ctx->frame_bytes < ctx->frame.hdr.len + 8) {
                ctx->frame.bytes[ctx->frame_bytes++] = data++[0];
                if  (--length == 0) {
                    return;
                }
            }
            ubx_msg_handler (ctx, &ctx->frame);
        }
    }

    while (length > 1) {
        ubx_msg_frame_t *m = (ubx_msg_frame_t *) (data);
        uint32_t msg_len;

        if (m->hdr.sync1 != UBX_SYNC1 || m->hdr.sync2 != UBX_SYNC2) {
            length--;
            data++;
            continue;
        }

        if (length < 8) {
            memcpy (&ctx->frame, data, length);
            ctx->frame_bytes = length;
            return;
        }

        msg_len = m->hdr.len + 8;

        if (msg_len > sizeof (ubx_msg_frame_t)) {
            length--;
            data++;
            continue;
        }

        if (msg_len > length) {
            memcpy (&ctx->frame, data, length);
            ctx->frame_bytes = length;
            return;
        }

        ubx_msg_handler (ctx, m);
        length -= msg_len;
        data += msg_len;
    }

    ctx->frame_bytes = length > 0 && data[0] == 0xb5 ? 1 : 0;
}


ubx_ctx_t *ubx_ctx_alloc()
{
    ubx_ctx_t *ctx = ubx_mem_alloc_zero(sizeof (ubx_ctx_t));
    return ctx;
}

void ubx_ctx_free(ubx_ctx_t *ctx)
{
    for (int i = 0; i < UBX_N_MSG_INDICES; i++)
        if (ctx->msg[i]) {
            ubx_mem_free(ctx->msg[i]);
        }
    ubx_mem_free(ctx);
}
