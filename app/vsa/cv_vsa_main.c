/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : cv_vsa_main.c
 @brief  : this file realize the function of vehicle safty application
 @author : wangyifeng
 @history:
           2014-6-19    wangyifeng    Created file
           ...
******************************************************************************/
#include "cv_osal.h"
#define OSAL_MODULE_DEBUG
#define OSAL_MODULE_DEBUG_LEVEL OSAL_DEBUG_INFO
#define MODULE_NAME "vsa"
#include "cv_osal_dbg.h"
OSAL_DEBUG_ENTRY_DEFINE(vsa)

#include "osal_sem.h"
#include "cv_vam.h"
#include "cv_cms_def.h"
#include "cv_vsa.h"
#include <math.h>

/*****************************************************************************
 * declaration of variables and functions                                    *
*****************************************************************************/
void space_null(void)
{
}

#define VSA_TIMER_PERIOD         SECOND_TO_TICK(1)
#define VSA_EBD_SEND_PERIOD      SECOND_TO_TICK(5)
#define VSA_POS_PERIOD           MS_TO_TICK(100)
#define DIRECTION_DIVIDE         22.5f

static int vbd_judge(vsa_node_st *p_node);
static int ebd_judge(vsa_node_st *p_node);

extern void test_comm(void);
extern uint8_t vam_get_gps_status(void);
extern void param_get(void);

/*****************************************************************************
 * implementation of functions                                               *
*****************************************************************************/






/* Convert vehicle angle to heading slice. */
static vehicle_heading_slice vsa_angle_to_heading_slice(float angle)
{
    if((0 * DEGREE_PER_HEADING_SLICE <= angle) && (angle < 1 * DEGREE_PER_HEADING_SLICE))
    {
        return SLICE0_000_0T022_5;
    }
    else if((1 * DEGREE_PER_HEADING_SLICE <= angle) && (angle < 2 * DEGREE_PER_HEADING_SLICE))
    {
        return SLICE1_022_5T045_0;
    }
    else if((2 * DEGREE_PER_HEADING_SLICE <= angle) && (angle < 3 * DEGREE_PER_HEADING_SLICE))
    {
        return SLICE2_045_0T067_5;
    }    
    else if((3 * DEGREE_PER_HEADING_SLICE <= angle) && (angle < 4 * DEGREE_PER_HEADING_SLICE))
    {
        return SLICE3_067_5T090_0;
    }
    else if((4 * DEGREE_PER_HEADING_SLICE <= angle) && (angle < 5 * DEGREE_PER_HEADING_SLICE))
    {
        return SLICE4_090_0T112_5;
    }
    else if((5 * DEGREE_PER_HEADING_SLICE <= angle) && (angle < 6 * DEGREE_PER_HEADING_SLICE))
    {
        return SLICE5_112_5T135_0;
    }
    else if((6 * DEGREE_PER_HEADING_SLICE <= angle) && (angle < 7 * DEGREE_PER_HEADING_SLICE))
    {
        return SLICE6_135_0T157_5;
    }
    else if((7 * DEGREE_PER_HEADING_SLICE <= angle) && (angle < 8 * DEGREE_PER_HEADING_SLICE))
    {
        return SLICE7_157_5T180_0;
    }    
    else if((8 * DEGREE_PER_HEADING_SLICE <= angle) && (angle < 9 * DEGREE_PER_HEADING_SLICE))
    {
        return SLICE8_180_0T202_5;
    }
    else if((9 * DEGREE_PER_HEADING_SLICE <= angle) && (angle < 10 * DEGREE_PER_HEADING_SLICE))
    {
        return SLICE9_202_5T225_0;
    }
    else if((10 * DEGREE_PER_HEADING_SLICE <= angle) && (angle < 11 * DEGREE_PER_HEADING_SLICE))
    {
        return SLICE10_225_0T247_5;
    }
    else if((11 * DEGREE_PER_HEADING_SLICE <= angle) && (angle < 12 * DEGREE_PER_HEADING_SLICE))
    {
        return SLICE11_247_5T270_0;
    }
    else if((12 * DEGREE_PER_HEADING_SLICE <= angle) && (angle < 13 * DEGREE_PER_HEADING_SLICE))
    {
        return SLICE12_270_0T292_5;
    }
    else if((13 * DEGREE_PER_HEADING_SLICE <= angle) && (angle < 14 * DEGREE_PER_HEADING_SLICE))
    {
        return SLICE13_292_5T315_0;
    }
    else if((14 * DEGREE_PER_HEADING_SLICE <= angle) && (angle < 15 * DEGREE_PER_HEADING_SLICE))
    {
        return SLICE14_315_0T337_5;
    }
    else if((15 * DEGREE_PER_HEADING_SLICE <= angle) && (angle < 16 * DEGREE_PER_HEADING_SLICE))
    {
        return SLICE15_337_5T360_0;
    }
    else
    {
        return SLICEx_NO_HEADING;
    }
}


