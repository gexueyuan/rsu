/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : test_sdk_msg.h
 @brief  : test_sdk_msg.h header file
 @author : wanglei
 @history:
           2015-12-7    wanglei    Created file
           ...
******************************************************************************/

#ifndef __TEST_SDK_MSG_H__
#define __TEST_SDK_MSG_H__


#define  CV_SDK_REQ_TYPE_INFO \
            xx(0,  CV_SDK_VAM_PARAM_CFG_GET, cv_oam_vam_cfg_get_byte_reverse,   cv_oam_vam_cfg_get_cb) \
            xx(1,  CV_SDK_VAM_PARAM_CFG_SET, cv_oam_vam_cfg_set_byte_reverse,   cv_oam_vam_cfg_set_cb) \
            xx(2,  CV_SDK_VAM_BSM_TRIGGER, cv_oam_vam_bsm_trigger_byte_reverse,   cv_oam_vam_bsm_trigger_cb) \
            xx(3,  CV_SDK_VAM_SET_PRINT, cv_oam_vam_set_print_byte_reverse,   cv_oam_vam_set_print_cb) \
            xx(4,  CV_SDK_REQ_TYPE_END,    0,  0)


typedef enum
{
     #undef  xx
     #define xx(SEQ,TYPE, HTON_FUNC ,HANDLER_FUNC) TYPE,
        CV_SDK_REQ_TYPE_INFO
} cs_sdk_req_type_t;


#endif
