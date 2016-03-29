/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : cv_sys_param.c
 @brief  : this file include the system parameter management
 @author : wangyifeng
 @history:
           2014-6-19    wangyifeng    Created file
           2014-7-28    gexueyuan     modified
           ...
******************************************************************************/

#include "cv_osal.h"
#define OSAL_MODULE_DEBUG
#define OSAL_MODULE_DEBUG_LEVEL OSAL_DEBUG_INFO
#define MODULE_NAME "param"
#include "cv_osal_dbg.h"
OSAL_DEBUG_ENTRY_DEFINE(param);

#include "cv_vam.h"
//#include "cv_vsa.h"
#include "cv_cms_def.h"
#include "cv_sys_param.h"



#if 0
extern  int drv_fls_erase(uint32_t sector);
extern  int drv_fls_read(uint32_t flash_address, uint8_t *p_databuf, uint32_t length);
extern  int drv_fls_write(uint32_t flash_address, uint8_t *p_databuf, uint32_t length);
#else

int drv_fls_erase(uint32_t sector)
{

    return 0;
}
int drv_fls_read(uint32_t flash_address, uint8_t *p_databuf, uint32_t length)
{
    return 0;
}
int drv_fls_write(uint32_t flash_address, uint8_t *p_databuf, uint32_t length)
{
    return 0;
}
#endif

/*****************************************************************************
 * declaration of variables and functions                                    *
*****************************************************************************/

cfg_param_t cms_param, *p_cms_param;

uint8_t param_init_words[] = "Vanet-param1";

uint16_t param_mode;
/*****************************************************************************
* implementation of functions                                               *
*****************************************************************************/

void load_default_param_custom(cfg_param_t *param)
{
    memset(param, 0 , sizeof(cfg_param_t));
    /******************** VAM *********************/
    param->vam.bsm_hops = 1; 
    param->vam.bsm_boardcast_mode = 2;  /* 0 - disable, 1 - auto, 2 - fixed period */
    param->vam.bsm_boardcast_saftyfactor = 5;  /* 1~10 */
    param->vam.bsm_pause_mode = 1;  /* 0 - disable, 1 - enable */
    param->vam.bsm_pause_hold_time = 5;  /* unit:s */
    param->vam.bsm_boardcast_period = 100;  /* 100~3000, unit:ms, min accuracy :10ms */
    
    param->vam.evam_hops = 1; 
    param->vam.evam_broadcast_type = 2;
    param->vam.evam_broadcast_peroid = 50;

    /******************** VSA *********************/
    param->vsa.danger_detect_speed_threshold = 60;  /* unit: km/h */
    param->vsa.lane_dis = 25;  /* unit:m, min accuracy :1m */
    
    param->vsa.crd_saftyfactor = 4;  /* 1~10 */
    param->vsa.crd_oppsite_speed = 20;/* <=255:30km/h*/
    param->vsa.crd_oppsite_rear_speed = 15;/* <=255:30km/h*/
    param->vsa.crd_rear_distance = 60;/*<=255:20m*/
    
    param->vsa.ebd_mode = 1;  /* 0 - disable, 1 - enable */
    param->vsa.ebd_acceleration_threshold = 3; /* unit:m/s2 */
    param->vsa.ebd_alert_hold_time = 5;  /* unit:s */

    /****************** wnet  *********************/
    param->wnet.channel = 13;
    param->wnet.txrate = 1;
    

}


void load_default_param_highway(cfg_param_t *param)
{
    memset(param, 0 , sizeof(cfg_param_t));

    /******************** VAM *********************/
    param->vam.bsm_hops = 1; 
    param->vam.bsm_boardcast_mode = 1;  /* 0 - disable, 1 - auto, 2 - fixed period */
    param->vam.bsm_boardcast_saftyfactor = 5;  /* 1~10 */
    param->vam.bsm_pause_mode = 1;  /* 0 - disable, 1 - enable */
    param->vam.bsm_pause_hold_time = 5;  /* unit:s */
    param->vam.bsm_boardcast_period = 100;  /* 100~3000, unit:ms, min accuracy :10ms */
    
    param->vam.evam_hops = 1; 
    param->vam.evam_broadcast_type = 2;
    param->vam.evam_broadcast_peroid = 50;

    /******************** VSA *********************/
    param->vsa.danger_detect_speed_threshold = 60;  /* unit: km/h */
    param->vsa.lane_dis = 25;  /* unit:m, min accuracy :1m */
    
    param->vsa.crd_saftyfactor = 4;  /* 1~10 */
    param->vsa.crd_oppsite_speed = 20;/* <=255:30km/h*/
    param->vsa.crd_oppsite_rear_speed = 15;/* <=255:30km/h*/
    param->vsa.crd_rear_distance = 60;/*<=255:20m*/
    
    param->vsa.ebd_mode = 1;  /* 0 - disable, 1 - enable */
    param->vsa.ebd_acceleration_threshold = 3; /* unit:m/s2 */
    param->vsa.ebd_alert_hold_time = 5;  /* unit:s */

    param->wnet.channel = 13;
    param->wnet.txrate = 1;
    

}


