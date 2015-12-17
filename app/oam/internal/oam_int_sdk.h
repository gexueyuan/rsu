/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : oam_int_sdk.h
 @brief  : oam_int_sdk.h header file
 @author : wanglei
 @history:
           2015-12-8    wanglei    Created file
           ...
******************************************************************************/

#ifndef __OAM_INT_SDK_H__
#define __OAM_INT_SDK_H__
#include "oam_cmn.h"
#include "oam_sdk_cmn.h"

typedef struct{
    cv_sdk_data_order_func_t    * xlats;
    cv_sdk_data_handler_func_t  * handlers; 
    uint32_t                    max_sdk_id;
}cv_sdk_api_ctrl_t;


void cv_oam_sdk_init(cv_sdk_data_order_func_t * xlats,
        cv_sdk_data_handler_func_t * handlers, uint32_t max_sdk_id);

int cv_oam_send_msg(int destfd, uint16_t msg_type, uint8_t * msg, uint16_t msg_len);
void cv_oam_sdk_msg_handler(void *src, cv_sdk_msg_t * msg);
#endif
