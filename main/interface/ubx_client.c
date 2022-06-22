/*
 * This file is part of the ESP32-XBee distribution (https://github.com/nebkat/esp32-xbee).
 * Copyright (c) 2022 Damjan Marion
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

#include <sys/param.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <uart.h>
#include <util.h>
#include <status_led.h>
#include <wifi.h>
#include <esp_log.h>
#include <sys/socket.h>
#include "interface/ubx_client.h"

#include <config.h>
#include <retry.h>
#include <stream_stats.h>
#include <tasks.h>

static const char *TAG = "UBX_CLIENT";
ubx_ctx_t *ubx_ctx = 0;

static stream_stats_handle_t stream_stats = NULL;

static void ubx_client_uart_handler(void *handler_args, esp_event_base_t base, int32_t length, void *buffer)
{
    stream_stats_increment(stream_stats, 0, length);
    ubx_parse_data ((ubx_ctx_t *) handler_args, buffer, length);
}

static void ubx_msg_poll(ubx_msg_index_t msg_index)
{
    uint8_t class = ubx_msg_class_by_index[msg_index];
    uint8_t id = ubx_msg_id_by_index[msg_index];

    struct {
        ubx_msg_hdr_t hdr;
        ubx_msg_cksum_t cksum;
    } data = {
        .hdr.sync1 = UBX_SYNC1,
        .hdr.sync2 = UBX_SYNC2,
        .hdr.class = class,
        .hdr.id = id,
        .cksum.ok_a = (class + id),
        .cksum.ok_b = (4 * class + 3 * id)
    };
    uart_write ((char *) &data, sizeof (data));
}

static void ubx_cfg_msg(ubx_msg_index_t msg_index, uint8_t rate)
{
    uint8_t class = ubx_msg_class_by_index[msg_index];
    uint8_t id = ubx_msg_id_by_index[msg_index];

    struct {
        ubx_msg_hdr_t hdr;
        uint8_t class;
        uint8_t id;
        uint8_t rate;
        ubx_msg_cksum_t cksum;
    } data = {
        .hdr.sync1 = UBX_SYNC1,
        .hdr.sync2 = UBX_SYNC2,
        .hdr.class = 6,
        .hdr.id = 1,
        .hdr.len = 3,
        .class = class,
        .id = id,
        .rate = rate
    };
    ubx_msg_calc_checksum ((ubx_msg_frame_t *) &data, 0);
    uart_write ((char *) &data, sizeof (data));
}

static void ubx_client_task(void *unused)
{
    ubx_ctx_t *ctx = ubx_ctx_alloc ();
    uart_register_read_handler_with_arg (ubx_client_uart_handler, ctx);

    stream_stats = stream_stats_new("ubx_client");

    for (int i = 0; i < 10 && ctx->protVerMajor == 0; i++) {
        if ((i % 3) == 0) {
            ubx_msg_poll (UBX_MON_VER);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (ctx->protVerMajor) {
        ESP_LOGI(TAG, "found %s fwVer '%s' protVer %u.%u",
                 ctx->mod, ctx->fwVer, ctx->protVerMajor, ctx->protVerMinor);
    } else {
        ESP_LOGW(TAG, "u-blox receiver not found");
        goto done;
    }

    ubx_cfg_msg(UBX_NMEA_GGA, 1);
    ubx_cfg_msg(UBX_NAV_PVT, 1);
    ubx_cfg_msg(UBX_NAV_STATUS, 1);
    ubx_cfg_msg(UBX_NAV_HPPOSLLH, 1);

    ubx_ctx = ctx;

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(500));
        ubx_msg_poll(UBX_NAV_SAT);
        ubx_msg_poll(UBX_NAV_SIG);
    }

done:
    ubx_ctx = 0;
    uart_unregister_read_handler(ubx_client_uart_handler);
    vTaskDelete(NULL);
    ubx_ctx_free (ctx);
}

void ubx_client_init()
{
    xTaskCreate(ubx_client_task, "ubx_client_task", 4096, NULL, TASK_PRIORITY_INTERFACE, NULL);
}