void load_default_param_mountain(cfg_param_t *param)
{
    memset(param, 0 , sizeof(cfg_param_t));

    /******************** VAM *********************/
    param->vam.bsm_hops = 1; 
    param->vam.bsm_boardcast_mode = 1;  /* 0 - disable, 1 - auto, 2 - fixed period */
    param->vam.bsm_boardcast_saftyfactor = 5;  /* 1~10 */
    param->vam.bsm_pause_mode = 1;  /* 0 - disable, 1 - enable */
    param->vam.bsm_pause_hold_time = 5;  /* unit:s */
    param->vam.bsm_boardcast_period = 100;  /* 100~3000, unit:ms, min accuracy :10ms */
    
    param->vam.evam_hops = 1; 
    param->vam.evam_broadcast_type = 2;
    param->vam.evam_broadcast_peroid = 50;

    /******************** VSA *********************/
    param->vsa.danger_detect_speed_threshold = 60;  /* unit: km/h */
    param->vsa.lane_dis = 25;  /*  unit:m, min accuracy :1m */
    
    param->vsa.crd_saftyfactor = 4;  /* 1~10 */
    param->vsa.crd_oppsite_speed = 20;/* <=255:30km/h*/
    param->vsa.crd_oppsite_rear_speed = 15;/* <=255:30km/h*/
    param->vsa.crd_rear_distance = 60;/*<=255:20m*/
    
    param->vsa.ebd_mode = 1;  /* 0 - disable, 1 - enable */
    param->vsa.ebd_acceleration_threshold = 3; /* unit:m/s2 */
    param->vsa.ebd_alert_hold_time = 5;  /* unit:s */

    param->wnet.channel = 13;
    param->wnet.txrate = 1;
    

}

void load_default_param_city(cfg_param_t *param)
{
    memset(param, 0 , sizeof(cfg_param_t));

    /******************** VAM *********************/
    param->vam.bsm_hops = 1; 
    param->vam.bsm_boardcast_mode = 1;  /* 0 - disable, 1 - auto, 2 - fixed period */
    param->vam.bsm_boardcast_saftyfactor = 5;  /* 1~10 */
    param->vam.bsm_pause_mode = 1;  /* 0 - disable, 1 - enable */
    param->vam.bsm_pause_hold_time = 5;  /* unit:s */
    param->vam.bsm_boardcast_period = 100;  /* 100~3000, unit:ms, min accuracy :10ms */
    
    param->vam.evam_hops = 1; 
    param->vam.evam_broadcast_type = 2;
    param->vam.evam_broadcast_peroid = 50;

    /******************** VSA *********************/
    param->vsa.danger_detect_speed_threshold = 60;  /* unit: km/h */
    param->vsa.lane_dis = 25;  /* unit:m, min accuracy :1m */
    
    param->vsa.crd_saftyfactor = 4;  /* 1~10 */
    param->vsa.crd_oppsite_speed = 20;/* <=255:30km/h*/
    param->vsa.crd_oppsite_rear_speed = 15;/* <=255:30km/h*/
    param->vsa.crd_rear_distance = 60;/*<=255:20m*/
    
    param->vsa.ebd_mode = 1;  /* 0 - disable, 1 - enable */
    param->vsa.ebd_acceleration_threshold = 3; /* unit:m/s2 */
    param->vsa.ebd_alert_hold_time = 5;  /* unit:s */

    param->wnet.channel = 13;
    param->wnet.txrate = 1;
    

}

