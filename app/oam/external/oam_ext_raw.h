/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : oam_ext_raw.h
 @brief  : oam_ext_raw.h header file
 @author : wanglei
 @history:
           2015-12-8    wanglei    Created file
           ...
******************************************************************************/

#ifndef __OAM_EXT_RAW_H__
#define __OAM_EXT_RAW_H__
#include "oam_cmn.h"
#include "oam_raw.h"

typedef void (*cv_raw_pkt_func_t)(
        int32_t  len,
        void* pkt_data, 
        void* cookie);

typedef struct cv_raw_pkt_cfg
{
    uint32_t              type;
    cv_raw_pkt_func_t       handler;
    void                  * cookie;
    struct cv_raw_pkt_cfg * next;
}cv_raw_pkt_cfg_t;

typedef struct{
    cv_raw_pkt_cfg_t         * pkt_cfg;
    uint32_t                 mutex;
}cv_raw_pkt_ctrl_t;


void cv_oam_raw_pkt_init();

int32_t cv_oam_raw_pkt_reg(
        uint32_t type,
        cv_raw_pkt_func_t   func,
        void * cookie);

void cv_oam_raw_pkt_proc(
        cv_raw_pkt_msg_t * msg);
#endif

