/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : cv_vam_main.c
 @brief  : this file include main function of vehicle application middleware
           
 @author : wangyifeng
 @history:
           2014-6-19    wangyifeng    Created file
           ...
******************************************************************************/
#include "cv_osal.h"
#define OSAL_MODULE_DEBUG
#define OSAL_MODULE_DEBUG_LEVEL OSAL_DEBUG_INFO
#define MODULE_NAME "vam"
#include "cv_osal_dbg.h"


#include "cv_vam.h"
#include "cv_cms_def.h"
#include "cv_wnet.h"
#include "nmea.h"




/*****************************************************************************
 * declaration of variables and functions                                    *
*****************************************************************************/
#define BSM_SEND_PERIOD_DEFAULT      1000
#define BSM_GPS_LIFE_DEFAULT         5000
#define NEIGHBOUR_LIFE_ACCUR         1000
#define EVAM_SEND_PERIOD_DEFAULT     (50)

extern void timer_send_bsm_callback(void* parameter);
extern void timer_send_evam_callback(void* parameter);
extern void timer_neigh_time_callback(void* parameter);
extern void timer_gps_life_callback(void* parameter);

extern uint8_t des(int offset);
extern void init_test_list(void);

vam_envar_t *p_vam_envar;

/*****************************************************************************
 * implementation of functions                                               *
*****************************************************************************/



void vam_main_proc(vam_envar_t *p_vam, sys_msg_t *p_msg)
{  
    switch(p_msg->id)
    {
        case VAM_MSG_START:
        {
            OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_TRACE, "%s: VAM_MSG_START. \n",__FUNCTION__);

            p_vam->flag |= (VAM_FLAG_RX | VAM_FLAG_TX_BSM);

            osal_timer_start(p_vam->timer_neighbour_life);
            vsm_start_bsm_broadcast(p_vam);
            break;    
        }
        case VAM_MSG_STOP:
        {
            /* Stop send bsm timer. */
            if(p_vam->timer_send_bsm != NULL)
            {
                osal_timer_stop(p_vam->timer_send_bsm);
                //p_vam->timer_send_bsm = NULL;
            }

            /* Stop neighbour life timer. */
            if(p_vam->timer_neighbour_life)
            {
                osal_timer_stop(p_vam->timer_neighbour_life);
                //p_vam->timer_neighbour_life = NULL;
            }
            
            p_vam->flag &= ~(VAM_FLAG_RX | VAM_FLAG_TX_BSM);
            
            break;
        }
        case VAM_MSG_RCPTX:
        {
            /* Send bsm message. */
            if(p_msg->argc == RCP_MSG_ID_BSM)  
            {
                rcp_send_bsm(p_vam);
            }

            /* Send evam message. */
            if(p_msg->argc == RCP_MSG_ID_EVAM) 
            {
                rcp_send_evam(p_vam);
            }

            /* Send rsa message. */
            if(p_msg->argc == RCP_MSG_ID_RSA)  
            {
                rcp_send_rsa(p_vam);
            }
            break;
        }
        case VAM_MSG_RCPRX:
        {
            rcp_parse_msg(p_vam, (wnet_rxinfo_t *)p_msg->argv, (uint8_t *)p_msg->argc, p_msg->len);
            wnet_release_rxbuf(WNET_RXBUF_PTR(p_msg->argv));
            
            break;
        }
        case VAM_MSG_NEIGH_TIMEOUT:
        {
            
            vam_update_sta(p_vam);
            break;
        }
        case VAM_MSG_GPSDATA:
            lip_gps_proc(p_vam, (uint8_t *)p_msg->argv, p_msg->len);
            break;
            
        default:
        {
            break;
        }       
    }
}

