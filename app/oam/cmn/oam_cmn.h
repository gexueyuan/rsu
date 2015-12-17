/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : oam_cmn.h
 @brief  : header file
 @author : wanglei
 @history:
           2015-12-4    wanglei    Created file
           ...
******************************************************************************/

#ifndef __OAM_CMN_H__
#define __OAM_CMN_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cv_osal.h"
#define OSAL_MODULE_DEBUG
#define OSAL_MODULE_DEBUG_LEVEL OSAL_DEBUG_INFO
#define MODULE_NAME "oam"
#include "cv_osal_dbg.h"

#include "oam_types.h"
#include "test_api.h"

#define OAM_MAX_CLIENT_NUM  (10)

/* 49152 ~ 65535  can't be registed. 'c' 'v' = 0x63 0x76 */
#define OAM_SERVER_PORT (56376)

#define OAM_PRINTF(arg,...)  printf(arg, ##__VA_ARGS__)
#define OAM_ERROR(arg,...)  printf(arg, ##__VA_ARGS__)

#define OAM_ASSERT(x)  \
    do{ \
        if (!(x))  { \
            OAM_ERROR("%s() _%d_: assert fail\n", __FUNCTION__, __LINE__); \
            return ; \
        } \
    }while(0)

#define OAM_ASSERT_RET(x,y) \
    do{ \
        if (!(x))  { \
            OAM_ERROR("%s() _%d_: assert fail\n", __FUNCTION__, __LINE__); \
            return (y); \
        } \
    }while(0)

//#define OAM_LOG(arg,...) OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_INFO, "%s(): " arg, __FUNCTION__,##__VA_ARGS__)
#define OAM_LOG(arg,...)  printf("%s(): " arg, __FUNCTION__,##__VA_ARGS__)
    


#define OAM_MALLOC(size)        malloc((size))
#define OAM_FREE(ptr)           if((ptr) != NULL){ free((ptr)); (ptr) = NULL; }
#define OAM_REALLOC(ptr,size)   realloc((ptr),(size))


#define     CV_OAM_INFO_LEN          32
#define     CV_OAM_MAX_CHIP_NUM      16



#define CV_OAM_RX_THREAD_NAME           "OAM_PKT_RX"
#define CV_OAM_RX_THREAD_STACK_SIZE     8192
#define CV_OAM_RX_THREAD_PRIORITY       TK_PRIO_DEFAULT


#define CV_OAM_PROC_THREAD_NAME             "OAM_CORE"
#define CV_OAM_PROC_THREAD_STACK_SIZE       8192
#define CV_OAM_PROC_THREAD_PRIORITY         TK_PRIO_DEFAULT


#define CV_FRAME_HEADER_LEN             (4)
#define CV_FRAME_PAYLOAD_LEN            (1024)
#define CV_IF_NAME_SIZE                 16
#define CV_MACADDR_LEN                  6
#define CV_ETHERTYPE_IROS               0xFFFF

/* PKT length related definitions */
#define CV_OAM_PKT_MAX_LEN                  1200 
#define CV_OAM_PKT_MIN_LEN                  64   


/* frame carrying heartbeat packet  */
#define CV_OAM_FRAME_TYPE_KEEPALIVE      1

/* frame carrying an command(get/set) message */
#define CV_OAM_FRAME_TYPE_CMD_MSG        2
 
/* frame carrying an Event/Alarm message */
#define CV_OAM_FRAME_TYPE_EVENT_MSG      3

/* frame carrying raw data of nmea, dscr... */
#define CV_OAM_FRAME_TYPE_RAW_DATA       4


/* oam frame encapsulation */
typedef struct {
    /* ether header */
    uint16_t   frame_type;
    /* layer three header */
    union {
        uint16_t   count;
        uint16_t   cmd_type;  /* 1: request,  2: respons */
        uint16_t   evt_type;  /* 1: event/alarm  2:ack */
        uint16_t   raw_data_type;
    }layer2_type;
    /* the payload */
    uint8_t    data[CV_FRAME_PAYLOAD_LEN];
} __attribute__((packed)) cv_oam_frame_t;


#define CV_OAM_L2_MSG_CMD_REQ    0x0001
#define CV_OAM_L2_MSG_CMD_RSP    0x0002

#define CV_OAM_L2_MSG_EVENT      0x0001
#define CV_OAM_L2_MSG_EVENT_ACK  0x0002



typedef struct _oam_client {
    uint8_t valid;
    uint16_t sock_port;
	int sock_fd;
	struct sockaddr_in client_addr;
} oam_client_t;

typedef struct _oam_server {
	int srv_fd;
	uint16_t srv_port;

	oam_client_t client[OAM_MAX_CLIENT_NUM];
	unsigned int client_cnt;
}oam_server_t;

int cv_oam_pkt_send(int dest, uint8_t * frame, int32_t len);

void oam_buf_print(uint8_t *pbuf , uint32_t len);
#endif