/*****************************************************************************
 @funcname: vsa_position_classify
 @brief   : describe target's postion
 @param   : None
 @return  : 
*****************************************************************************/
uint32_t  vsa_position_classify(const vam_stastatus_t *local, const vam_stastatus_t *remote, vam_pos_data *pos_data, float *delta_offset)
{
    double lat1, lng1, lat2, lng2;
    float angle, delta;

    /* reference point */
    lat1 = local->pos.latitude;
    lng1 = local->pos.longitude;

    /* destination point */
    lat2 = remote->pos.latitude;
    lng2 = remote->pos.longitude;


    angle = pos_data->angle;

    /* calculate the relative angle against north, clockwise  */
    if (lat2 >= lat1){
    /* north */
        if (lng2 >= lng1){
        /* easts */
            //equal
        }
        else{
            angle = 360-angle;
        }
    }
    else{
    /* south */
        if (lng2 >= lng1){
        /* easts */
            angle = 180-angle;
        }
        else{
            angle = 180+angle;
        }
    }

    /* calculate the angle detra between local front and remote position  */
    if (angle >= local->dir){
        delta = angle - local->dir;
    }
    else {
        delta = 360.0f-(local->dir - angle);
    }
    
    *delta_offset = delta;  

    return vsa_angle_to_heading_slice(delta);

}


/*****************************************************************************
 @funcname: vsa_safe_distance
 @brief   : calculate the safe diatance
 @param   : position:ahead or behind
 @return  : safe distance with the remote point  
*****************************************************************************/

uint32_t vsa_safe_distance(int32_t position,vam_stastatus_t local,vam_stastatus_t remote)
{
    vsa_envar_t *p_vsa = &p_cms_envar->vsa;

    if (position < 0){
        return (int32_t)p_vsa->working_param.crd_rear_distance;
    }

    if (local.speed > remote.speed){
       return(int32_t)((local.speed*2.0f - remote.speed)*p_vsa->working_param.crd_saftyfactor*1000.0f/3600.0f);
    }
    else{
       return 0; 
    }
}

int8_t vsa_position_get(uint8_t *pid, vsa_node_st_ptr node_ptr)
{
    int8_t i = 0;
    int8_t ret = -1;
    vsa_envar_t *p_vsa = &p_cms_envar->vsa;

    
    for(i = 0; i < p_vsa->node_cnt; i++) 
    {
        if(memcmp(p_vsa->node_group[i].pid, pid, RCP_TEMP_ID_LEN)==0) 
        {
            memcpy(node_ptr, &(p_vsa->node_group[i]), sizeof(p_vsa->node_group[i]));
            ret = 0;
            break;
        }
    }

    return ret;
}


/*****************************************************************************
 @funcname: vsa_preprocess_pos
 @brief   : process  information of points in the neighbour list in a period
 @param   : void
 @return  : void 
*****************************************************************************/

