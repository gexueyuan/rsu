/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : test_api.h
 @brief  : test_api.h header file
 @author : wanglei
 @history:
           2015-12-8    wanglei    Created file
           ...
******************************************************************************/
#ifndef __TEST_API_H__
#define __TEST_API_H__

#include "oam_types.h"
#include "cv_vam.h"      /* include vam_config_t  define file */

/* Mirr Set Mask enum */
#define CV_OAM_VAM_CFG_MASK_BSM_HOPS          (1)      /* Bit[0]: Configure bsm hops */
#define CV_OAM_VAM_CFG_MASK_BSM_BCAST_MODE    (1 << 1) /* Bit[1]: Configure bsm broadcast mode */
#define CV_OAM_VAM_CFG_MASK_BSM_BCAST_PERIOD  (1 << 2) /* Bit[2]: Configure bsm broadcast period */

oam_status cv_oam_vam_cfg_set(uint32_t mask, vam_config_t *cfg);
oam_status cv_oam_vam_cfg_get(vam_config_t *cfg);

#endif
