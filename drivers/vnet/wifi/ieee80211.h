#ifndef __IEEE80211
#define __IEEE80211

#include "platform.h"

/* 802.11n HT capability AMPDU settings (for ampdu_params_info) */
#define IEEE80211_HT_AMPDU_PARM_FACTOR          0x03
#define IEEE80211_HT_AMPDU_PARM_DENSITY         0x1C

#define IEEE80211_HT_CAP_SUP_WIDTH_20_40        0x0002
#define IEEE80211_HT_CAP_SGI_40                 0x0040
#define IEEE80211_HT_CAP_MAX_AMSDU              0x0800

#define IEEE80211_HT_MCS_MASK_LEN               10

/* value domain of 802.11 header FC.Tyte, which is b3..b2 of the 1st-byte of MAC header */
#define BTYPE_MGMT                  0
#define BTYPE_CNTL                  1
#define BTYPE_DATA                  2

/* value domain of 802.11 MGMT frame's FC.subtype, which is b7..4 of the 1st-byte of MAC header */
#define SUBTYPE_ASSOC_REQ           0
#define SUBTYPE_ASSOC_RSP           1
#define SUBTYPE_REASSOC_REQ         2
#define SUBTYPE_REASSOC_RSP         3
#define SUBTYPE_PROBE_REQ           4
#define SUBTYPE_PROBE_RSP           5
#define SUBTYPE_BEACON              8
#define SUBTYPE_ATIM                9
#define SUBTYPE_DISASSOC            10
#define SUBTYPE_AUTH                11
#define SUBTYPE_DEAUTH              12
#define SUBTYPE_ACTION              13
#define SUBTYPE_ACTION_NO_ACK       14


typedef struct _frame_control {
#ifndef __LITTLE_ENDIAN
    u16 Order:1;        /* Strict order expected */
    u16 Wep:1;        /* Wep data */
    u16 MoreData:1;    /* More data bit */
    u16 PwrMgmt:1;    /* Power management bit */
    u16 Retry:1;        /* Retry status bit */
    u16 MoreFrag:1;    /* More fragment bit */
    u16 FrDs:1;        /* From DS indication */
    u16 ToDs:1;        /* To DS indication */
    u16 SubType:4;    /* MSDU subtype */
    u16 Type:2;        /* MSDU type */
    u16 Ver:2;        /* Protocol version */
#else
    u16 Ver:2;        /* Protocol version */
    u16 Type:2;        /* MSDU type */
    u16 SubType:4;    /* MSDU subtype */
    u16 ToDs:1;        /* To DS indication */
    u16 FrDs:1;        /* From DS indication */
    u16 MoreFrag:1;    /* More fragment bit */
    u16 Retry:1;        /* Retry status bit */
    u16 PwrMgmt:1;    /* Power management bit */
    u16 MoreData:1;    /* More data bit */
    u16 Wep:1;        /* Wep data */
    u16 Order:1;        /* Strict order expected */
#endif    /* !__BIG_ENDIAN */
} frame_control_t;

struct ieee80211_hdr {
	frame_control_t fc;
	le16 duration_id;
	u8 addr1[6];
	u8 addr2[6];
	u8 addr3[6];
	le16 seq_ctrl;
	/* followed by 'u8 addr4[6];' if ToDS and FromDS is set in data frame
	 */
} __attribute__ ((packed));


#define IEEE80211_DA_FROMDS addr1
#define IEEE80211_SA_FROMDS addr2
#define IEEE80211_BSSID_FROMDS addr3

#define IEEE80211_HDRLEN (sizeof(struct ieee80211_hdr))
#define IEEE80211_FC(type, stype) host_to_le16((type << 2) | (stype << 4))


#define ARRAY_SIZE(ar) (sizeof(ar)/sizeof(ar[0]))
#define DIV_ROUND_UP(x, y) (((x) + (y - 1)) / (y))


#endif /* __IEEE80211 */
