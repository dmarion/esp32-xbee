#ifndef ESP32_XBEE_RTCM_H
#define ESP32_XBEE_RTCM_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <cJSON.h>

static inline uint64_t
rtcm_get_int (uint8_t *data, uint16_t bit_offset, uint8_t n_bits)
{
    uint64_t v = 0;
    int n;

    for (n = (bit_offset & 0x7) + n_bits ; n > 0; n -= 8) {
        v = (v << 8) | data++[bit_offset >> 3];
    }

    return (v >> (n * -1)) & ((1ULL << n_bits) - 1);
}

typedef struct {
    uint8_t *data;
    uint16_t bit_offset;
} rtcm_msg_parser_t;

static inline void
rtcm_msg_parser_init (rtcm_msg_parser_t *p, void *data)
{
    p->data = data;
    p->bit_offset = 0;
}

static inline uint64_t
rtcm_msg_parser_get_uint (rtcm_msg_parser_t *p, uint8_t n_bits)
{
    uint64_t v;
    v = rtcm_get_int (p->data, p->bit_offset, n_bits);
    p->bit_offset += n_bits;
    return v;
}

static inline int64_t
rtcm_msg_parser_get_int (rtcm_msg_parser_t *p, uint8_t n_bits)
{
    int64_t v = rtcm_msg_parser_get_uint(p, n_bits);
    int64_t r = v & ((1ULL << (n_bits - 1)) - 1);
    return  v == r ? r : -r;
}

#define rtcm_msg_parser_get_uint1(p) (uint8_t) rtcm_msg_parser_get_uint(p, 1)
#define rtcm_msg_parser_get_uint2(p) (uint8_t) rtcm_msg_parser_get_uint(p, 2)
#define rtcm_msg_parser_get_uint3(p) (uint8_t) rtcm_msg_parser_get_uint(p, 3)
#define rtcm_msg_parser_get_uint4(p) (uint8_t) rtcm_msg_parser_get_uint(p, 4)
#define rtcm_msg_parser_get_uint5(p) (uint8_t) rtcm_msg_parser_get_uint(p, 5)
#define rtcm_msg_parser_get_uint6(p) (uint8_t) rtcm_msg_parser_get_uint(p, 6)
#define rtcm_msg_parser_get_uint7(p) (uint8_t) rtcm_msg_parser_get_uint(p, 7)
#define rtcm_msg_parser_get_uint8(p) (uint8_t) rtcm_msg_parser_get_uint(p, 8)
#define rtcm_msg_parser_get_uint10(p) (uint16_t) rtcm_msg_parser_get_uint(p, 10)
#define rtcm_msg_parser_get_uint11(p) (uint16_t) rtcm_msg_parser_get_uint(p, 11)
#define rtcm_msg_parser_get_uint12(p) (uint16_t) rtcm_msg_parser_get_uint(p, 12)
#define rtcm_msg_parser_get_uint14(p) (uint16_t) rtcm_msg_parser_get_uint(p, 14)
#define rtcm_msg_parser_get_uint16(p) (uint16_t) rtcm_msg_parser_get_uint(p, 16)
#define rtcm_msg_parser_get_uint24(p) (uint32_t) rtcm_msg_parser_get_uint(p, 24)
#define rtcm_msg_parser_get_uint25(p) (uint32_t) rtcm_msg_parser_get_uint(p, 25)

#define rtcm_msg_parser_get_int8(p) (int8_t) rtcm_msg_parser_get_int(p, 8)
#define rtcm_msg_parser_get_int9(p) (int16_t) rtcm_msg_parser_get_int(p, 9)
#define rtcm_msg_parser_get_int15(p) (int16_t) rtcm_msg_parser_get_int(p, 15)
#define rtcm_msg_parser_get_int19(p) (int32_t) rtcm_msg_parser_get_int(p, 19)
#define rtcm_msg_parser_get_int20(p) (int32_t) rtcm_msg_parser_get_int(p, 20)
#define rtcm_msg_parser_get_int21(p) (int32_t) rtcm_msg_parser_get_int(p, 21)
#define rtcm_msg_parser_get_int22(p) (int32_t) rtcm_msg_parser_get_int(p, 22)
#define rtcm_msg_parser_get_int23(p) (int32_t) rtcm_msg_parser_get_int(p, 23)
#define rtcm_msg_parser_get_int24(p) (int32_t) rtcm_msg_parser_get_int(p, 24)
#define rtcm_msg_parser_get_int25(p) (int32_t) rtcm_msg_parser_get_int(p, 25)
#define rtcm_msg_parser_get_int32(p) (int32_t) rtcm_msg_parser_get_int(p, 32)
#define rtcm_msg_parser_get_int38(p) (int64_t) rtcm_msg_parser_get_int(p, 38)

static inline void
rtcm_msg_parser_skip (rtcm_msg_parser_t *p, uint8_t n_bits)
{
    p->bit_offset += n_bits;
}

static inline uint16_t
rtcm_msg_parser_get_string (rtcm_msg_parser_t *p, uint8_t n_bits, char *str)
{
    uint16_t n = rtcm_msg_parser_get_uint (p, n_bits);

    for (int i = 0; i < n; i++) {
        str[i] = rtcm_msg_parser_get_uint8 (p);
    }

    str[n] = 0;

    return n;
}

/*
 * Context
 */

typedef struct {
    /* Message Store */
    char mt1005[19];           /* Stationary RTK Reference Station ARP */
    char mt1021[52 + 31 + 31]; /* Helmert / Abridged Molodenski Transformation Parameters */
    char mt1023[73];           /* Residual Message, Ellipsoidal Grid Representation */
    char mt1033[9 + 5 * 31];   /* Receiver and Antenna Descriptors */
    /* tmp msg buffer */
    uint8_t msg_buf[1024 + 6];
    uint16_t msg_buf_n_bytes;
} rtcm_ctx_t;

/*
 * Inline functions
 */

static inline void *rtcm_mem_alloc(size_t sz)
{
    void *p = malloc (sz + sizeof (uint32_t));
    p += sizeof (uint32_t);
    ((uint32_t *)p)[-1]  = sz;
    return p;
}

static inline void *rtcm_mem_alloc_zero(size_t sz)
{
    void *p = rtcm_mem_alloc(sz);
    memset (p, 0, sz);
    return p;
}

static inline uint32_t rtcm_mem_size(void *p)
{
    if (p) {
        return ((uint32_t *) p)[-1];
    }
    return 0;
}

static inline void rtcm_mem_free(void *p)
{
    if (p) {
        free (p - 4);
    }
}


/*
 * Function Declarations
 */

rtcm_ctx_t *rtcm_ctx_alloc();
void rtcm_ctx_free(rtcm_ctx_t *ctx);
void rtcm_parse_data(rtcm_ctx_t *ctx, uint8_t *data, uint32_t length);
cJSON *rtcm_get_mt1005_json(rtcm_ctx_t *ctx);
cJSON *rtcm_get_mt1021_json(rtcm_ctx_t *ctx);
cJSON *rtcm_get_mt1023_json(rtcm_ctx_t *ctx);
cJSON *rtcm_get_mt1033_json(rtcm_ctx_t *ctx);
cJSON *rtcm_get_mt1033_json(rtcm_ctx_t *ctx);

#endif //ESP32_XBEE_RTCM_H
