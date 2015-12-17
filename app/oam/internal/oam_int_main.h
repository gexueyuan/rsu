/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : oam_int_main.h
 @brief  : oam_int_main.h header file
 @author : wanglei
 @history:
           2015-12-4    wanglei    Created file
           ...
******************************************************************************/
#ifndef __OAM_INT_MAIN_H__
#define __OAM_INT_MAIN_H__

#include "cv_osal.h"
#include "oam_sdk_cmn.h"

typedef struct{
    uint32_t len;
    uint8_t  data[1];
}cv_oam_msg_t;

typedef struct{
    uint32_t  len;
    uint8_t   pkt[1];
}cv_oam_msg_pkt_t;


void cv_oam_init();

void *cv_oam_rx_thread(void *arg);

void cv_oam_hello_proc(
        unsigned char * pkt,
        unsigned int len);


#endif