int32_t  vsa_preprocess_pos(vsa_envar_t_ptr p_vsa)
{
    vam_stastatus_t local_status;  
    vam_stastatus_t remote_status;
    vsa_node_st *p_pnt = NULL;
    vam_pos_data temp_data;
    int8_t i = 0;
    
    uint8_t peer_pid[VAM_NEIGHBOUR_MAXNUM][RCP_TEMP_ID_LEN];
    float temp_delta = 0;

    if (0x1 & g_dbg_print_type)
    {
        test_comm();
    }
    
    vam_get_all_peer_pid(peer_pid, VAM_NEIGHBOUR_MAXNUM, &(p_vsa->node_cnt));

    /* Return peer count when no pid. */
    if(p_vsa->node_cnt == 0) 
    {
        return p_vsa->node_cnt;
    }

    vam_get_local_current_status(&local_status);
    
    for(i = 0; i < p_vsa->node_cnt; i++) 
    {
        vam_get_peer_status(peer_pid[i], &remote_status); /*!!查询不到的风险!!*/  
        
        p_pnt = &(p_vsa->node_group[i]);
        memcpy(p_pnt->pid, remote_status.pid, RCP_TEMP_ID_LEN);

        temp_data = vsm_get_data(&local_status,&remote_status);

        p_pnt->vsa_location = vsa_position_classify(&local_status,&remote_status,&temp_data,&temp_delta);

        p_pnt->local_speed = local_status.speed;

        p_pnt->remote_speed = remote_status.speed;
        p_pnt->relative_speed = local_status.speed - remote_status.speed;                

        p_pnt->linear_distance = (int32_t)vsm_get_pos(&local_status,&remote_status,&temp_data);

        p_pnt->v_offset = (uint32_t)(p_pnt->linear_distance*cos(temp_delta*PI/180.0f));
        p_pnt->h_offset = (uint32_t)(p_pnt->linear_distance*sin(temp_delta*PI/180.0f));

        p_pnt->safe_distance = vsa_safe_distance(p_pnt->linear_distance,local_status,remote_status);
        
        p_pnt->relative_dir = vsm_get_relative_dir(&local_status,&remote_status);

        p_pnt->vam_alert = remote_status.alert_mask;
            
    }				
    
    return p_vsa->node_cnt;
}

/*****************************************************************************
 @funcname: timer_preprocess_pos_callback
 @brief   : preprocess for vsa module,information from neighbourlist
 @param   : None
 @return  : 
*****************************************************************************/
void  timer_preprocess_pos_callback( void *parameter )
{
    osal_sem_release(p_cms_envar->vsa.sem_vsa_proc);
}

/*****************************************************************************
 @funcname: vsa_find_pn
 @brief   : find position node in position link list
 @param   : None
 @return  : 
*****************************************************************************/
vsa_node_st *vsa_find_pn(vsa_envar_t *p_vsa, uint8_t *temporary_id)
{
    vsa_node_st   *p_pn = NULL;

  uint32_t node_index = 0;

  

    for(node_index = 0; node_index < p_vsa->node_cnt; node_index ++)
    {
        if(memcmp(p_vsa->node_group[node_index].pid, temporary_id, RCP_TEMP_ID_LEN) == 0)
        {
            p_pn = &(p_vsa->node_group[node_index]);
            break;
        }
    }
    
    /* not found */
    if(p_pn == NULL)
    {
        OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_WARN, "vsa position node need updating!\n\n");                    
    }

    return p_pn;
}

void vsa_bsm_alarm_update(void *parameter)
{
    /* Inform vsa module to calculate neighbour node. */
    osal_sem_release(p_cms_envar->vsa.sem_vsa_proc); 
}

void vsa_rsa_alarm_update(void *parameter)
{
    vam_rsa_evt_info_t *param = (vam_rsa_evt_info_t*)parameter;
    vsa_envar_t *p_vsa = &p_cms_envar->vsa;

    if(param->rsa_mask)
        vsa_add_event_queue(p_vsa, VSA_MSG_XXX_RCV, 0,0,NULL);
}

