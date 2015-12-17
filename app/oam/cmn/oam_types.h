/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : oam_types.h
 @brief  : oam_types.h header file
 @author : wanglei
 @history:
           2015-12-4    wanglei    Created file
           ...
******************************************************************************/
#ifndef __OAM_TYPES_H__
#define __OAM_TYPES_H__

#define _OAM_STRUCT_ALIGN_       __attribute__((packed))
#define CHECK(condition, ret) if (!(condition)) return ret

typedef unsigned long long      oam_uint64;
typedef long long               oam_int64;
typedef unsigned int            oam_uint32;
typedef int                     oam_int32;
typedef unsigned short          oam_uint16;
typedef short                   oam_int16;
typedef unsigned char           oam_uint8;
typedef char                    oam_int8;
typedef unsigned int            oam_boolean;
typedef double                  oam_double;
typedef oam_uint16              oam_ipv6_t[8];

typedef struct {
    /* invocation identifier */
    oam_uint32  invoke_id;
    /* the opaque application data */
    void *     apps_data;
} oam_callback_context_t;
/* to be remove - end */


typedef int (*oam_funcptr)(void *arg);

typedef enum{
    OAM_E_OK                         = 0,
    OAM_E_FORMAT                     = 1,
    OAM_E_NO_MEMORY                  = 2,
    OAM_E_PARAM                      = 3,
    OAM_E_NOT_FOUND                  = 4,
    OAM_E_NOT_SUPPORT                = 5,
    OAM_E_CONFLICT                   = 6,
    OAM_E_TIMEOUT                    = 7,
    OAM_E_TOO_MUCH_API               = 8,
    OAM_E_WRONG_SDK_ID               = 9,
    OAM_E_SOCKET_ERROR               = 10,
    OAM_E_CREATE_MSGQ                = 11,
    OAM_E_CREATE_THREAD              = 12,
    OAM_E_ERROR                      = 0xffffffff
}oam_status;


#pragma pack(1)
typedef union {
    oam_uint8       addr[6];
    struct {
        oam_uint32 hi32;
        oam_uint16 lo16;
    }_OAM_STRUCT_ALIGN_ u;   /* most callers will prefer this format; */
    struct {
        oam_uint16 hi16;
        oam_uint32 lo32;
    }_OAM_STRUCT_ALIGN_ u2;   /* while some callers will prefer this format. */
    struct {
        oam_uint16 val16[3];
    }_OAM_STRUCT_ALIGN_ u3;   /* very few callers will prefer this format */
}_OAM_STRUCT_ALIGN_ oam_mac_t;
#pragma pack()

#define OAM_MAC_ZERO(mac) ((!(mac).u2.lo32 && !(mac).u2.hi16) ? TRUE: FALSE)
#define OAM_MACADDR_LEN 6

#define OAM_OK    OAM_E_OK
#define OAM_ERROR OAM_E_ERROR

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL ((void *) 0)
#endif

#define _OAM_INLINE_ __inline__ static

#if defined(__BIG_ENDIAN)

#ifndef oam_ntohl
#define oam_ntohl(x)        (x)
#endif

#ifndef oam_ntohs
#define oam_ntohs(x)        (x)
#endif

#ifndef oam_htonl
#define oam_htonl(x)        (x)
#endif

#ifndef oam_htons
#define oam_htons(x)        (x)
#endif

#ifndef oam_ntohll
#define oam_ntohll(x)       (x)
#endif


#ifndef oam_htonll
#define oam_htonll(x)       (x)
#endif


#elif defined(__LITTLE_ENDIAN)

#ifndef oam_ntohl
#define oam_ntohl(x)        ((((x) & 0x000000ff) << 24) | \
                            (((x) & 0x0000ff00) <<  8) | \
                            (((x) & 0x00ff0000) >>  8) | \
                            (((x) & 0xff000000) >> 24))
#endif


#ifndef oam_htonl
#define oam_htonl(x)        ((((x) & 0x000000ff) << 24) | \
                            (((x) & 0x0000ff00) <<  8) | \
                            (((x) & 0x00ff0000) >>  8) | \
                            (((x) & 0xff000000) >> 24))
#endif


#ifndef oam_ntohs
#define oam_ntohs(x)        ((((x) & 0x00ff) << 8) | \
                            (((x) & 0xff00) >> 8))
#endif

#ifndef oam_htons
#define oam_htons(x)        ((((x) & 0x00ff) << 8) | \
                            (((x) & 0xff00) >> 8))

#endif

#ifndef oam_ntohll
#define oam_ntohll(x)        ((((oam_uint64)oam_ntohl(x)) << 32) | \
                                       oam_ntohl(x >> 32))
#endif


#ifndef oam_htonll
#define oam_htonll(x)        ((((oam_uint64)oam_htonl(x)) << 32) | \
                                 oam_htonl(x >> 32))
#endif

#else
#error Endianness not defined
#endif

#endif /* __OAM_TYPES_H__ */
