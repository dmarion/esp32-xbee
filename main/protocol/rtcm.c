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
#include <cJSON.h>

#include "protocol/rtcm.h"

static const char *TAG = "RTCM";
#define _(s, f, ...) ESP_LOGW(TAG, "%-50s: " f, s, __VA_ARGS__)

static void
rtcm_msg (rtcm_ctx_t *ctx, uint8_t *data, uint16_t msg_len)
{
    uint16_t msg_num;
    rtcm_msg_parser_t p;
    char buf[32];
    double x, y, z;

    rtcm_msg_parser_init (&p, data);
    msg_num = rtcm_msg_parser_get_uint12 (&p);

    ESP_LOGI(TAG, "msg %u len %u", msg_num, msg_len);

    if (msg_num == 1005) {
        memcpy (ctx->mt1005, data, msg_len);
    } else if (msg_num == 1033) {
        memcpy (ctx->mt1033, data, msg_len);
    } else if (msg_num == 1021) {
        memcpy (ctx->mt1021, data, msg_len);
#if 0
        rtcm_msg_parser_get_string(&p, 5, buf);
        _("Source-Name", "%s", buf);
        rtcm_msg_parser_get_string(&p, 5, buf);
        _("Target-Name", "%s", buf);
        _("System Identification Number", "%u", rtcm_msg_parser_get_uint8(&p));
        _("Utilized Transformation Message Indicator", "0x%x", rtcm_msg_parser_get_uint10 (&p));
        _("Plate number", "%u", rtcm_msg_parser_get_uint5 (&p));
        _("Computation Indicator", "%u", rtcm_msg_parser_get_uint4 (&p));
        _("Height Indicator", "%u", rtcm_msg_parser_get_uint2 (&p));

        _("[Lat, Lon] of Origin, Area of Validity", "[%.5fdeg, %.5fdeg]",
          2.0 * rtcm_msg_parser_get_int19 (&p) / 3600,
          2.0 * rtcm_msg_parser_get_int20 (&p) / 3600);

        _("[N/S, E/W]  Extension, Area of Validity", "[%.5fdeg, %.5fdeg]",
          2.0 * rtcm_msg_parser_get_uint14 (&p) / 3600,
          2.0 * rtcm_msg_parser_get_uint14 (&p) / 3600);

        x = 0.001 * rtcm_msg_parser_get_int23 (&p);
        y = 0.001 * rtcm_msg_parser_get_int23 (&p);
        z = 0.001 * rtcm_msg_parser_get_int23 (&p);
        _("Translation in [X,Y,Z]-direction", "[%.3fm, %.3fm, %.3fm]", x, y, z);

        x = 0.00002 * rtcm_msg_parser_get_int32 (&p) / 3600;
        y = 0.00002 * rtcm_msg_parser_get_int32 (&p) / 3600;
        z = 0.00002 * rtcm_msg_parser_get_int32 (&p) / 3600;
        _("Rotation Around the [X, Y, Z]-axis", "[%.9f deg, %.9fdeg, %.9fdeg]", x, y, z);

        _("Scale Correction", "%.5f PPM", 0.00001 * rtcm_msg_parser_get_int25 (&p));

        _("Semi-major Axis of Source System Ellipsoid", "%.3fm", 0.001 * rtcm_msg_parser_get_uint24 (&p));
        _("Semi-minor Axis of Source System Ellipsoid", "%.3fm", 0.001 * rtcm_msg_parser_get_uint25 (&p));
        _("Semi-major Axis of Target System Ellipsoid", "%.3fm", 0.001 * rtcm_msg_parser_get_uint24 (&p));
        _("Semi-minor Axis of Target System Ellipsoid", "%.3fm", 0.001 * rtcm_msg_parser_get_uint25 (&p));

        _("Horizontal Helmert/Molodenski Quality Indicator", "%u", rtcm_msg_parser_get_uint3 (&p));
        _("Vertical Helmert/Molodenski Quality Indicator", "%u", rtcm_msg_parser_get_uint3 (&p));
#endif
    } else if (msg_num == 1023) {
        memcpy (ctx->mt1023, data, msg_len);
#if 0
        _("System Identification Number", "%u", rtcm_msg_parser_get_uint8(&p));
        _("Horizontal Shift Indicator", "%u", rtcm_msg_parser_get_uint1(&p));
        _("Vertical Shift Indicator", "%u", rtcm_msg_parser_get_uint1(&p));

        x = 0.5 * rtcm_msg_parser_get_int21(&p) / 3600;
        y = 0.5 * rtcm_msg_parser_get_int22(&p) / 3600;
        _("[Lat, Lon] of Origin of Grids", "[%.6fdeg, %.6fdeg]", x, y);

        x = 0.5 * rtcm_msg_parser_get_uint12(&p) / 3600;
        y = 0.5 * rtcm_msg_parser_get_uint12(&p) / 3600;
        _("[N/S, E/W] Grid Area Extension", "[%.6fdeg, %.6fdeg]", x, y);

        x = 0.001 * rtcm_msg_parser_get_int8(&p) / 3600;
        y = 0.001 * rtcm_msg_parser_get_int8(&p) / 3600;
        z = 0.01 * rtcm_msg_parser_get_int15(&p);
        _("Mean [Lat, Lom, Height] Offset", "[%.9fdeg, %.9fdeg, %.2f]", x, y, z);

        for (int i = 0; i < 16; i++) {
            x = 0.00003 * rtcm_msg_parser_get_int9(&p);
            y = 0.00003 * rtcm_msg_parser_get_int9(&p);
            z = 0.001 * rtcm_msg_parser_get_int9(&p);
            _("Lat, Lon, Height Residual", "[%2u] %.5f\", %.5f\", %.3fm", i, x, y, z);
        }

        _("Horizontal Interpolation Method Indicator", "%u", rtcm_msg_parser_get_uint2(&p));
        _("Vertical Interpolation Method Indicator", "%u", rtcm_msg_parser_get_uint2(&p));
        _("Horizontal Grid Quality Indicator", "%u", rtcm_msg_parser_get_uint3(&p));
        _("Vertical Grid Quality Indicator", "%u", rtcm_msg_parser_get_uint3(&p));
        _("Modified Julian Day (MJD) Number", "%u", rtcm_msg_parser_get_uint16(&p));
#endif
    }
}