void vsa_start(void)
{
    vsa_envar_t *p_vsa = &p_cms_envar->vsa;    


    vam_set_event_handler(VAM_EVT_BSM_ALARM_UPDATE, vsa_bsm_alarm_update);
    vam_set_event_handler(VAM_EVT_RSA_UPDATE, vsa_rsa_alarm_update);
    
    osal_timer_start(p_vsa->timer_position_prepro);
    
    OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_INFO, "%s: --->\n", __FUNCTION__);
}

void timer_ebd_send_callback(void* parameter)
{
    vam_cancel_alert(VAM_ALERT_MASK_EBD);
    OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_INFO, "Cancel Emergency braking. \n\n");                                          
}

static int ebd_judge(vsa_node_st *p_node)
{
    vsa_envar_t *p_vsa = &p_cms_envar->vsa;
    

    if((p_node->vam_alert & VAM_ALERT_MASK_EBD) == 0)
    {
        return VSA_ID_NONE;
    }
    
    if(p_node->local_speed <= p_vsa->working_param.danger_detect_speed_threshold)
    { 
        return VSA_ID_NONE;
    }

    if(p_node->remote_speed <= p_vsa->working_param.danger_detect_speed_threshold)
    { 
        return VSA_ID_NONE;
    } 

    /* Return no alert when relative direction out of range. */
    if((10 < p_node->relative_dir) && (p_node->relative_dir < 350))
    {
       return VSA_ID_NONE;
    }

    /* remote is behind of local */
    if(p_node->linear_distance <= 0)
    {
        return VSA_ID_NONE;
    }
    
    return VSA_ID_EBD;

}

static int vbd_judge(vsa_node_st *p_node)
{
    vsa_envar_t *p_vsa = &p_cms_envar->vsa;

    
    if((p_node->vam_alert & VAM_ALERT_MASK_VBD) == 0)
    {
    	OSAL_DBGPRT(OSAL_DEBUG_TRACE, "p_node->vam_alert:%04x\n", p_node->vam_alert);
        return VSA_ID_NONE;
    }
    
    if (p_node->local_speed <= p_vsa->working_param.danger_detect_speed_threshold)
    {
    	OSAL_DBGPRT(OSAL_DEBUG_TRACE, "p_node->local_speed:%.lf p_vsa->working_param.danger_detect_speed_threshold=%.lf\n", p_node->local_speed, p_node->local_speed);
        return VSA_ID_NONE;
    }

    /* Horizontal distance compare with lane.*/
    if(p_node->h_offset > p_vsa->working_param.lane_dis)
    {
    	OSAL_DBGPRT(OSAL_DEBUG_TRACE, "p_node->h_offset: p_vsa->working_param.lane_dis:%d\n", p_node->h_offset, p_vsa->working_param.lane_dis);
        return VSA_ID_NONE;    
    }

    /* Return no alert when relative direction out of range. */
    if((10 < p_node->relative_dir) && (p_node->relative_dir < 350))
    {
       OSAL_DBGPRT(OSAL_DEBUG_TRACE, "p_node->relative_dir:%.lf p_node->relative_dir:%.lf%\n", p_node->relative_dir, p_node->relative_dir);
       return VSA_ID_NONE;
    }
    
    /* remote is behind of local. */
    if (p_node->linear_distance <= 0)
    {
    	OSAL_DBGPRT(OSAL_DEBUG_TRACE, "p_node->linear_distance:%d\n", p_node->linear_distance);
        return VSA_ID_NONE;
    }
    
    OSAL_DBGPRT(OSAL_DEBUG_TRACE, "alert value:%d\n", VSA_ID_VBD);
    return VSA_ID_VBD;

}

