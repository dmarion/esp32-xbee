#ifndef ESP32_XBEE_UBX_H
#define ESP32_XBEE_UBX_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <cJSON.h>

#define foreach_ubx_msg \
  _(0x01, 0x03, NAV, STATUS, nav_status) \
  _(0x01, 0x07, NAV, PVT, nav_pvt) \
  _(0x01, 0x13, NAV, HPPOSECEF, nav_hpposecef) \
  _(0x01, 0x14, NAV, HPPOSLLH, nav_hpposllh) \
  _(0x01, 0x35, NAV, SAT, nav_sat) \
  _(0x01, 0x43, NAV, SIG, nav_sig) \
  _(0x0a, 0x04, MON, VER, nav_ver) \
  _(0xf0, 0x00, NMEA, GGA, nmea_gga)

typedef enum {
    UBX_UNKNOWN_MSG = -1,
#define _(c,i,cn,mn,lc) UBX_##cn##_##mn,
    foreach_ubx_msg
#undef _
    UBX_N_MSG_INDICES
} ubx_msg_index_t;

static const uint8_t ubx_msg_class_by_index[] = {
#define _(c,i,cn,mn,lc) [UBX_##cn##_##mn] = (c),
    foreach_ubx_msg
#undef _
};

static const uint8_t ubx_msg_id_by_index[] = {
#define _(c,i,cn,mn,lc) [UBX_##cn##_##mn] = (i),
    foreach_ubx_msg
#undef _
};

typedef enum {
#define _(c,i,cn,mn,lc) XUBX_##cn##_##mn = (((c) << 8) | (i)),
    foreach_ubx_msg
#undef _
} ubx_msg_class_and_id_t;

#define UBX_MAX_MSG_PAYLOAD_LEN 1536