void
rtcm_parse_data (rtcm_ctx_t *ctx, uint8_t *data, uint32_t n_bytes)
{
    uint16_t msg_len;

    if (ctx->msg_buf_n_bytes) {
        while (ctx->msg_buf_n_bytes < 3) {
            ctx->msg_buf[ctx->msg_buf_n_bytes++] = data++[0];
            if (--n_bytes == 0) {
                return;
            }
        }

        if (ctx->msg_buf[0] == 0xd3 || (((ctx->msg_buf[1] << 8) | ctx->msg_buf[2]) <=  0x3ff)) {
            msg_len = rtcm_get_int(ctx->msg_buf, 14, 10);

            while (msg_len + 6 > ctx->msg_buf_n_bytes++) {
                if (n_bytes-- == 0) {
                    return;
                }
                ctx->msg_buf[ctx->msg_buf_n_bytes++] = data++[0];
            }
            rtcm_msg (ctx, ctx->msg_buf + 3, msg_len);
        }
        ctx->msg_buf_n_bytes = 0;
    }

    while (n_bytes >= 3) {
        if (data[0] != 0xd3 || (((data[1] << 8) | data[2]) > 0x3ff)) {
            data++;
            n_bytes--;
            continue;
        }
        msg_len = rtcm_get_int(data, 14, 10);

        if (msg_len + 6 <= n_bytes) {
            rtcm_msg (ctx, data + 3, msg_len);
            msg_len += 6;
            data += msg_len;
            n_bytes -= msg_len;
        } else {
            goto done;
        }
    }
    if (n_bytes > 0 && data[0] == 0xd3) {
done:
        memcpy(ctx->msg_buf, data, n_bytes);
        ctx->msg_buf_n_bytes = n_bytes;
    }
}


