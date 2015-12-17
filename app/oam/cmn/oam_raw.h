/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : oam_raw.h
 @brief  : header file
 @author : wanglei
 @history:
           2015-12-4    wanglei    Created file
           ...
******************************************************************************/

#ifndef __OAM_RAW_H__
#define __OAM_RAW_H__
#include "oam_cmn.h"
#include "oam_types.h"

#define CV_RAW_DATA_TYPE_NEMA  0x0001
#define CV_RAW_DATA_TYPE_DSRC  0x0002
#define CV_RAW_DATA_TYPE_USER  0x0003

typedef struct
{
    uint32_t  data_type;
    uint16_t  len;
    uint8_t   data[1];
} __attribute__((packed)) cv_raw_pkt_msg_t;

#endif