/*****************************************************************************
 @funcname: cfcw_judge
 @brief   : check and judge the close range danger of vehicles
 @param   : vsa_envar_t *p_vsa  
 @return  : 
            0 - no alert
            1 - alert
*****************************************************************************/
static int cfcw_judge(vsa_node_st *p_node)
{
    vsa_envar_t *p_vsa = &p_cms_envar->vsa;


    if(p_node->local_speed <= p_vsa->working_param.danger_detect_speed_threshold)
    {
    	OSAL_DBGPRT(OSAL_DEBUG_TRACE, "p_node->local_speed:%f p_vsa->working_param.danger_detect_speed_threshold=%d\n", p_node->local_speed, p_vsa->working_param.danger_detect_speed_threshold);
        return VSA_ID_NONE;
    }
 
    /* Return no alert when relative direction out of range. */
    if((10 < p_node->relative_dir) && (p_node->relative_dir < 350))
    {
    	OSAL_DBGPRT(OSAL_DEBUG_TRACE, "p_node->relative_dir:%f\n", p_node->relative_dir);
        return VSA_ID_NONE;
    }

    /* horizontal distance compare with lane. */
    if(p_vsa->working_param.lane_dis < p_node->h_offset)
    {
    	OSAL_DBGPRT(OSAL_DEBUG_TRACE, "p_vsa->working_param.lane_dis:%d p_node->h_offset:%d\n",p_vsa->working_param.lane_dis, p_node->h_offset);
        return VSA_ID_NONE;    
    }
        
    if(p_node->local_speed <= (p_node->remote_speed + p_vsa->working_param.crd_oppsite_speed))
    {
    	OSAL_DBGPRT(OSAL_DEBUG_TRACE, "p_node->local_speed:%f p_node->remote_speed + p_vsa->working_param.crd_oppsite_speed:%f\n", p_node->local_speed, (p_node->remote_speed + p_vsa->working_param.crd_oppsite_speed));
        return VSA_ID_NONE;
    }

    /* remote is behind of local */
    if(p_node->linear_distance <= 0)
    {
    	OSAL_DBGPRT(OSAL_DEBUG_TRACE, "p_node->linear_distance:%d\n", p_node->linear_distance);
        return VSA_ID_NONE;
    }
    
    if(p_node->safe_distance < p_node->linear_distance)
    {
    	OSAL_DBGPRT(OSAL_DEBUG_TRACE, "p_node->safe_distance:%d p_node->linear_distance:%d\n", p_node->safe_distance, p_node->linear_distance);
        return VSA_ID_NONE;
    }
    OSAL_DBGPRT(OSAL_DEBUG_INFO, "front vehicle alert:%0x\n", VSA_ID_CRD);
    return VSA_ID_CRD;

}

/*****************************************************************************
 @funcname: crcw_judge
 @brief   : check and judge the close range danger of vehicles
 @param   : vsa_envar_t *p_vsa  
 @return  : 
            0 - no alert
            1 - alert
*****************************************************************************/
static int crcw_judge(vsa_node_st *p_node)
{
    vsa_envar_t *p_vsa = &p_cms_envar->vsa;


    if (p_node->local_speed <= p_vsa->working_param.danger_detect_speed_threshold)
    {
        return VSA_ID_NONE;
    }

    if (p_node->remote_speed <= p_vsa->working_param.danger_detect_speed_threshold)
    {
        return VSA_ID_NONE;
    } 
    
    /* Return no alert when relative direction out of range. */
    if((10 < p_node->relative_dir) && (p_node->relative_dir < 350))
    {
       return VSA_ID_NONE;
    }
    
    if ((p_node->local_speed + p_vsa->working_param.crd_oppsite_speed) >= p_node->remote_speed)
    { 
        return VSA_ID_NONE;
    }

    if(0 < p_node->linear_distance)
    {
        return VSA_ID_NONE;
    }
    
    if(p_vsa->working_param.crd_rear_distance < (- p_node->linear_distance) )
    {
        return VSA_ID_NONE;
    }
    
    return VSA_ID_CRD_REAR;
}