#define STATIC_ASSERT_SIZEOF(d, s) \
    _Static_assert(sizeof (d) == s, "Size of " #d " must be " # s " bytes")

/*
 * UBX Message definitions
 */

/* UBX-NAV-SAT (0x01 0x35) */
typedef struct {
    uint8_t gnssId;
    uint8_t svId;
    uint8_t cno;
    int8_t elev;
    int16_t azim;
    int16_t prRes;
    union {
        uint32_t flags;
        struct {
            uint32_t qualityInd: 3;
            uint32_t svUsed: 1;
            uint32_t health: 2;
            uint32_t diffCorr: 1;
            uint32_t smoothed: 1;
            uint32_t orbitSource: 3;
            uint32_t ephAvail : 1;
            uint32_t almAvail : 1;
            uint32_t anoAvail : 1;
            uint32_t aopAvail : 1;
            uint32_t reserved1 : 1;
            uint32_t sbasCorrUsed : 1;
            uint32_t rtcmCorrUsed : 1;
            uint32_t slasCorrUsed : 1;
            uint32_t spartnCorrUsed : 1;
            uint32_t prCorrUsed : 1;
            uint32_t crCorrUsed : 1;
            uint32_t doCorrUsed : 1;
            uint32_t classCorrUsed : 1;
        };
    };
} __attribute__ ((packed)) ubx_nav_sat_sv_data_t;

STATIC_ASSERT_SIZEOF(ubx_nav_sat_sv_data_t, 12);

typedef struct {
    uint32_t iTOW;
    uint8_t version;
    uint8_t numSvs;
    uint8_t reserved0[2];
    ubx_nav_sat_sv_data_t sv[0];
} __attribute__ ((packed)) ubx_nav_sat_data_t;

STATIC_ASSERT_SIZEOF(ubx_nav_sat_data_t, 8);

/* UBX-NAV-SAT (0x01 0x43) */
typedef struct {
    uint8_t gnssId;
    uint8_t svId;
    uint8_t sigId;
    uint8_t freqId;
    int16_t prRes;
    uint8_t cno;
    uint8_t qualityInd;
    uint8_t corrSource;
    uint8_t ionoModel;
    union {
        uint16_t sigFlags;
        struct {
            uint16_t health : 2;
            uint16_t prSmoothed: 1;
            uint16_t prUsed: 1;
            uint16_t crUsed: 1;
            uint16_t doUsed: 1;
            uint16_t prCorrUsed: 1;
            uint16_t crCorrUsed: 1;
            uint16_t doCorrUsed: 1;
        };
    };
    uint8_t reserved4[4];
} __attribute__ ((packed)) ubx_nav_sig_sig_data_t;

STATIC_ASSERT_SIZEOF(ubx_nav_sig_sig_data_t, 16);

typedef struct {
    uint32_t iTOW;
    uint8_t version;
    uint8_t numSigs;
    uint8_t reserved0[2];
    ubx_nav_sig_sig_data_t sig[0];
} __attribute__ ((packed)) ubx_nav_sig_data_t;

STATIC_ASSERT_SIZEOF(ubx_nav_sig_data_t, 8);

typedef struct {
    uint32_t iTOW;
    uint8_t gpsFix;
    union {
        uint8_t flags;
        struct {
            uint8_t gpsFixOk : 1;
            uint8_t diffSoln : 1;
            uint8_t wknSet : 1;
            uint8_t towSet : 1;
        };
    };
    union {
        uint8_t fixStat;
        struct {
            uint8_t diffCorr : 1;
            uint8_t carrSolnValid: 1;
            uint8_t mapMatching: 1;
        };
    };
    union {
        uint8_t flags2;
        struct {
            uint8_t psmState: 2;
            uint8_t spoofDetState : 2;
            uint8_t carrSoln : 2;
        };
    };
    uint32_t ttff;
    uint32_t msss;
} __attribute__ ((packed)) ubx_nav_status_data_t;

STATIC_ASSERT_SIZEOF(ubx_nav_status_data_t, 16);

typedef struct {
    uint32_t iTOW;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    union {
        uint8_t valid;
        struct {
            uint8_t validDate : 1;
            uint8_t validTime : 1;
            uint8_t fullyResolved : 1;
            uint8_t validMag : 1;
        };
    };
    uint32_t tAcc;
    int32_t nano;
    uint8_t fixType;
    union {
        uint8_t flags;
        struct {
            uint8_t gnssFixOK : 1;
            uint8_t diffSoln : 1;
            uint8_t psmState : 3;
            uint8_t headVehValid : 1;
            uint8_t carrSoln : 2;
        };
    };
    union {
        uint8_t flags2;
        struct {
            uint8_t reserved : 5;
            uint8_t confirmedAvai : 1;
            uint8_t confirmedDate : 1;
            uint8_t confirmedTime : 1;
        };
    };
    uint8_t numSV;
    int32_t lon;
    int32_t lat;
    int32_t height;
    int32_t hMSL;
    uint32_t hAcc;
    uint32_t vAcc;
    int32_t velN;
    int32_t velE;
    int32_t velD;
    int32_t gSpeed;
    int32_t headMot;
    uint32_t sAcc;
    uint32_t headAcc;
    uint16_t pDOP;
    union {
        uint16_t flags3;
        struct {
            uint16_t invalidLlh : 1;
            uint16_t lastCorrectionAge: 4;
        };
    };
    uint8_t reserved0[4];
    int32_t headVeh;
    int16_t magDec;
    uint16_t magAcc;
} ubx_nav_pvt_data_t;

STATIC_ASSERT_SIZEOF(ubx_nav_pvt_data_t, 92);

typedef struct {
    uint8_t version;
    uint8_t reserved0[3];
    uint32_t iTOW;
    int32_t ecefX;
    int32_t ecefY;
    int32_t ecefZ;
    int8_t ecefXHp;
    int8_t ecefYHp;
    int8_t ecefZHp;
    union {
        uint8_t flags;
        struct {
            uint8_t invalidEcef : 1;
        };
    };
    uint32_t pAcc;
} __attribute__ ((packed)) ubx_nav_hpposecef_data_t;

STATIC_ASSERT_SIZEOF (ubx_nav_hpposecef_data_t, 28);

typedef struct {
    uint8_t version;
    uint8_t reserved0[2];
    union {
        uint8_t flags;
        struct {
            uint8_t invalidLlh : 1;
        };
    };
    uint32_t iTOW;
    int32_t lon;
    int32_t lat;
    int32_t height;
    int32_t hMSL;
    int8_t lonHp;
    int8_t latHp;
    int8_t heightHp;
    int8_t hMSLHp;
    uint32_t hAcc;
    uint32_t vAcc;
} __attribute__ ((packed)) ubx_nav_hpposllh_data_t;

STATIC_ASSERT_SIZEOF (ubx_nav_hpposllh_data_t, 36);

typedef struct {
    char extension[30];
} __attribute__ ((packed)) ubx_mon_ver_extension_data_t;
STATIC_ASSERT_SIZEOF (ubx_mon_ver_extension_data_t, 30);

typedef struct {
    char swVersion[30];
    char hwVersion[10];
    ubx_mon_ver_extension_data_t extensions[0];
} __attribute__ ((packed)) ubx_mon_ver_data_t;
STATIC_ASSERT_SIZEOF (ubx_mon_ver_data_t, 40);

/*
 * Context
 */

typedef struct {
#define UBX_SYNC1 0xb5
#define UBX_SYNC2 0x62
    uint8_t sync1;
    uint8_t sync2;
    uint8_t class;
    uint8_t id;
    uint16_t len;
} __attribute__ ((packed)) ubx_msg_hdr_t;

typedef struct {
    uint8_t ok_a;
    uint8_t ok_b;
} __attribute__ ((packed)) ubx_msg_cksum_t;

typedef union {
    struct {
        ubx_msg_hdr_t hdr;
        uint8_t payload[UBX_MAX_MSG_PAYLOAD_LEN + 2];
    };
    uint8_t bytes[0];
} __attribute__ ((packed)) ubx_msg_frame_t;

typedef struct {
    const char *mod;
    const char *fwVer;
    uint8_t protVerMajor;
    uint8_t protVerMinor;

    /* messages received */
    void *msg[UBX_N_MSG_INDICES];

    /* buffer for incomplite frames */
    ubx_msg_frame_t frame;
    uint32_t frame_bytes;
} ubx_ctx_t;

/*
 * Inline functions
 */

static inline void *ubx_mem_alloc(size_t sz)
{
    void *p = malloc (sz + sizeof (uint32_t));
    p += sizeof (uint32_t);
    ((uint32_t *)p)[-1]  = sz;
    return p;
}

static inline void *ubx_mem_alloc_zero(size_t sz)
{
    void *p = ubx_mem_alloc(sz);
    memset (p, 0, sz);
    return p;
}

static inline uint32_t ubx_mem_size(void *p)
{
    if (p) {
        return ((uint32_t *) p)[-1];
    }
    return 0;
}

static inline void ubx_mem_free(void *p)
{
    if (p) {
        free (p - 4);
    }
}

static inline void ubx_msg_calc_checksum (ubx_msg_frame_t *m, ubx_msg_cksum_t *cksum)
{
    uint8_t a = 0, b = 0;
    for (uint32_t i = 2; i < m->hdr.len + 6; i++) {
        a += m->bytes[i];
        b += a;
    }
    if (cksum == 0) {
        cksum = (ubx_msg_cksum_t *) (m->payload + m->hdr.len);
    }
    cksum->ok_a = a;
    cksum->ok_b = b;
}

/*
 * Function Declarations
 */

ubx_ctx_t *ubx_ctx_alloc();
void ubx_ctx_free(ubx_ctx_t *ctx);
void ubx_parse_data(ubx_ctx_t *ctx, uint8_t *data, uint32_t length);
const char *ubx_get_signal_name (uint8_t gnssId, uint8_t sigId);

cJSON * ubx_json_info_object (ubx_ctx_t *ctx);
#endif //ESP32_XBEE_UBX_H