void load_default_param(cfg_param_t *param)
{
    memset(param, 0 , sizeof(cfg_param_t));

    /******************ID************************/
    param->pid[0] = 0xcc;
    param->pid[1] = 0xcc;    
    param->pid[2] = 0xcc;
    param->pid[3] = 0xcc;
    /******************** VAM *********************/
    param->vam.bsm_hops = 1; 
    param->vam.bsm_boardcast_mode = 2;  /* 0 - disable, 1 - auto, 2 - fixed period */
    param->vam.bsm_boardcast_saftyfactor = 5;  /* 1~10 */
    param->vam.bsm_pause_mode = 1;  /* 0 - disable, 1 - enable */
    param->vam.bsm_pause_hold_time = 5;  /* unit:s */
    param->vam.bsm_boardcast_period = 100;  /* 100~3000, unit:ms, min accuracy :10ms */
    
    param->vam.evam_hops = 1; 
    param->vam.evam_broadcast_type = 2;
    param->vam.evam_broadcast_peroid = 50;

    /******************** VSA *********************/
    param->vsa.danger_detect_speed_threshold = 60;  /* unit: km/h */
    param->vsa.lane_dis = 25;  /* unit:m, min accuracy :1m */
    
    param->vsa.crd_saftyfactor = 4;  /* 1~10 */
    param->vsa.crd_oppsite_speed = 20;/* <=255:30km/h*/
    param->vsa.crd_oppsite_rear_speed = 15;/* <=255:30km/h*/
    param->vsa.crd_rear_distance = 60;/*<=255:20m*/
    
    param->vsa.ebd_mode = 1;  /* 0 - disable, 1 - enable */
    param->vsa.ebd_acceleration_threshold = 3; /* unit:m/s2 */
    param->vsa.ebd_alert_hold_time = 5;  /* unit:s */


    param->wnet.channel = 13;
    param->wnet.txrate = 1;
    

}

uint32_t get_param_addr(uint16_t mode)
{
    uint32_t param_addr;
  
    if(param_mode == CUSTOM_MODE)
        param_addr = PARAM_ADDR_CUSTOM;
    else if(param_mode == HIGHWAY_MODE)
        param_addr = PARAM_ADDR_HIGHWAY;
    else if(param_mode == MOUNTAIN_MODE)
        param_addr = PARAM_ADDR_MOUNTAIN;
    else if(param_mode == CITY_MODE)
        param_addr = PARAM_ADDR_CITY;

    return param_addr;

}


uint8_t get_param_num(uint16_t mode)
{
    uint8_t param_no;
  
    if(param_mode == CUSTOM_MODE){
        param_no = 0;
    }
    else if(param_mode == HIGHWAY_MODE){
        param_no = 1;
    }
    else if(param_mode == MOUNTAIN_MODE){
        param_no = 2;
    }
    else if(param_mode == CITY_MODE){
        param_no = 3;
    }

    return param_no;

}

uint16_t  mode_get(void)
{
    uint16_t mode;
    uint8_t get_str;
    //osal_printf("----------------------mode---------------------\n");
    drv_fls_read(PARAM_MODE_ADDR,(uint8_t*)&mode,sizeof(uint16_t));

    if(mode == CUSTOM_MODE){
        get_str = 0;
    }
    else if(mode == HIGHWAY_MODE){
        get_str = 1;
    }
    else if(mode == MOUNTAIN_MODE){
        get_str = 2;
    }
    else if(mode == CITY_MODE){
        get_str = 3;
    }
    else{ 
        OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"mode reading error!!\n");
    }
    
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"mode value = %04X,mode is %s\n", mode,mode_string[get_str]);

    return mode;

}

void mode_set(uint16_t mode)
{
    cfg_param_t all_param[4];
    param_mode = mode;
    
    drv_fls_read(PARAM_ADDR_CUSTOM,(uint8_t*)&all_param[0],sizeof(cfg_param_t));
    drv_fls_read(PARAM_ADDR_HIGHWAY,(uint8_t*)&all_param[1],sizeof(cfg_param_t));
    drv_fls_read(PARAM_ADDR_MOUNTAIN,(uint8_t*)&all_param[2],sizeof(cfg_param_t));
    drv_fls_read(PARAM_ADDR_CITY,(uint8_t*)&all_param[3],sizeof(cfg_param_t));



    drv_fls_erase(PARAM_SECTOR);
    drv_fls_write(PARAM_FLAG_ADDR,param_init_words,sizeof(param_init_words));    
    drv_fls_write(PARAM_MODE_ADDR,(uint8_t *)&param_mode,sizeof(uint16_t));
    drv_fls_write(PARAM_ADDR_CUSTOM,(uint8_t*)&all_param[0],sizeof(cfg_param_t));
    drv_fls_write(PARAM_ADDR_HIGHWAY,(uint8_t*)&all_param[1],sizeof(cfg_param_t));
    drv_fls_write(PARAM_ADDR_MOUNTAIN,(uint8_t*)&all_param[2],sizeof(cfg_param_t));
    drv_fls_write(PARAM_ADDR_CITY,(uint8_t*)&all_param[3],sizeof(cfg_param_t));
    osal_printf("mode = %d  write in flash\n", param_mode);

}