void * vam_thread_entry (void *parameter)
{
    int err;
    sys_msg_t *p_msg;
    vam_envar_t *p_vam = (vam_envar_t *)parameter;
    uint32_t len = 0;
    uint8_t buf[VAM_MQ_MSG_SIZE];
    int i=0;
    OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_INFO, "%s: ---->\n", __FUNCTION__);

    p_msg = (sys_msg_t *)buf;
	while(1){
        memset(buf, 0, VAM_MQ_MSG_SIZE);
        err = osal_queue_recv(p_vam->queue_vam, buf, &len, OSAL_WAITING_FOREVER);
        if (err == OSAL_STATUS_SUCCESS && len > 0){
            printf("msg id is %x,time is %d\n",p_msg->id,osal_get_systemtime()/1000);
            //if((p_msg->id == VAM_MSG_GPSDATA))
                //printf("get gps time is %d\n",osal_get_systemtime()/1000);
                //printf("get gps package %d\n",i);
            //printf("start is %d\n\n",osal_get_systemtime()/1000);
            vam_main_proc(p_vam, p_msg);           
            //printf("end is %d\n\n",osal_get_systemtime()/1000);
        }
        else{
            OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_ERROR, "%s: osal_queue_recv error [%d]\n",\
                            __FUNCTION__, err);
        }
	}
}

int vam_add_event_queue(vam_envar_t *p_vam, 
                             uint16_t msg_id, 
                             uint16_t msg_len, 
                             uint32_t msg_argc,
                             void    *msg_argv)
{
    int err = OSAL_STATUS_NOMEM;
    sys_msg_t *p_msg = NULL;

    uint32_t len = 0;
    uint8_t buf[VAM_MQ_MSG_SIZE] = {0};

    p_msg = (sys_msg_t *)buf;
    len = sizeof(sys_msg_t);
    if (p_msg) {
        p_msg->id = msg_id;
        p_msg->len = msg_len;
        p_msg->argc = msg_argc;
        p_msg->argv = msg_argv;
        err = osal_queue_send(p_vam->queue_vam, buf, len, 0, OSAL_NO_WAIT);
        
    
        if (err != OSAL_STATUS_SUCCESS) {
            OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_WARN, "%s: failed=[%d], msg=%04x\n",\
                               __FUNCTION__, err, msg_id);
        }
    }

    return err;
}

int vam_add_event_queue_2(vam_envar_t *p_vam, void *p_msg, uint32_t len)
{
    int err;
    
    err = osal_queue_send(p_vam->queue_vam, p_msg, len, 0, OSAL_NO_WAIT);
    if (err != OSAL_STATUS_SUCCESS){
        OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_WARN, "%s: failed=[%d], msg=%04x\n",\
                         __FUNCTION__, err, ((sys_msg_t *)p_msg)->id);
    }

    return err;
}

/*****************************************************************************
 @funcname: vam_init
 @brief   : vam module initial
 @param   : None
 @return  : 
*****************************************************************************/

