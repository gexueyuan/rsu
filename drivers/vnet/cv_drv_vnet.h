/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : cv_drv_vnet.h
 @brief  : vnet driver header file
 @author : wanglei
 @history:
           2015-12-3    wanglei    Created file
           ...
******************************************************************************/
#ifndef __CV_DRV_VNET_H__
#define __CV_DRV_VNET_H__

#ifdef VNET_DRIVER_TYPE_WIFI
#include "cv_drv_wifi.h"
#endif

#ifdef VNET_DRIVER_TYPE_LTE
#include "cv_drv_lte.h"
#endif



#endif /* __CV_DRV_VNET_H__ */
