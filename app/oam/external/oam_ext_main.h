/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : oam_ext_main.h
 @brief  : oam_ext_main.h header file
 @author : wanglei
 @history:
           2015-12-8    wanglei    Created file
           ...
******************************************************************************/

#ifndef __OAM_EXT_MAIN_H__
#define __OAM_EXT_MAIN_H__
#include "oam_cmn.h"
#include "oam_sdk_cmn.h"

typedef struct{
    uint16_t                  sdk_num;
    uint16_t                  evt_num;
    cv_sdk_data_order_func_t    *sdk_xlats;
    cv_sdk_data_order_func_t    *evt_xlats;
}cv_oam_init_info_t;


int cv_oam_client_send_msg(uint16_t frame_type, uint16_t msg_type, uint8_t * msg, uint16_t msg_len);
int cv_oam_init();

#endif
