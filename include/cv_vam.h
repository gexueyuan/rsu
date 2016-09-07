/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : cv_vam.h
 @brief  : the definition of vehicle application middleware
 @author : wangyifeng
 @history:
           2014-6-17    wangyifeng    Created file
           ...
******************************************************************************/
#ifndef __CV_VAM_H__
#define __CV_VAM_H__

#include "list.h"
#include "cv_osal.h"

#include "nmea.h"
#include "cv_rcp.h"
#include "app_msg_format.h"

#define VAM_ABS(x)                    (x < 0) ? (-x) : x

/*****************************************************************************
 * definition of micro                                                       *
*****************************************************************************/
#define RCP_TEMP_ID_LEN 4
#define RCP_MACADR_LEN 8

#define RCP_MSG_ID_BSM   DSRCmsgID_basicSafetyMessage_D
#define RCP_MSG_ID_EVAM  DSRCmsgID_emergencyVehicleAlert_D
#define RCP_MSG_ID_RSA   DSRCmsgID_roadSideAlert_D


#define BSM_BC_MODE_DISABLE 0
#define BSM_BC_MODE_AUTO    1
#define BSM_BC_MODE_FIXED   2

#define VAM_FLAG_RX           (0x0001)
#define VAM_FLAG_TX_BSM       (0x0002)    /* Send bsm flag. */
#define VAM_FLAG_TX_BSM_ALERT (0x0004)
#define VAM_FLAG_TX_BSM_PAUSE (0x0008)
#define VAM_FLAG_TX_EVAM      (0x0010)
#define VAM_FLAG_GPS_FIXED    (0x0020)    /* gps captured flag. */
#define VAM_FLAG_XXX          (0x0040)

/* for test roadside alert */
#define VAM_FLAG_TX_RSA       (0x0100)

#define VAM_NEIGHBOUR_MAXNUM   (80)
#define VAM_NEIGHBOUR_MAXLIFE  (5000)  /* unit: s. */


/* BEGIN: Added by wanglei, 2014/8/1 */
#define VAM_REMOTE_ALERT_MAXLIFE  (5)  //unit: second
#define VAM_NO_ALERT_EVAM_TX_TIMES (5) //取消所有警告的evam消息发送次数

/* END:   Added by wanglei, 2014/8/1 */


/* vehicle alert, need to conver to Eventflag when sended by bsm msg */
typedef enum _VAM_ALERT_MASK {
    VAM_ALERT_MASK_VBD = 0x01,   /* vehicle break down */
    VAM_ALERT_MASK_EBD = 0x02,   /* vehicle emergency brake down */
    VAM_ALERT_MASK_VOT = 0x04,   /* vehicle overturned */
} E_VAM_ALERT_MASK;

/* roadside alert and eva alert type. need to convert to ITIScode when sended by rsa */
/* 弯道/交叉口会车危险;
   特殊道路提示（急弯/隧道/桥梁/十字路口/...）;
   电子交通标识提示（信号灯/限速/...） */

#define  VAM_RSA_TYPE_2_ITISCODE \
        xx(0, RSA_TYPE_CROSS_WITH_CARE,     0x1C07) \
        xx(1, RSA_TYPE_CURVE,               0x1F5A) \
        xx(2, RSA_TYPE_TUNNEL,              0x2029) \
        xx(3, RSA_TYPE_BRIDGE,              0x2025) \
        xx(4, RSA_TYPE_INTERSECTION,        0x1F60) \
        xx(5, RSA_TYPE_SIGNAL_LIGHT,        0x1D07) \
        xx(6, RSA_TYPE_SPEED_RESTRICTION,   0x0A04) \
        xx(7, RSA_TYPE_EMERGENCY_VEHICLE,   0x0704) \
        xx(8, RSA_TYPE_MAX,                 0x0000) 

typedef enum _VAM_RSA_TYPE
{
 #undef  xx
 #define xx(SEQ, TYPE, ITISCODE) TYPE,
    VAM_RSA_TYPE_2_ITISCODE
} E_VAM_RSA_TYPE;


enum VAM_EVT{
    VAM_EVT_LOCAL_UPDATE = 0,
    VAM_EVT_PEER_UPDATE,
    VAM_EVT_PEER_ALARM,
    VAM_EVT_GPS_STATUS,
    VAM_EVT_GSNR_EBD_DETECT, 

    VAM_EVT_BSM_ALARM_UPDATE,    /* Received bsm alarm message. */
    VAM_EVT_RSA_UPDATE, 
    VAM_EVT_EVA_UPDATE, 
    VAM_EVT_MAX
};

/*****************************************************************************
 * definition of struct                                                      *
*****************************************************************************/

/* Save all the compiler settings. */
#pragma pack(push)

/* store data to reduce data size and off the optimization. */
#pragma pack(1)