void mode_change(uint8_t mode_num)
{
    uint16_t mode_setting;
    
    if(mode_num == 1){
        mode_setting = CUSTOM_MODE;
    }
    else if(mode_num == 2){
        mode_setting = HIGHWAY_MODE;
    }
    else if(mode_num == 3){
        mode_setting = MOUNTAIN_MODE;
    }
    else if(mode_num == 4){
        mode_setting = CITY_MODE;
    }

    mode_set(mode_setting);

}

void load_param_from_fl(void)
{
    uint32_t param_addr;
    p_cms_param = &cms_param;
    
    if(param_mode == CUSTOM_MODE){
        param_addr = PARAM_ADDR_CUSTOM;
    }
    else if(param_mode == HIGHWAY_MODE){
        param_addr = PARAM_ADDR_HIGHWAY;
    }
    else if(param_mode == MOUNTAIN_MODE){
        param_addr = PARAM_ADDR_MOUNTAIN;
    }
    else if(param_mode == CITY_MODE){
        param_addr = PARAM_ADDR_CITY;
    }
    
    drv_fls_read(param_addr,(uint8_t *)p_cms_param,sizeof(cfg_param_t));
    

}

void  write_def_param(void)
{
    cfg_param_t  flash_param;

    param_mode = CUSTOM_MODE;
    
    drv_fls_erase(PARAM_SECTOR);
    drv_fls_write(PARAM_FLAG_ADDR,param_init_words,sizeof(param_init_words));
    drv_fls_write(PARAM_MODE_ADDR,(uint8_t *)&param_mode,sizeof(uint16_t));

    
    load_default_param_custom(&flash_param);
    drv_fls_write(PARAM_ADDR_CUSTOM,(uint8_t *)&flash_param,sizeof(cfg_param_t));

    
    memset(&flash_param,0,sizeof(cfg_param_t));
    load_default_param_highway(&flash_param);
    drv_fls_write(PARAM_ADDR_HIGHWAY,(uint8_t *)&flash_param,sizeof(cfg_param_t));

    
    memset(&flash_param,0,sizeof(cfg_param_t));
    load_default_param_mountain(&flash_param);
    drv_fls_write(PARAM_ADDR_MOUNTAIN,(uint8_t *)&flash_param,sizeof(cfg_param_t));

    
    memset(&flash_param,0,sizeof(cfg_param_t));
    load_default_param_city(&flash_param);
    drv_fls_write(PARAM_ADDR_CITY,(uint8_t *)&flash_param,sizeof(cfg_param_t));
/*
    if(-1 == err)
        osal_printf("error happened when writing default param to flash");
    else
        osal_printf("write default param to flash  success\n");
*/

}




void param_init(void)
{
#if 1
    p_cms_param = &cms_param;
    load_default_param(p_cms_param);
#else
    uint8_t magic_word[sizeof(param_init_words)];    
    drv_fls_read(PARAM_FLAG_ADDR,magic_word,sizeof(param_init_words));
    param_mode = mode_get();
    //drv_fls_read(PARAM_MODE_ADDR,(uint8_t*)&param_mode,sizeof(uint16_t));
    if(strcmp((const char*)param_init_words,(const char*)magic_word) != 0){
            p_cms_param = &cms_param;
            osal_printf("loading default param,mode is custom\n");
            load_default_param_custom(p_cms_param);            
            write_def_param();
    }
    else{
        load_param_from_fl();
    }
#endif
}

