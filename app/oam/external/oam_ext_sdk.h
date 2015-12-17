/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : oam_ext_sdk.h
 @brief  : oam_ext_sdk.h header file
 @author : wanglei
 @history:
           2015-12-7    wanglei    Created file
           ...
******************************************************************************/
#ifndef __OAM_SDK_PROC_H__
#define __OAM_SDK_PROC_H__
#include "oam_cmn.h"
#define CV_SDK_MAX_REQ                255
#define CV_SDK_REQ_TIMEOUT            (6 * 1000)

typedef struct{
    uint32_t          valid;
    uint32_t          req_id;
    osal_sem_t        sem;
    cv_sdk_msg_t      *msg;
}cv_sdk_req_record_t;

typedef struct{
    cv_sdk_data_order_func_t  * xlats;
    cv_sdk_req_record_t         reqs[CV_SDK_MAX_REQ];
    uint32_t                  req_next;
    uint32_t                  max_sdk_id;
    osal_mutex_t              mutex;
}cv_sdk_api_ctrl_t;


void cv_oam_sdk_api_rsp(cv_sdk_msg_t * msg);
void cv_oam_sdk_init(cv_sdk_data_order_func_t * xlats, uint32_t max_sdk_id);
int cv_oam_sdk_api_request(cv_sdk_msg_t * msg);


#endif
