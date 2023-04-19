#ifndef ESP32_XBEE_UBX_CLIENT_H
#define ESP32_XBEE_UBX_CLIENT_H

#include <stdint.h>
#include <esp_event_base.h>
#include <protocol/ubx.h>

void ubx_client_init();
extern ubx_ctx_t *ubx_ctx;

#endif //ESP32_XBEE_UBX_CLIENT_H
