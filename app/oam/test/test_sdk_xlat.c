/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : test_sdk_xlat.c
 @brief  : oam api example for test
 @author : wanglei
 @history:
           2015-12-7    wanglei    Created file
           ...
******************************************************************************/

#include "oam_cmn.h"
#include "sdk_util.h"
#include "test_api.h"
#include "test_sdk_msg.h"
#include "test_sdk_msg_wrap.h"

void cv_oam_vam_cfg_get_byte_reverse(uint8_t*data, uint32_t size, uint8_t type)
{
    cv_vam_cfg_get_msg_wrap_t* cfg = (cv_vam_cfg_get_msg_wrap_t *) data;

    if(data == NULL){
        return;
    }
    cfg->cfg.bsm_boardcast_period = cv_short_byte_reverse(cfg->cfg.bsm_boardcast_period, type);

}

void cv_oam_vam_cfg_set_byte_reverse(uint8_t*data, uint32_t size, uint8_t type)
{
    cv_vam_cfg_set_msg_wrap_t* cfg = (cv_vam_cfg_set_msg_wrap_t *) data;

    if(data == NULL){
        return;
    }

    cfg->mask = cv_int_byte_reverse(cfg->mask, type);
    cfg->cfg.bsm_boardcast_period = cv_short_byte_reverse(cfg->cfg.bsm_boardcast_period, type);
}

cv_sdk_data_order_func_t  g_sdk_xlat_table[CV_SDK_REQ_TYPE_END+1] =
{
   #undef xx
   #define xx(SEQ,TYPE, REVERSE_FUNC, HANDLER_FUNC ) (REVERSE_FUNC),
   CV_SDK_REQ_TYPE_INFO
};

