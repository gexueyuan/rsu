/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : oam_ext_raw.c
 @brief  : handl raw data pkt 
 @author : wanglei
 @history:
           2015-12-7    wanglei    Created file
           ...
******************************************************************************/
#include "oam_ext_raw.h"

static cv_raw_pkt_ctrl_t  g_raw_ctrl;

void cv_oam_raw_pkt_init()
{
    uint32_t ret = 0 ;
    ret = os_mutex_create(&g_raw_ctrl.mutex);
    if(ret != 0){
        OAM_LOG("Create mutex failed, ret = %d \n",ret);
        return;
    }

    g_raw_ctrl.pkt_cfg = NULL;
}


int32_t cv_oam_raw_pkt_reg(
        uint32_t type,
        cv_raw_pkt_func_t   func,
        void * cookie)
{
    cv_raw_pkt_cfg_t * cfg = NULL;


    cfg = OAM_MALLOC(sizeof(cv_raw_pkt_cfg_t));
    if(cfg == NULL){
        OAM_LOG("No memory \n");
        return OAM_E_NO_MEMORY;
    }

    memset(cfg,0x00,sizeof(cv_raw_pkt_cfg_t));

    cfg->handler = func;
    cfg->type = type;
    cfg->cookie = cookie;
    cfg->next = g_raw_ctrl.pkt_cfg;
    g_raw_ctrl.pkt_cfg = cfg;

    return OAM_E_OK;
}


void cv_oam_raw_pkt_proc(cv_raw_pkt_msg_t * msg)
{
    uint32_t pkt_type = 0 ;
    uint16_t len = 0 ;
    cv_raw_pkt_cfg_t * cfg = NULL;

    OAM_ASSERT(msg != NULL);

    pkt_type = oam_ntohl(msg->data_type);
    len = oam_ntohs(msg->len);

    OAM_LOG("Recv raw pkt type = %d  \n", pkt_type);

    os_mutex_lock(&g_raw_ctrl.mutex);
    cfg = g_raw_ctrl.pkt_cfg;
    while(cfg != NULL){
        if(cfg->type == pkt_type 
                && cfg->handler != NULL){
            cfg->handler(len,msg->data,cfg->cookie);
        }
        cfg = cfg->next;
    }
    os_mutex_unlock(&g_raw_ctrl.mutex);

}