rtcm_ctx_t *rtcm_ctx_alloc()
{
    rtcm_ctx_t *ctx = rtcm_mem_alloc_zero(sizeof (rtcm_ctx_t));
    return ctx;
}

void rtcm_ctx_free(rtcm_ctx_t *ctx)
{
    rtcm_mem_free(ctx);
}

cJSON *
rtcm_get_mt1005_json(rtcm_ctx_t *ctx)
{
    cJSON *root = cJSON_CreateObject();
    rtcm_msg_parser_t p;
    double x, y, z;
    char *error = 0;

    if (ctx == 0) {
        error = "no data";
        goto error;
    }

    rtcm_msg_parser_init (&p, ctx->mt1005);
    if (rtcm_msg_parser_get_uint12 (&p) != 1005) {
        goto error;
    }

    cJSON_AddNumberToObject(root, "refStationId", rtcm_msg_parser_get_uint12(&p));
    rtcm_msg_parser_skip(&p, 6);
    cJSON_AddBoolToObject(root, "gpsSupported", rtcm_msg_parser_get_uint1(&p));
    cJSON_AddBoolToObject(root, "glonassSupported", rtcm_msg_parser_get_uint1(&p));
    cJSON_AddBoolToObject(root, "galileoSupported", rtcm_msg_parser_get_uint1(&p));
    cJSON_AddBoolToObject(root, "physRefStation", rtcm_msg_parser_get_uint1(&p));

    x = 1e-4 * rtcm_msg_parser_get_int38(&p);
    cJSON_AddBoolToObject(root, "singleRecvOsc", rtcm_msg_parser_get_uint1(&p));
    rtcm_msg_parser_skip(&p, 1);

    y = 1e-4 * rtcm_msg_parser_get_int38(&p);
    cJSON_AddNumberToObject(root, "quarterCycleInd", rtcm_msg_parser_get_uint2(&p));

    z = 1e-4 * rtcm_msg_parser_get_int38(&p);

    cJSON *a = cJSON_AddArrayToObject(root, "antennaRefPointECEF");
    cJSON_AddItemToArray(a, cJSON_CreateNumber (x));
    cJSON_AddItemToArray(a, cJSON_CreateNumber (y));
    cJSON_AddItemToArray(a, cJSON_CreateNumber (z));

error:
    if (error) {
        cJSON_AddStringToObject(root, "error", error);
    }
    return root;
}

