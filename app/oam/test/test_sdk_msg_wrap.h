/***********************************************************************/
/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : test_sdk_msg_wrap.h
 @brief  : test_sdk_msg_wrap.h header file
 @author : wanglei
 @history:
           2015-12-8    wanglei    Created file
           ...
******************************************************************************/

#ifndef __TEST_SDK_MSG_WRAP_H__
#define __TEST_SDK_MSG_WRAP_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include "oam_types.h"
#include "test_api.h"

typedef struct{
    vam_config_t cfg;
}__attribute__((packed)) cv_vam_cfg_get_msg_wrap_t;

typedef struct{
    uint32_t mask;
    vam_config_t cfg;
}__attribute__((packed)) cv_vam_cfg_set_msg_wrap_t;


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __TEST_SDK_MSG_WRAP_H__ */

