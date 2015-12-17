/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : oam_int_sdk.c
 @brief  : oam server process sdk msg
 @author : wanglei
 @history:
           2015-12-8    wanglei    Created file
           ...
******************************************************************************/

#include "oam_cmn.h"
#include "oam_sdk_cmn.h"
#include "oam_int_sdk.h"

cv_sdk_api_ctrl_t g_sdk_ctrl;

void cv_oam_sdk_msg_handler(void *src, cv_sdk_msg_t * msg)
{
    uint32_t  ret;
    uint16_t  sdk_id;
    uint16_t  len;
    int clientfd = *(int *)src;
    OAM_ASSERT(msg != NULL); 

    sdk_id = oam_ntohs(msg->sdk_id);
    len = oam_ntohs(msg->len);
    
    if(sdk_id >= g_sdk_ctrl.max_sdk_id){
        OAM_LOG("invalid sdk id %d \n",sdk_id);
        return;
    }
    
    if(g_sdk_ctrl.xlats == NULL 
            || g_sdk_ctrl.xlats[sdk_id] == NULL
            || g_sdk_ctrl.handlers == NULL 
            || g_sdk_ctrl.handlers [sdk_id] == NULL
            ){
        OAM_LOG("not found the handler ,id = %d \n",sdk_id);
        return;
    }

    g_sdk_ctrl.xlats[sdk_id](msg->data, len, CV_SDK_DATA_NTOH);

    ret = g_sdk_ctrl.handlers[sdk_id](msg->data, len);
    
    msg->ret = oam_htonl(ret);

    g_sdk_ctrl.xlats[sdk_id](msg->data, len, CV_SDK_DATA_HTON);

    cv_oam_send_msg(clientfd, CV_OAM_L2_MSG_CMD_RSP, (uint8_t *)msg,
            oam_ntohs(msg->len) + sizeof(cv_sdk_msg_t) - 1 ); 
}


int cv_oam_send_msg(int destfd, uint16_t msg_type, uint8_t * msg, uint16_t msg_len)
{
    uint8_t            pkt[CV_OAM_PKT_MAX_LEN] = {0};
    cv_oam_frame_t *  frame = (cv_oam_frame_t *)pkt;
    int ret = OAM_OK;

    OAM_ASSERT_RET(msg != NULL, OAM_E_PARAM);

    if(msg_len > CV_OAM_PKT_MAX_LEN - CV_FRAME_HEADER_LEN){
        return OAM_E_PARAM;
    }

    frame->frame_type = oam_htons(CV_OAM_FRAME_TYPE_CMD_MSG);
    frame->layer2_type.cmd_type = oam_htons(msg_type);

    memcpy(frame->data, msg, msg_len);

    ret = send(destfd, pkt, msg_len + CV_FRAME_HEADER_LEN, 0);
    if (ret < 0) {
        fprintf(stderr, "send to client failed: %s\r\n", strerror(errno));
        return OAM_E_SOCKET_ERROR;
    }
   
    return OAM_E_OK;
}


void cv_oam_sdk_init(cv_sdk_data_order_func_t * xlats,
        cv_sdk_data_handler_func_t * handlers, uint32_t max_sdk_id)
{
    g_sdk_ctrl.xlats = xlats;
    g_sdk_ctrl.handlers = handlers;
    g_sdk_ctrl.max_sdk_id = max_sdk_id;
}


