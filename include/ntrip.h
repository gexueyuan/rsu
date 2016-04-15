/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : ntrip.h
 @brief  : header of ntrip.c
 @author : gexueyuan
 @history:
           2016-4-5    gexueyuan    Created file
           ...
******************************************************************************/



/*****************************************************************************
 * declaration of variables and functions                                    *
*****************************************************************************/


/*****************************************************************************
 * implementation of functions                                               *
*****************************************************************************/
#ifndef __NTRIP_H__
#define __NTRIP_H__



#include "cv_osal.h"

#include "cv_cms_def.h"
#include "gps.h"


typedef struct _ntrip_gga {

    uint8_t flag_locate;

    char gga_buff[GPS_BUFF_SIZE];
    
} ntrip_gga_t;


void send_gga(char * gga_string,uint8_t flag_locate);

#endif

