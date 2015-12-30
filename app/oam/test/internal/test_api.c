/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : test_api.c
 @brief  : test 
 @author : wanglei
 @history:
           2015-12-4    wanglei    Created file
           ...
******************************************************************************/

#include <stdio.h>
#include "test_api.h"

oam_status cv_oam_vam_cfg_set(uint32_t mask, vam_config_t *cfg)
{
    vam_config_t oldcfg;
    vam_get_config(&oldcfg);

    if (mask & CV_OAM_VAM_CFG_MASK_BSM_BCAST_MODE) {
        oldcfg.bsm_boardcast_mode = cfg->bsm_boardcast_mode;
    }

    if (mask & CV_OAM_VAM_CFG_MASK_BSM_BCAST_PERIOD) {
        oldcfg.bsm_boardcast_period = cfg->bsm_boardcast_period;
    }

    vam_set_config(&oldcfg);
    return 0;
}

oam_status cv_oam_vam_cfg_get(vam_config_t *cfg)
{
    vam_get_config(cfg);
    return 0;
}

oam_status cv_oam_vam_bsm_trigger(uint8_t trigger)
{
    if(trigger){
        vam_start();
    }
    else{
        
        vam_stop();
    }
    return 0;
}

oam_status cv_oam_vam_set_print(int type)
{
    vam_set_print(type);
    return 0;
}