cJSON *
rtcm_get_mt1021_json(rtcm_ctx_t *ctx)
{
    cJSON *a, *o = cJSON_CreateObject();
    rtcm_msg_parser_t p;
    char *error = 0;
    char tmp[32];

    if (ctx == 0) {
        error = "no data";
        goto error;
    }

    rtcm_msg_parser_init (&p, ctx->mt1021);
    if (rtcm_msg_parser_get_uint12 (&p) != 1021) {
        error = "no data";
        goto error;
    }

    rtcm_msg_parser_get_string(&p, 5, tmp);
    cJSON_AddStringToObject (o, "sourceName", tmp);

    rtcm_msg_parser_get_string(&p, 5, tmp);
    cJSON_AddStringToObject (o, "targetName", tmp);

    cJSON_AddNumberToObject(o, "sysIdNum", rtcm_msg_parser_get_uint8(&p));

    rtcm_msg_parser_skip(&p, 10);

    cJSON_AddNumberToObject(o, "plateNum", rtcm_msg_parser_get_uint5(&p));
    cJSON_AddNumberToObject(o, "compInd", rtcm_msg_parser_get_uint4(&p));
    cJSON_AddNumberToObject(o, "heightInd", rtcm_msg_parser_get_uint2(&p));

    a = cJSON_AddArrayToObject(o, "areaOfValidityOrigin");
    cJSON_AddItemToArray(a, cJSON_CreateNumber (2.0 * rtcm_msg_parser_get_int19(&p) / 3600));
    cJSON_AddItemToArray(a, cJSON_CreateNumber (2.0 * rtcm_msg_parser_get_int20(&p) / 3600));

    a = cJSON_AddArrayToObject(o, "areaOfValidityExt");
    cJSON_AddItemToArray(a, cJSON_CreateNumber (2.0 * rtcm_msg_parser_get_uint14(&p) / 3600));
    cJSON_AddItemToArray(a, cJSON_CreateNumber (2.0 * rtcm_msg_parser_get_uint14(&p) / 3600));

    a = cJSON_AddArrayToObject(o, "translation");
    cJSON_AddItemToArray(a, cJSON_CreateNumber (0.001 * rtcm_msg_parser_get_int23(&p)));
    cJSON_AddItemToArray(a, cJSON_CreateNumber (0.001 * rtcm_msg_parser_get_int23(&p)));
    cJSON_AddItemToArray(a, cJSON_CreateNumber (0.001 * rtcm_msg_parser_get_int23(&p)));

    a = cJSON_AddArrayToObject(o, "rotation");
    cJSON_AddItemToArray(a, cJSON_CreateNumber (0.00002 * rtcm_msg_parser_get_int32(&p) / 3600));
    cJSON_AddItemToArray(a, cJSON_CreateNumber (0.00002 * rtcm_msg_parser_get_int32(&p) / 3600));
    cJSON_AddItemToArray(a, cJSON_CreateNumber (0.00002 * rtcm_msg_parser_get_int32(&p) / 3600));

    cJSON_AddNumberToObject(o, "scaleCorr", 0.00001 * rtcm_msg_parser_get_int25(&p));

    cJSON_AddNumberToObject(o, "semiMajorAxisSrc", 0.001 * rtcm_msg_parser_get_uint24(&p));
    cJSON_AddNumberToObject(o, "semiMinorAxisSrc", 0.001 * rtcm_msg_parser_get_uint25(&p));
    cJSON_AddNumberToObject(o, "semiMajorAxisTgt", 0.001 * rtcm_msg_parser_get_uint24(&p));
    cJSON_AddNumberToObject(o, "semiMinorAxisTgt", 0.001 * rtcm_msg_parser_get_uint25(&p));

    cJSON_AddNumberToObject(o, "hHMQualityInd", rtcm_msg_parser_get_uint3(&p));
    cJSON_AddNumberToObject(o, "vHMQualityInd", rtcm_msg_parser_get_uint3(&p));
error:
    if (error) {
        cJSON_AddStringToObject(o, "error", error);
    }
    return o;
}