static int vsa_vbd_send_proc(vsa_envar_t *p_vsa, void *arg)
{
#if 0
  int err = 1;  /* '1' represent is not handled. */ 
  sys_msg_t *p_msg = (sys_msg_t *)arg;

  
  if(p_msg->argc)
  {
      vam_active_alert(VAM_ALERT_MASK_VBD);
      OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_INFO, "Active Vihicle Break Down Alert\n");
  }   
  else 
  {   
      vam_cancel_alert(VAM_ALERT_MASK_VBD);
      OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_INFO, "Cancel Vihicle Break Down Alert\n");
  }
#endif  
  
  return 0;
}


static int vsa_ebd_send_proc(vsa_envar_t *p_vsa, void *arg)
{
    vam_stastatus_t_ptr local_vam_ptr = &(p_cms_envar->vam.local);
    static uint8_t              count = 0;


    /* Detect local vehicle's edb condition. */
    if(local_vam_ptr->acce_set.longitudinal < EBD_ACCELERATION_THRESHOLD)
    {
        count ++;
        
        if(EBD_ACCELERATION_CNT < count)
        {
            count = 0;
            
            OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_INFO, "Detect local vehicle's EBD. \n\n");

            /* Active ebd alert and start the close alert timer. */
            vam_active_alert(VAM_ALERT_MASK_EBD);
            osal_timer_stop(p_vsa->timer_ebd_send);
            osal_timer_start(p_vsa->timer_ebd_send); 
        }
    }
    else
    {
        count = 0;
    }

    return 0;
}
static int vsa_auto_broadcast_proc(vsa_envar_t *p_vsa, void *arg)
{

    return 0;
}




static int vsa_cfcw_alarm_proc(vsa_envar_t *p_vsa, void *arg)
{
    int           vsa_id = 0;
    vsa_node_st *pos_pnt = NULL;


    pos_pnt = &p_vsa->node_group[*((int*)arg)];
    vsa_id = cfcw_judge(pos_pnt);

    return vsa_id;
}

static int vsa_crcw_alarm_proc(vsa_envar_t *p_vsa, void *arg)
{
    int           vsa_id = 0;
    vsa_node_st *pos_pnt = NULL;
    

    pos_pnt = &p_vsa->node_group[*((int*)arg)];
    vsa_id = crcw_judge(pos_pnt);

    return vsa_id;
}

static int vsa_opposite_alarm_proc(vsa_envar_t *p_vsa, void *arg)
{
    return VSA_ID_NONE;
}
static int vsa_side_alarm_proc(vsa_envar_t *p_vsa, void *arg)
{
    return VSA_ID_NONE;
}

static int vsa_vbd_rcv_proc(vsa_envar_t *p_vsa, void *arg)
{
    int           vsa_id = 0;
    vsa_node_st *pos_pnt = NULL;
    

    pos_pnt = &p_vsa->node_group[*((int*)arg)];
    vsa_id = vbd_judge(pos_pnt);

    return vsa_id;
}

static int vsa_ebd_rcv_proc(vsa_envar_t *p_vsa, void *arg)
{
    int           vsa_id = 0;
    vsa_node_st *pos_pnt = NULL;
    

    pos_pnt = &p_vsa->node_group[*((int*)arg)];
    vsa_id = ebd_judge(pos_pnt);

    return vsa_id;
}

static int vsa_x_recieve_proc(vsa_envar_t *p_vsa, void *arg)
{


  return 0;
}

static int vsa_xx_recieve_proc(vsa_envar_t *p_vsa, void *arg)
{


  return 0;
}

static int vsa_xxx_recieve_proc(vsa_envar_t *p_vsa, void *arg)
{


  return 0;
}



