/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : test_api.c
 @brief  : externel oam cmd api
 @author : wanglei
 @history:
           2015-12-8    wanglei    Created file
           ...
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "oam_cmn.h"
#include "oam_sdk_cmn.h"
#include "test_sdk_msg.h"
#include "test_api.h"
#include "test_sdk_msg_wrap.h"


oam_status cv_oam_vam_cfg_get(vam_config_t * cfg)
{
    uint8_t buf[CV_SDK_MSG_MAX_LEN] = {0};
    cv_sdk_msg_t *pMsg = (cv_sdk_msg_t *)buf;
    oam_status result = OAM_E_ERROR;
    cv_vam_cfg_get_msg_wrap_t * _wrap_cfg = (cv_vam_cfg_get_msg_wrap_t*)(pMsg->data);

    if( NULL == cfg){
        return OAM_E_PARAM;
    }

    memset(buf,0x00,sizeof(buf));

    pMsg->len = sizeof(cv_vam_cfg_get_msg_wrap_t);
    pMsg->sdk_id = CV_SDK_VAM_PARAM_CFG_GET;
    
    result = (oam_status)  cv_oam_sdk_api_request(pMsg);

    if(result != OAM_E_OK){
        return result;
    }

    memcpy(cfg, &_wrap_cfg->cfg, sizeof(vam_config_t));

    return OAM_E_OK;
}


oam_status cv_oam_vam_cfg_set(uint32_t mask, vam_config_t * cfg)
{
    uint8_t buf[CV_SDK_MSG_MAX_LEN] = {0};
    cv_sdk_msg_t *pMsg = (cv_sdk_msg_t *)buf;
    oam_status result = OAM_E_ERROR;
    cv_vam_cfg_set_msg_wrap_t * _wrap_cfg = (cv_vam_cfg_set_msg_wrap_t*)(pMsg->data);

    if( NULL == cfg){
        return OAM_E_PARAM;
    }

    memset(buf,0x00,sizeof(buf));

    pMsg->len = sizeof(cv_vam_cfg_set_msg_wrap_t);
    pMsg->sdk_id = CV_SDK_VAM_PARAM_CFG_SET;

    _wrap_cfg->mask = mask;
    memcpy(&_wrap_cfg->cfg, cfg, sizeof(vam_config_t));

    result = (oam_status)  cv_oam_sdk_api_request(pMsg);
    memcpy(cfg, &_wrap_cfg->cfg, sizeof(vam_config_t));

    if(result != OAM_E_OK){
        return result;
    }

    return OAM_E_OK;
}

oam_status cv_oam_vam_bsm_trigger(uint8_t trigger)
{
    cv_sdk_msg_t pMsg;
    oam_status result = OAM_E_ERROR;
    memset(&pMsg,0,sizeof(cv_sdk_msg_t));

    pMsg.len = sizeof(uint8_t);
    pMsg.sdk_id = CV_SDK_VAM_BSM_TRIGGER;
    pMsg.data[0] = trigger;
    
    result = (oam_status)  cv_oam_sdk_api_request(&pMsg);
    
    if(result != OAM_E_OK){
        return result;
    }

    return OAM_E_OK;
}


oam_status cv_oam_vam_set_print(int type)
{


}