void param_get(void)
{
    //cfg_param_t  *param_temp;

    //drv_fls_read(PARAM_ADDR,(uint8_t *)param_temp,sizeof(cfg_param_t));
        
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"-------------------parameters in ram------------------\n");
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"work mode is %d\n",param_mode);
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"----------------------vam---------------------\n");
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"ID(0)=%d%d%d%d\n",p_cms_param->pid[0],p_cms_param->pid[1],p_cms_param->pid[2],p_cms_param->pid[3]);
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"vam.bsm_hops(1)=%d\n", p_cms_param->vam.bsm_hops);
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"vam.bsm_boardcast_mode(2)=%d\n", p_cms_param->vam.bsm_boardcast_mode);
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"vam.bsm_boardcast_saftyfactor(3)=%d\n", p_cms_param->vam.bsm_boardcast_saftyfactor);
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"vam.bsm_pause_mode(4)=%d\n", p_cms_param->vam.bsm_pause_mode);
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"vam.bsm_pause_hold_time(5)=%d (s)\n", p_cms_param->vam.bsm_pause_hold_time);
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"vam.bsm_boardcast_period(6)=%d (ms)\n", p_cms_param->vam.bsm_boardcast_period);

    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"vam.evam_hops(7)=%d\n", p_cms_param->vam.evam_hops);
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"vam.evam_broadcast_type(8)=%d\n", p_cms_param->vam.evam_broadcast_type);
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"vam.evam_broadcast_peroid(9)=%d (ms)\n\n", p_cms_param->vam.evam_broadcast_peroid);

    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"----------------------vsa---------------------\n");
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"vsa.danger_detect_speed_threshold(10)=%d (km/h)\n", p_cms_param->vsa.danger_detect_speed_threshold);
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"vsa.lane_dis(11)=%d (m)\n", p_cms_param->vsa.lane_dis);
    
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"vsa.crd_saftyfactor(12)=%d\n", p_cms_param->vsa.crd_saftyfactor);
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"vsa.crd_oppsite_speed(13)=%d (km/h)\n", p_cms_param->vsa.crd_oppsite_speed);
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"vsa.crd_oppsite_rear_speed(14)=%d (km/h)\n", p_cms_param->vsa.crd_oppsite_rear_speed);
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"vsa.crd_rear_distance(15)=%d (m)\n", p_cms_param->vsa.crd_rear_distance);

        
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"vsa.ebd_mode(16)=%d\n", p_cms_param->vsa.ebd_mode);
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"vsa.ebd_acceleration_threshold(17)=%d (m/s2)\n", p_cms_param->vsa.ebd_acceleration_threshold);
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"vsa.ebd_alert_hold_time(18)=%d (s)\n\n", p_cms_param->vsa.ebd_alert_hold_time);


    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"----------------------wnet---------------------\n");
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"wnet.channel(29)=%d\n",p_cms_param->wnet.channel);
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"wnet.txrate(30)=%d\n",p_cms_param->wnet.txrate);
    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"...\n");

    OSAL_MODULE_DBGPRT(MODULE_NAME,OSAL_DEBUG_INFO,"----------------------end---------------------\n");
}


void print_init_word(void)//print  flag of initialized
{
    
    uint8_t init_word[sizeof(param_init_words)];


    drv_fls_read(PARAM_FLAG_ADDR,init_word,sizeof(param_init_words));

    osal_printf("init word in flash is \"%s\"\n",init_word);

}

