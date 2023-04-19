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

    rtcm_msg_parser_init (&p, data);
    msg_num = rtcm_msg_parser_get_uint12 (&p);

    if (msg_num == 1005) {
	rtcm_ctx_lock (ctx);
        memcpy (ctx->mt1005, data, msg_len);
	rtcm_ctx_unlock (ctx);
    } else if (msg_num == 1007) {
	rtcm_ctx_lock (ctx);
        memcpy (ctx->mt1007, data, msg_len);
	rtcm_ctx_unlock (ctx);
    } else if (msg_num == 1032) {
	rtcm_ctx_lock (ctx);
        memcpy (ctx->mt1032, data, msg_len);
	rtcm_ctx_unlock (ctx);
    } else if (msg_num == 1033) {
	rtcm_ctx_lock (ctx);
        memcpy (ctx->mt1033, data, msg_len);
	rtcm_ctx_unlock (ctx);
    } else if (msg_num == 1021) {
	rtcm_ctx_lock (ctx);
        memcpy (ctx->mt1021, data, msg_len);
	rtcm_ctx_unlock (ctx);
    } else if (msg_num == 1023) {
	rtcm_ctx_lock (ctx);
        memcpy (ctx->mt1023, data, msg_len);
	rtcm_ctx_unlock (ctx);
    } else {
	//ESP_LOGI(TAG, "unknown msg %u len %u", msg_num, msg_len);
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
rtcm_get_json(rtcm_ctx_t *ctx_arg)
{
    cJSON *root = cJSON_CreateObject();
    rtcm_msg_parser_t p;
    rtcm_ctx_t ctx;
    char tmp[256];

    if (ctx_arg == 0) {
	return root;
    }

    rtcm_ctx_lock(ctx_arg);
    memcpy(&ctx, ctx_arg, sizeof (rtcm_ctx_t));
    rtcm_ctx_unlock(ctx_arg);

    rtcm_msg_parser_init (&p, ctx.mt1005);
    if (rtcm_msg_parser_get_uint12 (&p) == 1005) {
	cJSON *o = cJSON_AddObjectToObject(root, "1005");
	double x, y, z;
	cJSON_AddNumberToObject(o, "refStationId", rtcm_msg_parser_get_uint12(&p));
	cJSON_AddNumberToObject(o, "itrfRealizationYear", rtcm_msg_parser_get_uint6(&p));
	cJSON_AddBoolToObject(o, "gpsSupported", rtcm_msg_parser_get_uint1(&p));
	cJSON_AddBoolToObject(o, "glonassSupported", rtcm_msg_parser_get_uint1(&p));
	cJSON_AddBoolToObject(o, "galileoSupported", rtcm_msg_parser_get_uint1(&p));
	cJSON_AddBoolToObject(o, "physRefStation", rtcm_msg_parser_get_uint1(&p));

	x = 1e-4 * rtcm_msg_parser_get_int38(&p);
	cJSON_AddBoolToObject(o, "singleRecvOsc", rtcm_msg_parser_get_uint1(&p));
	rtcm_msg_parser_skip(&p, 1);

	y = 1e-4 * rtcm_msg_parser_get_int38(&p);
	cJSON_AddNumberToObject(o, "quarterCycleInd", rtcm_msg_parser_get_uint2(&p));

	z = 1e-4 * rtcm_msg_parser_get_int38(&p);

	cJSON *a = cJSON_AddArrayToObject(o, "antennaRefPointECEF");
	cJSON_AddItemToArray(a, cJSON_CreateNumber (x));
	cJSON_AddItemToArray(a, cJSON_CreateNumber (y));
	cJSON_AddItemToArray(a, cJSON_CreateNumber (z));
    }

    rtcm_msg_parser_init (&p, ctx.mt1007);
    if (rtcm_msg_parser_get_uint12 (&p) == 1007) {
	cJSON *o = cJSON_AddObjectToObject(root, "1007");
	cJSON_AddNumberToObject(o, "refStationId", rtcm_msg_parser_get_uint12(&p));
	rtcm_msg_parser_get_string(&p, 8, tmp);
	cJSON_AddStringToObject (o, "antennaDesc", tmp);
	cJSON_AddNumberToObject(o, "antennaSetupId", rtcm_msg_parser_get_uint8(&p));
    }

    rtcm_msg_parser_init (&p, ctx.mt1021);
    if (rtcm_msg_parser_get_uint12 (&p) == 1021) {
	cJSON *o = cJSON_AddObjectToObject(root, "1021");
	cJSON *a;
	uint16_t utmi, v;

	rtcm_msg_parser_get_string(&p, 5, tmp);
	cJSON_AddStringToObject (o, "sourceName", tmp);

	rtcm_msg_parser_get_string(&p, 5, tmp);
	cJSON_AddStringToObject (o, "targetName", tmp);

	cJSON_AddNumberToObject(o, "sysIdNum", rtcm_msg_parser_get_uint8(&p));

	utmi = rtcm_msg_parser_get_uint10(&p);
	a = cJSON_AddArrayToObject(o, "utilTransMsgInd");
	for (int i = 0; i < 5; i++)
	    if (utmi & (1 << i))
		cJSON_AddItemToArray(a, cJSON_CreateNumber (1023 + i));

	v = rtcm_msg_parser_get_uint5(&p);

	if (v > 0 && v < 17) {
	    static char *plates[] = {0,	     "AFRC", "ANTA", "ARAB", "AUST",
				     "CARB", "COCO", "EURA", "INDI", "NOAM",
				     "NAZC", "PCFC", "SOAM", "JUFU", "PHIL",
				     "RIVR", "SCOT"};

	    static char *plateDescs[] = {0,
					 "Africa",
					 "Antarctica",
					 "Arabia",
					 "Australia",
					 "Caribbea",
					 "Cocos",
					 "Eurasia",
					 "India",
					 "N. America",
					 "Nazca",
					 "Pacific",
					 "S. America",
					 "Juan de Fuca",
					 "Philippine",
					 "Rivera",
					 "Scotia"};

	    cJSON_AddStringToObject(o, "plate", plates[v]);
	    cJSON_AddStringToObject(o, "plateDesc", plateDescs[v]);
	}

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

	cJSON_AddNumberToObject(o, "semiMajorAxisSrc", 0.001 * rtcm_msg_parser_get_uint24(&p) + 6370000);
	cJSON_AddNumberToObject(o, "semiMinorAxisSrc", 0.001 * rtcm_msg_parser_get_uint25(&p) + 6350000);
	cJSON_AddNumberToObject(o, "semiMajorAxisTgt", 0.001 * rtcm_msg_parser_get_uint24(&p) + 6370000);
	cJSON_AddNumberToObject(o, "semiMinorAxisTgt", 0.001 * rtcm_msg_parser_get_uint25(&p) + 6350000);

	cJSON_AddNumberToObject(o, "hHMQualityInd", rtcm_msg_parser_get_uint3(&p));
	cJSON_AddNumberToObject(o, "vHMQualityInd", rtcm_msg_parser_get_uint3(&p));
    }

    rtcm_msg_parser_init (&p, ctx.mt1023);
    if (rtcm_msg_parser_get_uint12 (&p) == 1023) {
	cJSON *o = cJSON_AddObjectToObject(root, "1023");
	cJSON *a;
	uint32_t v;
	static const char *interpolationMethods[] = {"bilinear", "bicubic", "bispline", "reserved"};

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

	v = rtcm_msg_parser_get_uint2(&p);
	cJSON_AddStringToObject(o, "hInterpolationNethod", interpolationMethods[v]);
	v = rtcm_msg_parser_get_uint2(&p);
	cJSON_AddStringToObject(o, "vInterpolationNethod", interpolationMethods[v]);
	cJSON_AddNumberToObject(o, "hGridQualityInd", rtcm_msg_parser_get_uint3(&p));
	cJSON_AddNumberToObject(o, "vGridQualityInd", rtcm_msg_parser_get_uint3(&p));
	cJSON_AddNumberToObject(o, "modifiedJulianDayNum", rtcm_msg_parser_get_uint16(&p));
    }

    rtcm_msg_parser_init (&p, ctx.mt1032);
    if (rtcm_msg_parser_get_uint12 (&p) == 1032) {
	cJSON *o = cJSON_AddObjectToObject(root, "1032");
	cJSON *a;

	cJSON_AddNumberToObject(o, "physRefStationId", rtcm_msg_parser_get_uint12(&p));
	cJSON_AddNumberToObject(o, "nonPhysRefStationId", rtcm_msg_parser_get_uint12(&p));
	cJSON_AddNumberToObject(o, "itrfEpochYear", rtcm_msg_parser_get_uint6(&p));

	a = cJSON_AddArrayToObject(o, "antennaRefPointECEF");
	cJSON_AddItemToArray(a, cJSON_CreateNumber (1e-4 * rtcm_msg_parser_get_int38 (&p)));
	cJSON_AddItemToArray(a, cJSON_CreateNumber (1e-4 * rtcm_msg_parser_get_int38 (&p)));
	cJSON_AddItemToArray(a, cJSON_CreateNumber (1e-4 * rtcm_msg_parser_get_int38 (&p)));
    }

    rtcm_msg_parser_init (&p, ctx.mt1033);
    if (rtcm_msg_parser_get_uint12 (&p) == 1033) {
	cJSON *o = cJSON_AddObjectToObject(root, "1033");

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
    }

    return root;
}
