/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : oam_sdk_cmn.h
 @brief  : oam_sdk_cmn.h header file
 @author : wanglei
 @history:
           2015-12-4    wanglei    Created file
           ...
******************************************************************************/

#ifndef __OAM_SDK_CMN_H__
#define __OAM_SDK_CMN_H__
#include "oam_cmn.h"
#include "oam_types.h"

#define CV_SDK_MSG_MAX_LEN  1024 

typedef struct
{   
    uint32_t  ret;
    uint32_t  req_id;
    uint32_t  reservd;

    uint16_t  sdk_id;
    uint16_t  len;   
    uint8_t   data[1];
} __attribute__((packed))cv_sdk_msg_t;

#define CV_SDK_MSG_HDR_LEN			((uint32_t)(((cv_sdk_msg_t*)0)->data))

typedef enum
{
    CV_SDK_DATA_HTON,
    CV_SDK_DATA_NTOH
} cv_sdk_data_xlat_type;

typedef void (*cv_sdk_data_order_func_t)(uint8_t* data, uint32_t len, uint8_t type);
typedef uint32_t(*cv_sdk_data_handler_func_t)(uint8_t *, uint32_t);

typedef struct
{
    uint32_t  seq_num;
    uint16_t  evt_id;
    uint16_t  len;
    uint8_t   data[1];
} __attribute__((packed)) cv_sdk_evt_msg_t;

typedef struct {
    uint32_t  seq_num;
}__attribute__((packed)) cv_sdk_evt_ack_t;

extern int cv_oam_sdk_api_request(cv_sdk_msg_t * msg);

#endif