int param_set(uint8_t param, int32_t value)
{

    int err;

    cfg_param_t *cfg_param;

    uint16_t mode;

    uint8_t param_num;

    cfg_param_t all_param[4];

    
    drv_fls_read(PARAM_ADDR_CUSTOM,(uint8_t*)&all_param[0],sizeof(cfg_param_t));
    drv_fls_read(PARAM_ADDR_HIGHWAY,(uint8_t*)&all_param[1],sizeof(cfg_param_t));
    drv_fls_read(PARAM_ADDR_MOUNTAIN,(uint8_t*)&all_param[2],sizeof(cfg_param_t));
    drv_fls_read(PARAM_ADDR_CITY,(uint8_t*)&all_param[3],sizeof(cfg_param_t));
    
    
    
    drv_fls_read(PARAM_MODE_ADDR,(uint8_t *)&mode,sizeof(uint16_t));

    param_num = get_param_num(mode);

    osal_printf("mode is %04X\n",mode);

    cfg_param = &all_param[param_num];


    if(param == 29){
        if((value < 1)&&(value > 13)){
            osal_printf("wrong channel!\n\n");
            return -1;
        }
    }

   if(param == 30){
        if((value != 1)&&(value != 2)&&(value != 6)){
            osal_printf("param of Txrate is just 1,2,6\n\n");
            return -1;
        }

   }
                
    switch(param){

    case 0:
        if(value > 9999){
                osal_printf("invalid  ID!!\n");
                return -1;
        }    
        cfg_param->pid[0] = value/1000;
        cfg_param->pid[1] = (value%1000)/100;
        cfg_param->pid[2] = ((value%1000)%100)/10;
        cfg_param->pid[3] =    ((value%1000)%100)%10;
        break;

    case 1:
        cfg_param->vam.bsm_hops = value;
        break;
    case 2:
        cfg_param->vam.bsm_boardcast_mode = value;
        break;        
    case 3:
        cfg_param->vam.bsm_boardcast_saftyfactor = value;
        break;
    case 4:
        cfg_param->vam.bsm_pause_mode = value;
        break;
    case 5:
        cfg_param->vam.bsm_pause_hold_time = value;
        break;
    case 6:
        cfg_param->vam.bsm_boardcast_period = value;
        break;
        
    case 7:
        cfg_param->vam.evam_hops = value;
        break;
    case 8:
        cfg_param->vam.evam_broadcast_type = value;
        break;
    case 9:
        cfg_param->vam.evam_broadcast_peroid = value;
        break;            


        
    case 10:
        cfg_param->vsa.danger_detect_speed_threshold = value;
        break;
    case 11:
        cfg_param->vsa.lane_dis = value;
        
        break;            
    case 12:
        cfg_param->vsa.crd_saftyfactor = value;
        break;
    case 13:
        cfg_param->vsa.crd_oppsite_speed = value;
        break;
    case 14:
        cfg_param->vsa.crd_oppsite_rear_speed = value;
        break;
    case 15:
        cfg_param->vsa.crd_rear_distance = value;
        break;

        
    case 16:
        cfg_param->vsa.ebd_mode = value;
        break;
    case 17:
        cfg_param->vsa.ebd_acceleration_threshold = value;
        break;            
    case 18:
        cfg_param->vsa.ebd_alert_hold_time = value;
        break;
        
    case 29:
        cfg_param->wnet.channel = value;
        break;

    case 30:
        cfg_param->wnet.txrate = value;
        break;
    case 31:
        cfg_param->print_xxx = value;
        break;
    default:          
        osal_printf("invalid  parameter  number!!\n");
        break;

    }

    memcpy((uint8_t*)p_cms_param,(uint8_t*)cfg_param,sizeof(cfg_param_t));


    osal_printf("param is setting .....please don't power off!\n");
        
    drv_fls_erase(PARAM_SECTOR);
    drv_fls_write(PARAM_FLAG_ADDR, param_init_words,sizeof(param_init_words));
    drv_fls_write(PARAM_MODE_ADDR, (uint8_t *)&mode,sizeof(uint16_t));
    
    drv_fls_write(PARAM_ADDR_CUSTOM, (uint8_t*)&all_param[0],sizeof(cfg_param_t));
    drv_fls_write(PARAM_ADDR_HIGHWAY, (uint8_t*)&all_param[1],sizeof(cfg_param_t));
    drv_fls_write(PARAM_ADDR_MOUNTAIN, (uint8_t*)&all_param[2],sizeof(cfg_param_t));
    err = drv_fls_write(PARAM_ADDR_CITY, (uint8_t*)&all_param[3],sizeof(cfg_param_t));

    if(err == -1){
        osal_printf("parameter writing process error!!!\n");
    }
    else{
        osal_printf("parameter set success!\n");
    }

    cfg_param = NULL;

    return 0;

}


int erase_sector4(void)
{
    int err = 0;
#if 0
    err = drv_fls_erase(FLASH_Sector_4);

    drv_fls_write(0x8010000,param_init_words,sizeof(param_init_words));
    drv_fls_write(0x8010010,param_init_words,sizeof(param_init_words));
    
    drv_fls_write(0x8010020,param_init_words,sizeof(param_init_words));
    drv_fls_write(0x8010220,param_init_words,sizeof(param_init_words));
    drv_fls_write(0x8010420,param_init_words,sizeof(param_init_words));
    err = drv_fls_write(0x8010620,param_init_words,sizeof(param_init_words));
    return err;
#endif
}


#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(mode_get, get mode);
FINSH_FUNCTION_EXPORT(mode_set, set mode by hex);
FINSH_FUNCTION_EXPORT(mode_change, set mode by num:1-custom;2-highway;3-mountain;4-city);
FINSH_FUNCTION_EXPORT(write_def_param, debug:write default  param to flash);
FINSH_FUNCTION_EXPORT(param_get, get system parameters);
FINSH_FUNCTION_EXPORT(print_init_word, print init words  in flash);
FINSH_FUNCTION_EXPORT(param_set, set system parameters);
FINSH_FUNCTION_EXPORT(erase_sector4, set system parameters);
#endif