/* Vehicle position accuracy structure. */
typedef struct _vam_pos_accuracy_t
{
    /* Radius of semi-major axis of an ellipsoid. Unit: meter. */
    float        semi_major_accu;

    /* Radius of semi-minor axis of an ellipsoid. Unit: meter. */
    float        semi_minor_accu;

    /* Angle of semi-major axis of an ellipsoid. Unit: degree. */
    float semi_major_orientation;

}vam_pos_accuracy_t, * vam_pos_accuracy_t_ptr;

#define VAM_POS_ACCURACY_T_LEN    (sizeof(vam_pos_accuracy_t))


/* Vehicle position structure. */
typedef struct _vam_position_t
{
    /* Geographic latitude. Unit: degree. */
    double  latitude;

    /* Geographic longitude. Unit: degree. */
    double longitude;

    /* Geographic position above or below the reference ellipsoid. Unit: meter. */
    float  elevation;
    
}vam_position_t, * vam_position_t_ptr;

#define VAM_POSITION_T_LEN    (sizeof(vam_position_t))


/* Acceleration set 4 way. */
typedef struct _vam_acceset_t
{   
    /* Along the vehicle longitudinal axis. Unit: m/s^2. */
    float  longitudinal;

    /* Along the vehicle lateral axis. Unit: m/s^2. */
    float       lateral;

    /* Alone the vehicle vertical axis. Unit: m/s^2. */
    float      vertical;

    /* Vehicle's yaw rate. Unit: degrees per second. */
    float      yaw_rate;
    
}vam_acceset_t, * vam_acceset_t_ptr;

#define VAM_ACCESET_T_LEN    (sizeof(vam_acceset_t))


/* Vehicle size structure. */
typedef struct _vam_vehicle_size_t
{
    /* Vehicle size unit: m. */
    float   vec_width;
    float  vec_length;

}vam_vehicle_size_t, * vam_vehicle_size_t_ptr;

#define VAM_VEHICLE_SIZE_T_LEN    (sizeof(vam_vehicle_size_t))







/* Vehicle status structure. */
typedef struct _vam_stastatus_t
{ 
    uint32_t                     time;  /* This location point corresponding time. */  

    uint8_t      pid[RCP_TEMP_ID_LEN];  /* Product id. */
    uint16_t                  dsecond;  /* DSRC second. */

    vam_position_t                pos;  /* Vehicle position. */
    vam_pos_accuracy_t   pos_accuracy;  /* Vehicle position accuracy. */

    uint8_t	       transmission_state;  /* Transmission status. */
    float                       speed;  /* Driving speed. Unit km/h. */
    float                         dir;  /* Driving direction. Unit degree. */
    float	        steer_wheel_angle;  /* Steering wheel angle. Unit degree. */
    vam_acceset_t            acce_set;  /* Driving acceleration set. */
    brake_system_status_st brake_stat;  /* Brake system status. */
    exterior_lights_st exterior_light;  /* Exterior lights status. */

    uint8_t                  vec_type;  /* Vehicle type. */
    vam_vehicle_size_t       vec_size;  /* Vehicle size. */
   
    uint16_t               alert_mask;  /* bit0-VBD, bit1-EBD;  1-active, 0-cancel. */
    
} vam_stastatus_t, * vam_stastatus_t_ptr;

typedef struct _vam_sta_node{
    /* !!!DON'T modify it!!! */
    list_head_t list;

    vam_stastatus_t s;

    /* private */
    uint16_t life;
    uint16_t alert_life;
    /* os related */
}vam_sta_node_t;

typedef struct _test_comm_node{
    /* !!!DON'T modify it!!! */
    list_head_t list;

    uint8_t pid[RCP_TEMP_ID_LEN];  //temporary ID

    /* private */
    uint32_t rx_cnt;
    float    distance_dev;
    /* os related */
}test_comm_node_t;

typedef struct _vam_config{

    /* 
        Basic Safty Message TX function:    
    */
    uint8_t bsm_hops; //BSM消息最大跳数；
    uint8_t bsm_boardcast_mode;  /* 0 - disable, 1 - auto, 2 - fixed period */ 
    uint8_t bsm_boardcast_saftyfactor;  /* 1~10 */
    uint8_t bsm_pause_mode;  /* 0 - disable, 1-tx evam enable, 2-rx evam pause bsm*/
    uint8_t bsm_pause_hold_time;  /* unit:s */
    uint16_t bsm_boardcast_period;  /* 100~3000, unit:ms, min accuracy :10ms */
    /* 
        Emergency Vehicle Alert Message TX function:    
    */
    uint8_t evam_hops; //EVAM消息最大跳数；
	
	uint8_t evam_broadcast_type;  /* 0 - disable, 1 - auto, 2 - fixed period */
	
	uint16_t evam_broadcast_peroid;  //EVAM消息广播周期 ms
    

}vam_config_t;



typedef void (*vam_evt_handler)(void *);