/*  */
vsa_app_handler vsa_app_handler_tbl[] = 
{
    vsa_vbd_send_proc,
    vsa_ebd_send_proc,
    vsa_auto_broadcast_proc,

    vsa_cfcw_alarm_proc,
    vsa_crcw_alarm_proc,
    vsa_opposite_alarm_proc,
    vsa_side_alarm_proc,

    vsa_vbd_rcv_proc,
    vsa_ebd_rcv_proc,
    
    vsa_x_recieve_proc,
    vsa_xx_recieve_proc,
    vsa_xxx_recieve_proc,
};




void * vsa_base_proc(void *parameter)
{
    int              i = 0;
    vsa_envar_t *p_vsa = (vsa_envar_t *)parameter;

    
    while(1)
    {
        osal_sem_take(p_vsa->sem_vsa_proc, OSAL_WAITING_FOREVER);

        vsa_preprocess_pos(&(p_cms_envar->vsa));

        for(i = 0; i < p_vsa->node_cnt; i++)
        {
            /* CFCW alarm detect. */
            if(vsa_app_handler_tbl[VSA_MSG_CFCW_ALARM] != NULL)
            {    
                if(vsa_app_handler_tbl[VSA_MSG_CFCW_ALARM](p_vsa, &i) == VSA_ID_CRD)
                {
                    p_vsa->node_group[i].vsa_alert |= (1 << VSA_ID_CRD);
                }
                else
                {
                    p_vsa->node_group[i].vsa_alert &= ~(1 << VSA_ID_CRD);
                }
            }

            /* CRCW alarm detect. */
            if(vsa_app_handler_tbl[VSA_MSG_CRCW_ALARM] != NULL)
            {    
                if(vsa_app_handler_tbl[VSA_MSG_CRCW_ALARM](p_vsa,&i) == VSA_ID_CRD_REAR)
                {
                    p_vsa->node_group[i].vsa_alert |= (1 << VSA_ID_CRD_REAR);
                }
                else
                {
                    p_vsa->node_group[i].vsa_alert &= ~(1 << VSA_ID_CRD_REAR);
                }
            }

            /* VBD alarm detect. */
            if(vsa_app_handler_tbl[VSA_MSG_VBD_RCV] != NULL)
            {    
                if(vsa_app_handler_tbl[VSA_MSG_VBD_RCV](p_vsa,&i) == VSA_ID_VBD)
                {
                    p_vsa->node_group[i].vsa_alert |= (1 << VSA_ID_VBD);
                }
                else
                {
                    p_vsa->node_group[i].vsa_alert &= ~(1 << VSA_ID_VBD);
                }
            }

            /* EBD alarm detect. */
            if(vsa_app_handler_tbl[VSA_MSG_EBD_RCV] != NULL)
            {    
                if(vsa_app_handler_tbl[VSA_MSG_EBD_RCV](p_vsa,&i) == VSA_ID_EBD)
                {
                    p_vsa->node_group[i].vsa_alert |= (1 << VSA_ID_EBD);
                }
                else
                {
                    p_vsa->node_group[i].vsa_alert &= ~(1 << VSA_ID_EBD);
                }
            }

            /* Local vehicle's ebd detect. */
            if(vsa_app_handler_tbl[VSA_MSG_EBD_SEND] != NULL)
            {
                vsa_app_handler_tbl[VSA_MSG_EBD_SEND](p_vsa, &i);
            }
        }
    }
}

void * vsa_thread_entry(void *parameter)
{
    osal_status_t err;
    sys_msg_t* p_msg = NULL;
    vsa_envar_t *p_vsa = (vsa_envar_t *)parameter;
    uint32_t len = 0;
    
    OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_INFO, "%s: ---->\n", __FUNCTION__);

    uint8_t buf[VSA_MQ_MSG_SIZE];
    p_msg = (sys_msg_t *)buf;
        
    while(1)
    {
        memset(buf, 0, VSA_MQ_MSG_SIZE);
        err = osal_queue_recv(p_vsa->queue_vsa, buf, &len, OSAL_WAITING_FOREVER);
        if (err == OSAL_STATUS_SUCCESS)
        {
            if(vsa_app_handler_tbl[p_msg->id] != NULL)
            {
                err = vsa_app_handler_tbl[p_msg->id](p_vsa, p_msg); 
            }
        }
        else
        {
            OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_INFO, "%s: rt_mq_recv error [%d]\n", __FUNCTION__, err);          
        }      
    }
}


