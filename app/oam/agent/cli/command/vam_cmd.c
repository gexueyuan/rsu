/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : vam_cmd.c
 @brief  : vam command
 @author : wanglei
 @history:
           2015-12-8    wanglei    Created file
           ...
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "readline.h"
#include "history.h"
#include "command.h"
#include "cli_user_lib.h"
#include "vector.h"
#include "vty.h"
#include "command.h"
#include "cli_user_lib.h"
#include "cli_lib.h"

#include "os_core.h"
#include "oam_cmn.h"


static int show_vam_config(struct vty *vty)
{
    int ret = 0;
    vam_config_t cfg;
    ret = cv_oam_vam_cfg_get(&cfg);
    if (ret) {
        return CMD_WARNING;
    }

    VTY_OUT_EC(vty, 
                "%sbsm broadcast hops    : %d%s"
                "%sbsm broadcast mode    : %d%s"
                "%sbsm broadcast period  : %d%s",
                "%sbsm消息跳数           : %d%s"
                "%sbsm消息发送方式       : %d%s"
                "%sbsm消息发送间隔(ms)   : %d%s",
           VTY_LINE_HEAD, cfg.bsm_hops, VTY_NEWLINE,
           VTY_LINE_HEAD, cfg.bsm_boardcast_mode, VTY_NEWLINE,
           VTY_LINE_HEAD, cfg.bsm_boardcast_period, VTY_NEWLINE);
    
    return CMD_SUCCESS;
}


DEFUN
(
    show_config_info,
    show_config_cmd,
    "show parameter (all|vam|wnet)",
    SHOW_TIPS_EN SHOW_TIPS_CH
    "working parameter\n配置参数\n"
    "all config info\n所有配置参数信息\n"
    "vam config info\nvam配置参数信息\n"
    "wireless net config info\n无线网络配置参数信息\n",
    CMD_LEVEL_MONITOR_10
)
{
    switch (argv[0][0])
    {
        /* all */
        case 'a':
        {
            //TBD;
            break;
        }

        /* vam */
        case 'v':
        {
            return show_vam_config(vty);
            break;
        }

        /* wnet */
        case 'w':
        {
            //TBD;
            break;
        }

        default:
            return CMD_WARNING;
    }
    return CMD_SUCCESS;
}



DEFUN
(
    set_config_info,
    set_config_cmd,
    "set parameter bsm_period <10-3000>",
    SET_TIPS_EN SET_TIPS_CH
    "working parameter\n运行参数\n"
    "bsm broadcast period\n发送间隔\n"
    "period(ms)\n时间(毫秒)\n",
    CMD_LEVEL_MONITOR_10
)
{
    int ret = 0;
    int period = 0;
    vam_config_t cfg;

    int mask = CV_OAM_VAM_CFG_MASK_BSM_BCAST_PERIOD;

    period = atoi(argv[0]);
    cfg.bsm_boardcast_period = period;
    
    ret = cv_oam_vam_cfg_set(mask, &cfg);
    if (ret != OAM_OK) {
        VTY_SET_UNSUCCESSFULLY(vty);
        return CMD_WARNING;

    }
    
    return CMD_SUCCESS;
}


void install_vam_manage_cmd()
{
    install_element(ENABLE_NODE, &show_config_cmd);
    install_element(ENABLE_NODE, &set_config_cmd);
    return;
}

