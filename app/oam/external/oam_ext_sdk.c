/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : oam_ext_sdk.c
 @brief  : externel client  oam cmd api process
 @author : wanglei
 @history:
           2015-12-8    wanglei    Created file
           ...
******************************************************************************/
#include "oam_cmn.h"
#include "oam_sdk_cmn.h"
#include "oam_ext_sdk.h"

static cv_sdk_api_ctrl_t g_sdk_ctrl;

cv_sdk_req_record_t * cv_sdk_req_record_alloc(
        cv_sdk_msg_t * msg)
{
    uint32_t i = 0 ; 
    cv_sdk_req_record_t * req = NULL;
    
    for(i = 0; i < CV_SDK_MAX_REQ; i++){
        if(!g_sdk_ctrl.reqs[i].valid){
            req = &g_sdk_ctrl.reqs[i];
            break;
        }
    }

    if(req != NULL){
        req->req_id = g_sdk_ctrl.req_next;
        g_sdk_ctrl.req_next ++;
        req->msg = msg;

        if(os_sem_init(&req->sem, 0,0)) {
            (void) fprintf(stderr, "req_record_alloc sem_init failed: %s\r\n", strerror(errno));
            req = NULL;
            goto Exit;
        }
        req->valid = 1; 
    }

Exit:
    
    return req;
}

void cv_sdk_req_record_free(
        cv_sdk_req_record_t * req)
{
    OAM_ASSERT(req != NULL);

    os_sem_destroy(&req->sem);
    memset(req, 0x00, sizeof(cv_sdk_req_record_t));
    
}


cv_sdk_req_record_t * cv_sdk_req_record_find(uint32_t req_id)
{
    uint32_t i = 0 ; 
    cv_sdk_req_record_t * req = NULL;
    
    
    for(i = 0; i < CV_SDK_MAX_REQ; i++){
        if(!g_sdk_ctrl.reqs[i].valid){
            continue;
        }
        if(g_sdk_ctrl.reqs[i].req_id == req_id){
            req = &g_sdk_ctrl.reqs[i];
            break;
        }
    }

    return req;
}


int  cv_oam_sdk_api_request(cv_sdk_msg_t * msg)
{
    uint16_t len = 0;
    cv_sdk_req_record_t * req = NULL;

    OAM_ASSERT_RET(msg != NULL, OAM_E_PARAM);
    
    if(msg->sdk_id > g_sdk_ctrl.max_sdk_id){
        return OAM_E_WRONG_SDK_ID;
    }
    msg->ret =  OAM_E_OK;

    len = msg->len;
    /* convert data byte order */
    if(g_sdk_ctrl.xlats != NULL 
            && g_sdk_ctrl.xlats[msg->sdk_id] != NULL){
        g_sdk_ctrl.xlats[msg->sdk_id](
                msg->data,
                msg->len,
                CV_SDK_DATA_HTON);
    }
    
    msg->sdk_id = oam_htons(msg->sdk_id);
    msg->len = oam_htons(msg->len);

    os_mutex_lock(&g_sdk_ctrl.mutex);

    req = cv_sdk_req_record_alloc(msg);
    if(req == NULL){
        OAM_PRINTF("cv_sdk_req_record_alloc failed\r\n");
        msg->ret =  OAM_E_TOO_MUCH_API;
        goto Exit2;
    }

    msg->req_id = oam_htonl(req->req_id);
    msg->ret = cv_oam_client_send_msg(CV_OAM_FRAME_TYPE_CMD_MSG, CV_OAM_L2_MSG_CMD_REQ,
            (uint8_t *)msg, sizeof(cv_sdk_msg_t) - 1 + len);
    if(msg->ret != OAM_E_OK){
        goto Exit;
    }
    os_mutex_unlock(&g_sdk_ctrl.mutex);

    if(0 != os_sem_wait(&req->sem, CV_SDK_REQ_TIMEOUT)){
        msg->ret = OAM_E_TIMEOUT;
    }

    os_mutex_lock(&g_sdk_ctrl.mutex);
Exit:
    cv_sdk_req_record_free(req);
Exit2:
    os_mutex_unlock(&g_sdk_ctrl.mutex);
    return msg->ret;

}

void cv_oam_sdk_api_rsp(cv_sdk_msg_t * msg)
{
    uint32_t req_id = 0;
    cv_sdk_req_record_t * req = NULL;

    OAM_ASSERT(msg != NULL);

    req_id = oam_ntohl(msg->req_id);
    
    os_mutex_lock(&g_sdk_ctrl.mutex);
    req = cv_sdk_req_record_find(req_id);
    
    if(req == NULL || req->msg == NULL){
        OAM_PRINTF("no such req id %d \n", req_id);
        goto Exit;
    }

    msg->sdk_id = oam_ntohs(msg->sdk_id);
    msg->len = oam_ntohs(msg->len);
    msg->ret = oam_ntohl(msg->ret);


    if(msg->sdk_id > g_sdk_ctrl.max_sdk_id){
        OAM_PRINTF("invalid sdk id %d \n", msg->sdk_id);
    }

    /* convert data byte order */
    if(g_sdk_ctrl.xlats != NULL 
            && g_sdk_ctrl.xlats[msg->sdk_id] != NULL){
        g_sdk_ctrl.xlats[msg->sdk_id](
                msg->data,
                msg->len,
                CV_SDK_DATA_NTOH);
    }
    
    if(msg->len != oam_ntohs(req->msg->len)){
        OAM_PRINTF("len mismatch, expect %d , recv %d \n",
            oam_ntohs(req->msg->len),
            msg->len);
        goto Exit;
    }
    memcpy(req->msg, msg, sizeof(cv_sdk_msg_t) - 1 + msg->len); 

    os_sem_post(&req->sem);
Exit:
    os_mutex_unlock(&g_sdk_ctrl.mutex);

}


void cv_oam_sdk_init(cv_sdk_data_order_func_t * xlats, uint32_t max_sdk_id)
{
    uint32_t ret = 0 ;
    g_sdk_ctrl.max_sdk_id = max_sdk_id;
    g_sdk_ctrl.req_next = 0;

    /* make sure xlats was a global variable */
    g_sdk_ctrl.xlats = xlats;
    
    ret = os_mutex_create(&g_sdk_ctrl.mutex);
    if(ret != 0){
        OAM_LOG("Create mutex failed, ret = %d \n",ret);
        return;
    }

    memset(g_sdk_ctrl.reqs, 0x00,sizeof(g_sdk_ctrl.reqs));

}
