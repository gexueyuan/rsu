/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : oam_int_raw.c
 @brief  : raw data pkt process
 @author : wanglei
 @history:
           2015-12-8    wanglei    Created file
           ...
******************************************************************************/
#include "oam_raw.h"
#include "oam_cmn.h"

void cv_oam_raw_data_send(
        uint32_t destfd,
        uint32_t data_type,
        uint16_t len,
        uint8_t* data)
{
    uint8_t            pkt[CV_OAM_PKT_MAX_LEN] = {0};
    cv_oam_frame_t *   frame = (cv_oam_frame_t *)pkt;
    cv_raw_pkt_msg_t *   msg = (cv_raw_pkt_msg_t*) frame->data;
    uint32_t           pkt_len = 0;
    OAM_ASSERT(pkt != NULL);

    if(len > 
            (CV_OAM_PKT_MAX_LEN 
                - CV_FRAME_HEADER_LEN 
                - sizeof(cv_raw_pkt_msg_t) + 1)){
        OAM_LOG("pkt len error , len = %d \n",len);
        return ;
    }

    frame->frame_type = oam_htons(CV_OAM_FRAME_TYPE_RAW_DATA);
    pkt_len = CV_FRAME_HEADER_LEN;

    msg->data_type = oam_htonl(data_type);
    msg->len = oam_htons(len);
    
    pkt_len += sizeof(cv_raw_pkt_msg_t) - 1;
    memcpy(msg->data, data, len);
    pkt_len += len;

    //cv_oam_pkt_send(destfd, pkt, pkt_len);
}


int cv_oam_raw_data_proc(uint16_t type, uint8_t *data, uint32_t len)
{

    return 0;   
}