void vam_init(void)
{
    int i;
    vam_envar_t *p_vam = &p_cms_envar->vam;
    uint8_t zero_pid[RCP_TEMP_ID_LEN] = {0};

    p_vam_envar = p_vam;
    
    memset(p_vam, 0, sizeof(vam_envar_t));
    memcpy(&p_vam->working_param, &p_cms_param->vam, sizeof(vam_config_t));

   
    if (0 == memcmp(p_cms_param->pid, zero_pid, RCP_TEMP_ID_LEN)){
        for (i=0; i<RCP_TEMP_ID_LEN; i++){
            p_vam->local.pid[i] = des(MACADDR_LENGTH-1-i);
        }
    }
    else {
        memcpy(p_vam->local.pid, p_cms_param->pid, RCP_TEMP_ID_LEN);
    }
    OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_WARN, "PID: %02x %02x %02x %02x\r\n", \
        p_vam->local.pid[0], p_vam->local.pid[1], p_vam->local.pid[2], p_vam->local.pid[3]);

    INIT_LIST_HEAD(&p_vam->neighbour_list);
    INIT_LIST_HEAD(&p_vam->sta_free_list);
    for(i = 0;i< VAM_NEIGHBOUR_MAXNUM;i++){
        list_add_tail(&p_vam->remote[i].list, &p_vam->sta_free_list);
    }
    
    init_test_list();
     /* os object for vam */
    p_vam->queue_vam = osal_queue_create("q-vam", VAM_QUEUE_SIZE, VAM_MQ_MSG_SIZE);
    osal_assert(p_vam->queue_vam != NULL);

	p_vam->task_vam = osal_task_create("tk-vam",
                           vam_thread_entry, p_vam,
                           RT_VAM_THREAD_STACK_SIZE, RT_VAM_THREAD_PRIORITY);
    osal_assert(p_vam->task_vam != NULL);
        
    p_vam->timer_send_bsm = osal_timer_create("tm-sb",timer_send_bsm_callback,p_vam,\
        BSM_SEND_PERIOD_DEFAULT, TIMER_INTERVAL|TIMER_STOPPED, TIMER_PRIO_NORMAL); 					
    osal_assert(p_vam->timer_send_bsm != NULL);


    p_vam->timer_send_evam = osal_timer_create("tm-se",timer_send_evam_callback, p_vam,\
        EVAM_SEND_PERIOD_DEFAULT, TIMER_INTERVAL|TIMER_STOPPED, TIMER_PRIO_NORMAL); 					
    osal_assert(p_vam->timer_send_evam != NULL);

    p_vam->timer_gps_life = osal_timer_create("tm-gl",timer_gps_life_callback,p_vam,\
        BSM_GPS_LIFE_DEFAULT, TIMER_ONESHOT|TIMER_STOPPED, TIMER_PRIO_NORMAL); 					
    osal_assert(p_vam->timer_gps_life != NULL);

    p_vam->timer_neighbour_life = osal_timer_create("tm-nl",timer_neigh_time_callback,p_vam,\
        NEIGHBOUR_LIFE_ACCUR, TIMER_INTERVAL|TIMER_STOPPED, TIMER_PRIO_NORMAL); 					
    osal_assert(p_vam->timer_neighbour_life != NULL);

    p_vam->sem_sta = osal_sem_create("s-sta", 1);
    osal_assert(p_vam->sem_sta != NULL);

	OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_INFO, "module initial\n");
}

/*****************************************************************************
 @funcname: vam_deinit
 @brief   : vam module unstall
 @param   : None
 @return  : 
*****************************************************************************/
void vam_deinit()
{
    vam_envar_t *p_vam = &p_cms_envar->vam;
    osal_timer_delete(p_vam->timer_send_bsm);
    osal_timer_delete(p_vam->timer_send_evam);
    osal_timer_delete(p_vam->timer_gps_life);
    osal_timer_delete(p_vam->timer_neighbour_life);
    osal_task_del(p_vam->task_vam);
    osal_sem_delete(p_vam->sem_sta);
    
	OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_INFO, "module deinit\n");
}


void dump_pos(vam_stastatus_t *p_sta)
{
    char str[64];

    osal_printf("------------sta---------\n");
    osal_printf("PID:%02x-%02x-%02x-%02x\n", p_sta->pid[0], p_sta->pid[1]\
                                          , p_sta->pid[2], p_sta->pid[3]);
    sprintf(str,"%f", p_sta->pos.latitude);
    osal_printf("pos.lat:%s\n", str);
    sprintf(str,"%f", p_sta->pos.longitude);
    osal_printf("pos.lon:%s\n", str);
    sprintf(str,"%f", p_sta->pos.elevation);
    osal_printf("pos.elev:%s\n", str);
    sprintf(str,"%f", p_sta->pos_accuracy.semi_major_accu);
    osal_printf("pos.accu.semi_major_accu:%s\n", str);
    sprintf(str,"%f", p_sta->pos_accuracy.semi_major_orientation);
    osal_printf("pos.accu.semi_major_orientation:%s\n", str);
    sprintf(str,"%f", p_sta->pos_accuracy.semi_minor_accu);
    osal_printf("pos.accu.semi_minor_accu:%s\n", str);
    sprintf(str,"%f", p_sta->dir);
    osal_printf("pos.heading:%s\n", str);
    sprintf(str,"%f", p_sta->speed);
    osal_printf("pos.speed:%s\n", str);
    osal_printf("------------end---------\n");
}