cJSON *
rtcm_get_mt1023_json(rtcm_ctx_t *ctx)
{
    cJSON *a, *o = cJSON_CreateObject();
    rtcm_msg_parser_t p;
    char *error = 0;

    if (ctx == 0) {
        error = "no data";
        goto error;
    }

    rtcm_msg_parser_init (&p, ctx->mt1023);
    if (rtcm_msg_parser_get_uint12 (&p) != 1023) {
        error = "no data";
        goto error;
    }

    cJSON_AddNumberToObject(o, "sysIdNum", rtcm_msg_parser_get_uint8(&p));
    cJSON_AddBoolToObject(o, "hShift", rtcm_msg_parser_get_uint1(&p));
    cJSON_AddBoolToObject(o, "vShift", rtcm_msg_parser_get_uint1(&p));

    a = cJSON_AddArrayToObject(o, "originOfGrids");
    cJSON_AddItemToArray(a, cJSON_CreateNumber (0.5 * rtcm_msg_parser_get_int21(&p) / 3600));
    cJSON_AddItemToArray(a, cJSON_CreateNumber (0.5 * rtcm_msg_parser_get_int22(&p) / 3600));

    a = cJSON_AddArrayToObject(o, "gridAreaExt");
    cJSON_AddItemToArray(a, cJSON_CreateNumber (0.5 * rtcm_msg_parser_get_uint12(&p) / 3600));
    cJSON_AddItemToArray(a, cJSON_CreateNumber (0.5 * rtcm_msg_parser_get_uint12(&p) / 3600));

    a = cJSON_AddArrayToObject(o, "meanOffset");
    cJSON_AddItemToArray(a, cJSON_CreateNumber (0.001 * rtcm_msg_parser_get_int8(&p) / 3600));
    cJSON_AddItemToArray(a, cJSON_CreateNumber (0.001 * rtcm_msg_parser_get_int8(&p) / 3600));
    cJSON_AddItemToArray(a, cJSON_CreateNumber (0.01 * rtcm_msg_parser_get_int15(&p)));

    a = cJSON_AddArrayToObject(o, "residual");
    for (int i = 0; i < 16; i++) {
        cJSON *b = cJSON_CreateArray();
        cJSON_AddItemToArray(b, cJSON_CreateNumber (0.00003 * rtcm_msg_parser_get_int9(&p) / 3600));
        cJSON_AddItemToArray(b, cJSON_CreateNumber (0.00003 * rtcm_msg_parser_get_int9(&p) / 3600));
        cJSON_AddItemToArray(b, cJSON_CreateNumber (0.001 * rtcm_msg_parser_get_int9(&p)));
        cJSON_AddItemToArray(a, b);
    }

    cJSON_AddNumberToObject(o, "hInterpolationNethod", rtcm_msg_parser_get_uint2(&p));
    cJSON_AddNumberToObject(o, "vInterpolationNethod", rtcm_msg_parser_get_uint2(&p));
    cJSON_AddNumberToObject(o, "hGridQualityInd", rtcm_msg_parser_get_uint3(&p));
    cJSON_AddNumberToObject(o, "vGridQualityInd", rtcm_msg_parser_get_uint3(&p));
    cJSON_AddNumberToObject(o, "modifiedJulianDayNum", rtcm_msg_parser_get_uint16(&p));

error:
    if (error) {
        cJSON_AddStringToObject(o, "error", error);
    }
    return o;
}

cJSON *
rtcm_get_mt1033_json(rtcm_ctx_t *ctx)
{
    cJSON *o = cJSON_CreateObject();
    rtcm_msg_parser_t p;
    char tmp[32];
    char *error = 0;

    if (ctx == 0) {
        error = "no data";
        goto error;
    }

    rtcm_msg_parser_init (&p, ctx->mt1033);
    if (rtcm_msg_parser_get_uint12 (&p) != 1033) {
        error = "no data";
        goto error;
    }

    cJSON_AddNumberToObject(o, "refStationId", rtcm_msg_parser_get_uint12(&p));
    rtcm_msg_parser_get_string(&p, 8, tmp);
    cJSON_AddStringToObject (o, "antennaDesc", tmp);
    cJSON_AddNumberToObject(o, "antennaSetupId", rtcm_msg_parser_get_uint8(&p));
    rtcm_msg_parser_get_string(&p, 8, tmp);
    cJSON_AddStringToObject (o, "antennaSerialNum", tmp);
    rtcm_msg_parser_get_string(&p, 8, tmp);
    cJSON_AddStringToObject (o, "receiverType", tmp);
    rtcm_msg_parser_get_string(&p, 8, tmp);
    cJSON_AddStringToObject (o, "receiverFwVer", tmp);
    rtcm_msg_parser_get_string(&p, 8, tmp);
    cJSON_AddStringToObject (o, "receiverSerialNum", tmp);
error:
    if (error) {
        cJSON_AddStringToObject(o, "error", error);
    }
    return o;
}