/* Vam environment structure. */
typedef struct _vam_envar_t
{
    /* working_param */
    vam_config_t working_param;

    int flag;
    uint16_t bsm_send_period_ticks;

    uint8_t tx_bsm_msg_cnt;
    uint8_t tx_evam_msg_cnt;
    uint8_t tx_rsa_msg_cnt;

    vam_stastatus_t   local;
    
    list_head_t                   sta_free_list;
    list_head_t                  neighbour_list;
    uint8_t                       neighbour_cnt;
    vam_sta_node_t remote[VAM_NEIGHBOUR_MAXNUM];

    vam_evt_handler evt_handler[VAM_EVT_MAX];

    /* os related */
    osal_task_t   *task_vam;
    osal_queue_t *queue_vam;

    osal_timer_t *timer_send_bsm;
    osal_timer_t *timer_send_evam;
    osal_timer_t *timer_gps_life;
    osal_timer_t *timer_neighbour_life;

    osal_timer_t *timer_send_rsa;

    osal_sem_t *sem_sta;

}vam_envar_t, * vam_envar_t_ptr;

#define VAM_ENVAR_T_LEN    (sizeof(vam_envar_t))



typedef struct _vam_rsa_evt_info 
{
	uint16_t dsecond;//DSRC second time
    uint16_t rsa_mask;
	uint8_t	priority;
    heading_slice_t head;
    vam_position_t pos;
} vam_rsa_evt_info_t;


typedef struct _vam_pos_data{

    float distance_1_2;
    float distance_2_3;
    float angle;

}vam_pos_data;


/* restore all compiler settings in stacks. */
#pragma pack(pop)
/*****************************************************************************
 * declaration of global variables and functions                             *
*****************************************************************************/

extern vam_envar_t *p_vam_envar;

void vsm_start_bsm_broadcast(vam_envar_t *p_vam);
void vsm_update_bsm_bcast_timer(vam_envar_t *p_vam);

int vam_add_event_queue(vam_envar_t *p_vam, 
                             uint16_t msg_id, 
                             uint16_t msg_len, 
                             uint32_t msg_argc,
                             void    *msg_argv);
int vam_add_event_queue_2(vam_envar_t *p_vam, void *p_msg, uint32_t len);

vam_sta_node_t *vam_find_sta(vam_envar_t *p_vam, uint8_t *temporary_id);
void vam_update_sta(vam_envar_t *p_vam);


void lip_gps_proc(vam_envar_t *p_vam, uint8_t *databuf, uint32_t len);
void lip_update_local(t_nmea_rmc *p_rmc, float *p_accu);
void lip_update_local_acc(float x, float y, float z);

float vsm_get_distance(vam_position_t *p_src, vam_position_t *p_dest);
vam_pos_data vsm_get_data(vam_stastatus_t *p_src, vam_stastatus_t *p_dest);
float vsm_get_pos(vam_stastatus_t *p_src, vam_stastatus_t *p_dest,vam_pos_data *pos_data);
float vsm_get_relative_pos(vam_stastatus_t *p_src, vam_stastatus_t *p_dest);
float vsm_get_relative_dir(const vam_stastatus_t *p_src, const  vam_stastatus_t *p_dest);
int8_t vsm_get_rear_dir(vam_stastatus_t *p_dest);
void vsm_start_evam_broadcast(vam_envar_t *p_vam);
int32_t vsm_get_dr_current(vam_stastatus_t *last, vam_stastatus_t *current);


int32_t vam_start(void);
int32_t vam_stop(void);
int32_t vam_set_event_handler(uint32_t evt, vam_evt_handler callback);
int32_t vam_get_peer_relative_pos(uint8_t *pid, uint8_t flag);
int32_t vam_get_peer_relative_dir(const vam_stastatus_t *local,const vam_stastatus_t *remote);
int32_t vam_get_peer_alert_status(uint16_t *alert_mask);
int32_t vam_active_alert(uint16_t alert);
int32_t vam_cancel_alert(uint16_t alert);
void vam_gsnr_ebd_detected(uint8_t status);

extern int32_t vam_set_local_status(vam_stastatus_t *local);

int32_t vam_get_all_peer_pid(uint8_t pid[][RCP_TEMP_ID_LEN], uint32_t maxitem, uint32_t *actual);

int32_t vam_get_local_status(vam_stastatus_t *local);
int32_t vam_get_local_current_status(vam_stastatus_t *local);

int32_t vam_get_peer_status(uint8_t *pid, vam_stastatus_t *local);
int32_t vam_get_peer_current_status(uint8_t *pid, vam_stastatus_t *local);

int vam_rcp_recv(wnet_rxinfo_t *rxinfo, uint8_t *databuf, uint32_t datalen);
int rcp_parse_msg(vam_envar_t *p_vam,
                  wnet_rxinfo_t *rxinfo, 
                  uint8_t *databuf, 
                  uint32_t datalen);
int rcp_send_bsm(vam_envar_t *p_vam);
int rcp_send_evam(vam_envar_t *p_vam);
int rcp_send_rsa(vam_envar_t *p_vam);

extern void vam_stop_alert(void);

/* return 1-gps is located,  0-gps is lost */
extern uint8_t vam_get_gps_status(void);




#endif /* __CV_VAM_H__ */

