/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : sdk_util.h
 @brief  : sdk_util.h header file
 @author : wanglei
 @history:
           2015-12-4    wanglei    Created file
           ...
******************************************************************************/
#ifndef __SDK_UTIL_H__
#define __SDK_UTIL_H__

#include "oam_cmn.h"
#include "oam_types.h"
#include "oam_sdk_cmn.h"


uint16_t cv_short_byte_reverse(uint16_t data, int8_t type);
uint32_t cv_int_byte_reverse(uint32_t data, int8_t type);
uint64_t cv_long_byte_reverse(uint64_t data, int8_t type);
double cv_double_byte_reverse(double data, int8_t type);
void cv_ipv6_byte_reverse(oam_ipv6_t ipv6, int8_t type);

#endif
