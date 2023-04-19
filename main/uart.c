/*
 * This file is part of the ESP32-XBee distribution (https://github.com/nebkat/esp32-xbee).
 * Copyright (c) 2019 Nebojsa Cvetkovic.
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

#include <driver/uart.h>
#include <driver/gpio.h>
#include <esp_event.h>
#include <esp_log.h>
#include <string.h>
#include <protocol/nmea.h>
#include <stream_stats.h>

#include "uart.h"
#include "config.h"
#include "interface/socket_server.h"
#include "tasks.h"

static const char *TAG = "UART";

ESP_EVENT_DEFINE_BASE(UART_EVENT_READ);
ESP_EVENT_DEFINE_BASE(UART_EVENT_WRITE);

void uart_register_read_handler(esp_event_handler_t event_handler) {
    ESP_ERROR_CHECK(esp_event_handler_register(UART_EVENT_READ, ESP_EVENT_ANY_ID, event_handler, NULL));
}

void uart_register_read_handler_with_arg(esp_event_handler_t event_handler, void *arg) {
    ESP_ERROR_CHECK(esp_event_handler_register(UART_EVENT_READ, ESP_EVENT_ANY_ID, event_handler, arg));
}

void uart_unregister_read_handler(esp_event_handler_t event_handler) {
    ESP_ERROR_CHECK(esp_event_handler_unregister(UART_EVENT_READ, ESP_EVENT_ANY_ID, event_handler));
}

void uart_register_write_handler(esp_event_handler_t event_handler) {
    ESP_ERROR_CHECK(esp_event_handler_register(UART_EVENT_WRITE, ESP_EVENT_ANY_ID, event_handler, NULL));
}

void uart_unregister_write_handler(esp_event_handler_t event_handler) {
    ESP_ERROR_CHECK(esp_event_handler_unregister(UART_EVENT_WRITE, ESP_EVENT_ANY_ID, event_handler));
}

static uart_port_t log_uart_port = -1;
static uart_port_t gnss_uart_port = -1;

static stream_stats_handle_t stream_stats;

static void uart_task(void *ctx);

typedef struct {
    const char *flow_ctrl_rts;
    const char *flow_ctrl_cts;
    const char *baud_rate;
    const char *data_bits;
    const char *parity;
    const char *stop_bits;
    const char *tx_pin;
    const char *rx_pin;
    const char *rts_pin;
    const char *cts_pin;
} uart_port_conf_items_t;

static const uart_port_conf_items_t port_conf_items[] = {
    [0] = {
	.flow_ctrl_rts = KEY_CONFIG_UART0_FLOW_CTRL_RTS,
	.flow_ctrl_cts = KEY_CONFIG_UART0_FLOW_CTRL_CTS,
	.baud_rate = KEY_CONFIG_UART0_BAUD_RATE,
	.data_bits = KEY_CONFIG_UART0_DATA_BITS,
	.parity = KEY_CONFIG_UART0_PARITY,
	.stop_bits = KEY_CONFIG_UART0_STOP_BITS,
	.tx_pin = KEY_CONFIG_UART0_TX_PIN,
	.rx_pin = KEY_CONFIG_UART0_RX_PIN,
	.rts_pin = KEY_CONFIG_UART0_RTS_PIN,
	.cts_pin = KEY_CONFIG_UART0_CTS_PIN,
    },
    [1] = {
	.flow_ctrl_rts = KEY_CONFIG_UART1_FLOW_CTRL_RTS,
	.flow_ctrl_cts = KEY_CONFIG_UART1_FLOW_CTRL_CTS,
	.baud_rate = KEY_CONFIG_UART1_BAUD_RATE,
	.data_bits = KEY_CONFIG_UART1_DATA_BITS,
	.parity = KEY_CONFIG_UART1_PARITY,
	.stop_bits = KEY_CONFIG_UART1_STOP_BITS,
	.tx_pin = KEY_CONFIG_UART1_TX_PIN,
	.rx_pin = KEY_CONFIG_UART1_RX_PIN,
	.rts_pin = KEY_CONFIG_UART1_RTS_PIN,
	.cts_pin = KEY_CONFIG_UART1_CTS_PIN,
    },
    [2] = {
	.flow_ctrl_rts = KEY_CONFIG_UART2_FLOW_CTRL_RTS,
	.flow_ctrl_cts = KEY_CONFIG_UART2_FLOW_CTRL_CTS,
	.baud_rate = KEY_CONFIG_UART2_BAUD_RATE,
	.data_bits = KEY_CONFIG_UART2_DATA_BITS,
	.parity = KEY_CONFIG_UART2_PARITY,
	.stop_bits = KEY_CONFIG_UART2_STOP_BITS,
	.tx_pin = KEY_CONFIG_UART2_TX_PIN,
	.rx_pin = KEY_CONFIG_UART2_RX_PIN,
	.rts_pin = KEY_CONFIG_UART2_RTS_PIN,
	.cts_pin = KEY_CONFIG_UART2_CTS_PIN,
    },
};

bool uart_initialized[3] = {};

void uart_init_port(uart_port_t port) {
    uart_hw_flowcontrol_t flow_control;
    const uart_port_conf_items_t *pci = port_conf_items + port;

    if (uart_initialized[port])
	return;

    bool flow_ctrl_rts = config_get_bool1(CONF_ITEM(pci->flow_ctrl_rts));
    bool flow_ctrl_cts = config_get_bool1(CONF_ITEM(pci->flow_ctrl_cts));
    int rts_pin = flow_ctrl_rts ? config_get_i8(CONF_ITEM(pci->rts_pin)) : UART_PIN_NO_CHANGE;
    int cts_pin = flow_ctrl_cts ? config_get_i8(CONF_ITEM(pci->cts_pin)) : UART_PIN_NO_CHANGE;

    if (flow_ctrl_rts && flow_ctrl_cts) {
	flow_control = UART_HW_FLOWCTRL_CTS_RTS;
    } else if (flow_ctrl_rts) {
	flow_control = UART_HW_FLOWCTRL_RTS;
    } else if (flow_ctrl_cts) {
	flow_control = UART_HW_FLOWCTRL_CTS;
    } else {
	flow_control = UART_HW_FLOWCTRL_DISABLE;
    }

    uart_config_t uart_config = {
	.baud_rate = config_get_u32(CONF_ITEM(pci->baud_rate)),
	.data_bits = config_get_u8(CONF_ITEM(pci->data_bits)),
	.parity = config_get_u8(CONF_ITEM(pci->parity)),
	.stop_bits = config_get_u8(CONF_ITEM(pci->stop_bits)),
	.flow_ctrl = flow_control,
    };

    ESP_ERROR_CHECK(uart_param_config(port, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(
	port,
	config_get_i8(CONF_ITEM(pci->tx_pin)),
	config_get_i8(CONF_ITEM(pci->rx_pin)),
	rts_pin,
	cts_pin
    ));
    ESP_ERROR_CHECK(uart_driver_install(port, UART_BUFFER_SIZE, UART_BUFFER_SIZE, 0, NULL, 0));

    uart_initialized[port] = 1;
    ESP_LOGI(TAG, "UART%u initialized", port);
}

void uart_init() {
    log_uart_port = config_get_i8(CONF_ITEM(KEY_CONFIG_LOG_UART_NUM));
    gnss_uart_port = config_get_i8(CONF_ITEM(KEY_CONFIG_GNSS_UART_NUM));

    uart_init_port(log_uart_port);
    uart_init_port(gnss_uart_port);

    stream_stats = stream_stats_new("uart");

    xTaskCreate(uart_task, "uart_task", 8192, NULL, TASK_PRIORITY_UART, NULL);
}

static void uart_task(void *ctx) {
    uint8_t buffer[UART_BUFFER_SIZE];

    while (true) {
        int32_t len = uart_read_bytes(gnss_uart_port, buffer, sizeof(buffer), pdMS_TO_TICKS(50));
        if (len < 0) {
            ESP_LOGE(TAG, "Error reading from UART");
        } else if (len == 0) {
            continue;
        }

        stream_stats_increment(stream_stats, len, 0);

        esp_event_post(UART_EVENT_READ, len, &buffer, len, portMAX_DELAY);
    }
}

void uart_inject(void *buf, size_t len) {
    esp_event_post(UART_EVENT_READ, len,  buf, len, portMAX_DELAY);
}

int uart_log(char *buf, size_t len) {
    if (log_uart_port < 0) return 0;
    if (len == 0) return 0;

    return uart_write_bytes(log_uart_port, buf, len);
}

int uart_nmea(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char *nmea;
    nmea_vasprintf(&nmea, fmt, args);
    int l = uart_write(nmea, strlen(nmea));
    free(nmea);

    va_end(args);

    return l;
}

int uart_write(char *buf, size_t len) {
    if (gnss_uart_port < 0) return 0;
    if (len == 0) return 0;

    int written = uart_write_bytes(gnss_uart_port, buf, len);
    if (written < 0) return written;

    stream_stats_increment(stream_stats, 0, len);

    esp_event_post(UART_EVENT_WRITE, len, buf, len, portMAX_DELAY);

    return written;
}
