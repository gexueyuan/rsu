/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : cv_vsa.h
 @brief  : this file include the definition of vsa module
 @author : wangyifeng
 @history:
           2014-6-20    wangyifeng    Created file
           ...
******************************************************************************/
#ifndef __CV_VSA_H__
#define __CV_VSA_H__


#define VSA_TIMER_PERIOD         SECOND_TO_TICK(1)
#define VSA_EBD_SEND_PERIOD      SECOND_TO_TICK(5)
#define VSA_POS_PERIOD           MS_TO_TICK(100)
#define DIRECTION_DIVIDE         22.5f

#define PI 3.1415926f


/* Local vehicle's ebd detect factor. */
#define EBD_ACCELERATION_THRESHOLD     -3.0
#define EBD_ACCELERATION_CNT           4



#define VSA_NODE_MAX  VAM_NEIGHBOUR_MAXNUM





/*down to top*/
#define  HIGHWAY_MODE    0x0F9F
#define  MOUNTAIN_MODE   0x0FBF
#define  CITY_MODE       0x0FD7
#define  CUSTOM_MODE     0x019F



/* Vsa message type. */
enum VSA_MSG_TYPE
{
    VSA_MSG_VBD_SEND = 0,  /* Local vehicle's vbd alert detect handler. */
    VSA_MSG_EBD_SEND,      /* Local vehicle's ebd alert detect handler. */
    VSA_MSG_AUTO_BC,
    
    VSA_MSG_CFCW_ALARM,    /* Cfcw alert detect handler. */
    VSA_MSG_CRCW_ALARM,    /* Crcw alert detect handler. */
    VSA_MSG_OPPOSITE_ALARM,
    VSA_MSG_SIDE_ALARM,

    VSA_MSG_VBD_RCV,       /* Vbd alert receive handler. */
    VSA_MSG_EBD_RCV,       /* Ebd alert receive handler. */
    
    VSA_MSG_X_RCV,
    VSA_MSG_XX_RCV,
    VSA_MSG_XXX_RCV
};


enum VSA_APP_ID
{
    VSA_ID_NONE = 0,
        
    VSA_ID_CRD,
    VSA_ID_CRD_REAR,
    VSA_ID_OPS,
    VSA_ID_SIDE,
    VSA_ID_VBD,    
    VSA_ID_EBD,
    VSA_ID_VOT,
    VSM_ID_END
};


/* Vehicle heading slice definition. */
typedef enum _vehicle_heading_slice
{
    SLICEx_NO_HEADING = 0,
    SLICEx_ALL_HEADING,

    SLICE0_000_0T022_5,  /* 22.5 degree starting from North and moving Eastward. */
    SLICE1_022_5T045_0,
    SLICE2_045_0T067_5,
    SLICE3_067_5T090_0,

    SLICE4_090_0T112_5,
    SLICE5_112_5T135_0,
    SLICE6_135_0T157_5,
    SLICE7_157_5T180_0,

    SLICE8_180_0T202_5,
    SLICE9_202_5T225_0,
    SLICE10_225_0T247_5,
    SLICE11_247_5T270_0,

    SLICE12_270_0T292_5,
    SLICE13_292_5T315_0,
    SLICE14_315_0T337_5,
    SLICE15_337_5T360_0

}vehicle_heading_slice;


#define DEGREE_PER_HEADING_SLICE    ((float)22.5)



/* Vsa node information structure. */
typedef struct _vsa_node_st
{
    uint8_t pid[RCP_TEMP_ID_LEN];
    
    uint32_t   vsa_location;

    float       local_speed;
    float      remote_speed;

    float    relative_speed;
    float      relative_dir;
      
    uint32_t       v_offset;
    uint32_t       h_offset;

    int32_t linear_distance;
    uint32_t  safe_distance;

    uint16_t      vam_alert;    /* Vam alert mark. */
    uint32_t      vsa_alert;    /* Vsa alert mark. */
    
}vsa_node_st, * vsa_node_st_ptr;

#define VSA_NODE_ST_LEN    (sizeof(vsa_node_st))


/*****************************************************************************
 * definition of struct                                                      *
*****************************************************************************/

typedef struct _vsa_config_t
{
    /* General */
    uint8_t danger_detect_speed_threshold;  /* unit: km/h */
    uint16_t                     lane_dis;  /* unit:m, min accuracy :1m*/

    /* Close Range Danger function. */
    uint8_t  crd_saftyfactor;      /* 1~10 */
    uint8_t crd_oppsite_speed;     /*<=255:30km/h*/
    uint8_t crd_oppsite_rear_speed;/*<=255:30km/h*/
    uint8_t crd_rear_distance;     /*<=255:10m*/

    /* Emergency Braking Danger function. */
    uint8_t ebd_mode;                   /* 0 - disable, 1 - enable */
    uint8_t ebd_acceleration_threshold; /* unit:m/s2 */
    uint8_t ebd_alert_hold_time;        /* unit:s */
	
}vsa_config_t;



/* Vsa environment structure. */
typedef struct _vsa_envar_t
{
    uint32_t          vsa_mode;            /* Vsa detect mode.*/
    vsa_config_t working_param;            /* Working parameter. */

    uint32_t                    node_cnt;  /* Neighbour node's count in node group. */
    vsa_node_st node_group[VSA_NODE_MAX];  /* Neighbour group. */

    /* os related */
    osal_task_t  *task_vsa_l;
    osal_task_t  *task_vsa_r;

    osal_sem_t * sem_vsa_proc;
   
    osal_queue_t *queue_vsa;

    osal_timer_t *timer_ebd_send;
    osal_timer_t *timer_position_prepro;
                 
}vsa_envar_t, * vsa_envar_t_ptr;

#define VSA_ENVAR_T_LEN    (sizeof(vsa_envar_t))



typedef int (*vsa_app_handler)(vsa_envar_t *p_vsa, void *p_msg);
extern int8_t vsa_position_get(uint8_t *pid, vsa_node_st_ptr node_ptr);


void vsa_start(void);
extern osal_status_t vsa_add_event_queue(vsa_envar_t *p_vsa, uint16_t msg_id, uint16_t msg_len, uint32_t msg_argc, void *msg_argv);



#endif

