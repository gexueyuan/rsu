/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : test_sdk_handler.c
 @brief  : internel sdk cmd msg handler functions
 @author : wanglei
 @history:
           2015-12-8    wanglei    Created file
           ...
******************************************************************************/
#include "oam_sdk_cmn.h"
#include "test_api.h"
#include "test_sdk_msg.h"
#include "test_sdk_msg_wrap.h"

oam_status cv_oam_vam_cfg_get_cb(uint8_t * data, uint32_t size)
{
    cv_vam_cfg_get_msg_wrap_t* cfg = (cv_vam_cfg_get_msg_wrap_t *) data;
    return cv_oam_vam_cfg_get(&cfg->cfg);
}

oam_status cv_oam_vam_cfg_set_cb(uint8_t * data, uint32_t size)
{
    cv_vam_cfg_set_msg_wrap_t* cfg = (cv_vam_cfg_set_msg_wrap_t *)data;

    return cv_oam_vam_cfg_set(cfg->mask, &cfg->cfg);
}


oam_status cv_oam_vam_bsm_trigger_cb(uint8_t * data, uint32_t size)
{
    int bsm_trigger = *(int*)data;

    return cv_oam_vam_bsm_trigger(bsm_trigger);
}

oam_status cv_oam_vam_set_print_cb(uint8_t * data, uint32_t size)
{
    int print_type = *(int*)data;

    return cv_oam_vam_set_print(print_type);
}

cv_sdk_data_handler_func_t g_sdk_handler_table[CV_SDK_REQ_TYPE_END+1] =
{
   #undef xx
   #define xx(SEQ,TYPE,HTON_FUNC,HANDLER_FUNC ) (HANDLER_FUNC),
   CV_SDK_REQ_TYPE_INFO
};