/* Send message queue to vsa module. */
osal_status_t vsa_add_event_queue(vsa_envar_t *p_vsa, uint16_t msg_id, uint16_t msg_len, uint32_t msg_argc, void *msg_argv)
{
    osal_status_t err = OSAL_STATUS_NOMEM;
    sys_msg_t     msg = { msg_id, msg_len, msg_argc, msg_argv };


    /* Send message queue to vam. */
    err = osal_queue_send(p_vsa->queue_vsa, &msg, sizeof(msg), 0, OSAL_NO_WAIT);
    if (err != OSAL_STATUS_SUCCESS) 
    {
        OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_WARN, "%s: failed=[%d], msg=%04x. \n", __FUNCTION__, err, msg_id);
    }
    
    return err;
}


/*****************************************************************************
 @funcname: vsa_init
 @brief   : vsa module initial
 @param   : None
 @return  : 
*****************************************************************************/
void vsa_init(void)
{
    int              i = 0;
    vsa_envar_t *p_vsa = &p_cms_envar->vsa;

    
    p_vsa->vsa_mode = ~CUSTOM_MODE;
    
    memset(p_vsa, 0, sizeof(vsa_envar_t));
    memcpy(&p_vsa->working_param, &p_cms_param->vsa, sizeof(vsa_config_t));

    /* Initial vsa detect item based on vsa mode.  */
    for(i = 0; i < sizeof(vsa_app_handler_tbl)/sizeof(vsa_app_handler_tbl[0]); i++)
    {
        if(p_vsa->vsa_mode & (1 << i))
        {
            vsa_app_handler_tbl[i] = NULL;
        }            
    }

    /* os object for vsa */
    p_vsa->queue_vsa = osal_queue_create("q-vsa",  VSA_QUEUE_SIZE, VSA_MQ_MSG_SIZE);
    osal_assert(p_vsa->queue_vsa != NULL);
    
    p_vsa->timer_ebd_send = osal_timer_create("tm-ebd", timer_ebd_send_callback, NULL, VSA_EBD_SEND_PERIOD, TIMER_INTERVAL|TIMER_STOPPED, TIMER_PRIO_NORMAL);                     
    osal_assert(p_vsa->timer_ebd_send != NULL);

    p_vsa->timer_position_prepro = osal_timer_create("tm-pos", timer_preprocess_pos_callback, NULL, VSA_POS_PERIOD, TIMER_INTERVAL|TIMER_STOPPED, TIMER_PRIO_NORMAL);
    osal_assert(p_vsa->timer_position_prepro != NULL);

    p_vsa->sem_vsa_proc = osal_sem_create("sem-vsa",0);
    osal_assert(p_vsa->sem_vsa_proc != NULL);
    
    p_vsa->task_vsa_l = osal_task_create("t-vsa-l", vsa_base_proc, p_vsa, RT_VSA_THREAD_STACK_SIZE, RT_VSA_THREAD_PRIORITY);
    osal_assert(p_vsa->task_vsa_l != NULL);
    
    p_vsa->task_vsa_r = osal_task_create("t-vsa-r", vsa_thread_entry, p_vsa, RT_VSA_THREAD_STACK_SIZE, RT_VSA_THREAD_PRIORITY);
    osal_assert(p_vsa->task_vsa_r != NULL);



    OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_INFO, "module initial\n");
                             
}

/*****************************************************************************
 @funcname: vsa_deinit
 @brief   : vsa module unstall
 @param   : None
 @return  : 
*****************************************************************************/
void vsa_deinit(void)
{
    OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_INFO, "vsa module initial\n");
}

