/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : cv_vam_rcp.c
 @brief  : this file realize the vehicle Remote Communicat Protocol
 @author : wangyifeng
 @history:
           2014-6-17    wangyifeng    Created file
           2014-7-29    wanglei       modified file: process evam msg 
           ...
******************************************************************************/
#include "cv_osal.h"
#define OSAL_MODULE_DEBUG
#define OSAL_MODULE_DEBUG_LEVEL OSAL_DEBUG_INFO
#define MODULE_NAME "rcp"
#include "cv_osal_dbg.h"
#include "cv_vam_api.h"
#include "cv_vam.h"
#include "cv_cms_def.h"
#include "cv_wnet.h"
#include "J2735.h"
#include "app_msg_format.h"


/*****************************************************************************
 * declaration of variables and functions                                    *
*****************************************************************************/
itis_codes_t itiscode[RSA_TYPE_MAX+1] = 
{
#undef xx
#define xx(SEQ, TYPE, ITISCODE) (ITISCODE),
    VAM_RSA_TYPE_2_ITISCODE
};

float rcp_dbg_distance = 0;
list_head_t test_comm_list;
test_comm_node_t *test_node;

/*****************************************************************************
 * implementation of functions                                               *
*****************************************************************************/
void init_test_list(void)
{
    INIT_LIST_HEAD(&test_comm_list);

}

test_comm_node_t *test_find_sta(uint8_t *temporary_id)
{
    test_comm_node_t *p_sta = NULL, *pos;

    list_for_each_entry(pos, test_comm_node_t, &test_comm_list, list){
        if (memcmp(pos->pid, temporary_id, RCP_TEMP_ID_LEN) == 0){
            p_sta = pos;
            break;
        }
    }

    /* not found, allocate new one */
    if (p_sta == NULL){
        p_sta = (test_comm_node_t*)malloc(sizeof(test_comm_node_t));
        if(p_sta != NULL){
            memset(p_sta,0,sizeof(test_comm_node_t));
            memcpy(p_sta->pid,temporary_id,RCP_TEMP_ID_LEN);
            list_add(&p_sta->list,&test_comm_list);
        }
        else{
            osal_printf("malloc error!");       
        }        
    }

    return p_sta;
}

void empty_test_list(void)
{
    test_comm_node_t *pos = NULL;
/*
    list_for_each_entry(pos, test_comm_node_t, &test_comm_list, list){
        list_del(&pos->list);
        free(pos);
    }
*/
    while (!list_empty(&test_comm_list)) {
        pos = list_first_entry(&test_comm_list, test_comm_node_t, list);
        list_del(&pos->list);
        free(pos);
        }
    
}

void printf_stats(void)
{

    test_comm_node_t *pos;

    list_for_each_entry(pos, test_comm_node_t, &test_comm_list, list){
        osal_printf("\r\npid(%02X %02X %02X %02X)[RX]Act=%d Ratio=%d%% dis=%d\r\n\r\n",\
            pos->pid[0],pos->pid[1],pos->pid[2],pos->pid[3],pos->rx_cnt,pos->rx_cnt,(int)pos->distance_dev);
    }



}



__COMPILE_INLINE__ uint16_t encode_vehicle_alert(uint16_t x)
{
    uint16_t r = 0;


    /* Vehicle break down. */
    if (x & VAM_ALERT_MASK_VBD){
        r |= EventHazardLights;        
    }
    else {
        r &= ~EventHazardLights;
    }

    /* Vehicle emergency brake down. */
    if (x & VAM_ALERT_MASK_EBD){
        r |= EventHardBraking;        
    }
    else {
        r &= ~EventHardBraking;
    }

    /* Vehicle overturned. */
    if (x & VAM_ALERT_MASK_VOT){
        r |= EventDisabledVehicle;
    }
    else {
        r &= ~EventDisabledVehicle;
    }

    return cv_ntohs(r);
}

__COMPILE_INLINE__ uint16_t decode_vehicle_alert(uint16_t x)
{
    uint16_t r = 0;

    
    x = cv_ntohs(x);
    if (x & EventHazardLights) {
        r |= VAM_ALERT_MASK_VBD;        
    }
   
    if (x & EventHardBraking){
        r |= VAM_ALERT_MASK_EBD;        
    }

    if (x & EventDisabledVehicle){
        r |= VAM_ALERT_MASK_VOT;        
    }
    
    return r;
}

/* BEGIN: Added by wanglei, 2015/1/4. for rsa test */
__COMPILE_INLINE__ uint16_t encode_itiscode(uint16_t rsa_mask, itis_codes_t *p_des)
{
    uint16_t r = 0;
    int bit;

    for(bit=0; bit<9; bit++)
    {
        if(rsa_mask & (1<<bit)){
            if (0 == bit){
                r = cv_ntohs(itiscode[bit]);
            }
            else{
                p_des[bit-1] = cv_ntohs(itiscode[bit]);
            }
        }
    }
    
    return r;
}

static void itiscode_2_rsa_mask(itis_codes_t type, uint16_t *rsa_mask)
{
    int i = 0;
    for (i=0; i<RSA_TYPE_MAX; i++)
    {
        if (itiscode[i] == type){
           *rsa_mask |= 1<<i;
           break;
        }
    }
}
__COMPILE_INLINE__ uint16_t decode_itiscode(itis_codes_t typeEvent, itis_codes_t *p_des)
{
    uint16_t k = 0;
	uint16_t rsa_mask = 0;
	uint16_t r;
    r = cv_ntohs(typeEvent);
    itiscode_2_rsa_mask(r, &rsa_mask);    
    for(k=0; k<8; k++)
    {
        r = cv_ntohs(p_des[k]);
        itiscode_2_rsa_mask(r, &rsa_mask);    
    }
    return rsa_mask;

}

int rcp_mda_process(uint8_t msg_hops, 
                      uint8_t msg_count,
                      uint8_t *p_temp_id, 
                      uint8_t *p_forward_id,
                      uint8_t * data,
                      uint32_t datalen)
{
    mda_msg_info_t src;
    mda_envar_t * p_mda;
    int ret;

    p_mda = &p_cms_envar->mda;
    src.left_hops = msg_hops;
    src.msg_count = msg_count;
    memcpy(src.temorary_id, p_temp_id, RCP_TEMP_ID_LEN);
    memcpy(src.forward_id, p_forward_id, RCP_TEMP_ID_LEN);
    
    ret = mda_handle(p_mda, &src, NULL, data, datalen);
    return ret;
}


int rcp_parse_bsm(vam_envar_t *p_vam, wnet_rxinfo_t *rxinfo, uint8_t *databuf, uint32_t datalen)
{
    vam_sta_node_t        *p_sta = NULL;
    rcp_msg_basic_safty_t *p_bsm = (rcp_msg_basic_safty_t *)databuf;;


    /* Return error when data length less then bsm minimum length. */
    if(datalen < (sizeof(rcp_msg_basic_safty_t) - sizeof(vehicle_safety_ext_t)))
    {
        return -1;
    }

    /* Return out when receive my own's bsm message. */
    if(0 == memcmp(p_bsm->header.temporary_id, p_vam->local.pid, RCP_TEMP_ID_LEN))
    {
        return 0;
    }
    
    rcp_mda_process(p_bsm->header.msg_id.hops, p_bsm->header.msg_count, 
                     p_bsm->header.temporary_id, p_bsm->forward_id, databuf, datalen);

    /* Find the current sta node id structure from neighbour list.*/
    p_sta = vam_find_sta(p_vam, p_bsm->header.temporary_id);

    if(p_sta != NULL)
    {
        p_sta->s.dsecond = cv_ntohs(p_bsm->dsecond);
        p_sta->s.time = osal_get_systemtime();

        p_sta->s.pos.longitude = decode_longitude(p_bsm->position.lon);
        p_sta->s.pos.latitude = decode_latitude(p_bsm->position.lat);
        p_sta->s.pos.elevation = decode_elevation(p_bsm->position.elev);
        p_sta->s.pos_accuracy.semi_major_accu = decode_semimajor_axis_accuracy(p_bsm->position.accu.semi_major);
        p_sta->s.pos_accuracy.semi_major_orientation = decode_semimajor_axis_orientation(p_bsm->position.accu.semi_major_orientation);
        p_sta->s.pos_accuracy.semi_minor_accu = decode_semiminor_axis_accuracy(p_bsm->position.accu.semi_minor);

        p_sta->s.transmission_state = p_bsm->motion.transmission_state;
        p_sta->s.speed = decode_absolute_velocity(p_bsm->motion.speed);
        p_sta->s.dir = decode_angle(p_bsm->motion.heading);
        
        p_sta->s.steer_wheel_angle = decode_steer_wheel_angle(p_bsm->motion.steering_wheel_angle);
        
        p_sta->s.acce_set.longitudinal = decode_acceleration(p_bsm->motion.acce.lon);
        p_sta->s.acce_set.lateral = decode_acceleration(p_bsm->motion.acce.lat);
        p_sta->s.acce_set.vertical = decode_vertical_acceleration(p_bsm->motion.acce.vert);
        p_sta->s.acce_set.yaw_rate = decode_yawrate(p_bsm->motion.acce.yaw);

        decode_brake_sytem_status(&p_bsm->brakes, &p_sta->s.brake_stat);

        p_sta->s.vec_size.vec_length = decode_vehicle_length(p_bsm->size.length);
        p_sta->s.vec_size.vec_width = decode_vehicle_width(p_bsm->size.width);

        /* for test  */
        if (1 == g_dbg_print_type){
            rcp_dbg_distance = vsm_get_distance(&p_vam->local.pos, &p_sta->s.pos);
            test_node = test_find_sta(p_sta->s.pid);
            test_node->distance_dev = rcp_dbg_distance;
            test_node->rx_cnt++;
            //osal_printf("cnt is %d\n",test_node->rx_cnt);
        }

        /* Parsing event domain when has extra data. */
        if((sizeof(rcp_msg_basic_safty_t) - sizeof(vehicle_safety_ext_t)) < datalen)
        {
            p_sta->s.alert_mask = decode_vehicle_alert(p_bsm->safetyExt.events); 

            /* inform the app layer once */
            if(p_vam->evt_handler[VAM_EVT_BSM_ALARM_UPDATE] != NULL)
            {
                p_vam->evt_handler[VAM_EVT_BSM_ALARM_UPDATE](&p_sta->s);
            } 
        }
        else
        {
            p_sta->s.alert_mask = 0;
        }

  
    }

    return 0;
}


int rcp_parse_evam(vam_envar_t *p_vam, wnet_rxinfo_t *rxinfo, uint8_t *databuf, uint32_t datalen)
{
    vam_sta_node_t *p_sta;
    rcp_msg_emergency_vehicle_alert_t *p_evam;
    uint16_t alert_mask;

    if (datalen < sizeof(rcp_msg_emergency_vehicle_alert_t)){
        return -1;
    }

    p_evam = (rcp_msg_emergency_vehicle_alert_t *)databuf;

    if (0 == memcmp(p_evam->temporary_id, p_vam->local.pid, RCP_TEMP_ID_LEN)){
        return 0;
    }
    
    rcp_mda_process(p_evam->msg_id.hops, p_evam->rsa.msg_count, 
                     p_evam->temporary_id, p_evam->forward_id, databuf, datalen);


    //TBD
    alert_mask = decode_itiscode(p_evam->rsa.typeEvent, p_evam->rsa.description);
    //rt_kprintf("recv evam: alert_mask = 0x%04x\r\n", alert_mask);

    p_sta = vam_find_sta(p_vam, p_evam->temporary_id);
    if(p_sta != NULL)
    {
    	p_sta->s.dsecond = cv_ntohs(p_evam->rsa.time_stamp);
        p_sta->s.time = osal_get_systemtime();

        p_sta->s.pos.longitude = decode_longitude(p_evam->rsa.position.lon);
        p_sta->s.pos.latitude = decode_latitude(p_evam->rsa.position.lat);
        p_sta->s.pos.elevation = decode_elevation(p_evam->rsa.position.elev);

        p_sta->s.dir = decode_angle(p_evam->rsa.position.heading);
        p_sta->s.speed = decode_absolute_velocity(p_evam->rsa.position.speed.speed);
#if 0
        p_sta->s.acce_set.longitudinal = decode_acceleration(p_evam->motion.acce.lon);
        p_sta->s.acce_set.lateral = decode_acceleration(p_evam->motion.acce.lat);
        p_sta->s.acce_set.vertical = decode_vertical_acceleration(p_evam->motion.acce.vert);
        p_sta->s.acce_set.yaw_rate = decode_yawrate(p_evam->motion.acce.yaw);
#endif
        p_sta->s.alert_mask = alert_mask;

        /* inform the app layer once */
        if(p_vam->evt_handler[VAM_EVT_EVA_UPDATE] != NULL)
        {
            (p_vam->evt_handler[VAM_EVT_EVA_UPDATE])(&p_sta->s);
        }
    }
    return 0;
}


int rcp_parse_rsa(vam_envar_t *p_vam, wnet_rxinfo_t *rxinfo, uint8_t *databuf, uint32_t datalen)
{
    rcp_msg_roadside_alert_t *p_rsa;
    vam_rsa_evt_info_t param;
        
    if (datalen < sizeof(rcp_msg_roadside_alert_t)){
        return -1;
    }

    p_rsa = (rcp_msg_roadside_alert_t *)databuf;

    param.dsecond  = cv_ntohs(p_rsa->time_stamp);
    param.rsa_mask = decode_itiscode(p_rsa->typeEvent, p_rsa->description);
    param.pos.longitude = decode_longitude(p_rsa->position.lon);
    param.pos.latitude = decode_latitude(p_rsa->position.lat);

    if(p_vam->evt_handler[VAM_EVT_RSA_UPDATE] != NULL)
    {
        (p_vam->evt_handler[VAM_EVT_RSA_UPDATE])(&param);
    }

    return 0;
}


int rcp_parse_msg(vam_envar_t *p_vam, wnet_rxinfo_t *rxinfo, uint8_t *databuf, uint32_t datalen)
{
    rcp_msgid_t *p_msgid = (rcp_msgid_t *)databuf;


    /* Error detection. */
    if (datalen < sizeof(rcp_msg_head_t))
    {
        return -1;
    }

    switch(p_msgid->id)
    {
        case RCP_MSG_ID_BSM:  {
            rcp_parse_bsm(p_vam, rxinfo, databuf, datalen);    break;
        } 
        case RCP_MSG_ID_EVAM: {
            rcp_parse_evam(p_vam, rxinfo, databuf, datalen);   break;
        } 
        case RCP_MSG_ID_RSA:  {
            rcp_parse_rsa(p_vam, rxinfo, databuf, datalen);    break;   
        }
        default:              {
                                                               break; 
        }   
    }

    return p_msgid->id;
}


/*****************************************************************************
 @funcname: vam_rcp_recv
 @brief   : RCP receive data frame from network layer
 @param   : wnet_rxinfo_t *rxinfo  
 @param   : uint8_t *databuf      
 @param   : uint32_t datalen      
 @return  : 
*****************************************************************************/
int vam_rcp_recv(wnet_rxinfo_t *rxinfo, uint8_t *databuf, uint32_t datalen)
{
    vam_envar_t *p_vam = &p_cms_envar->vam;
    //osal_printf("vam_rcp_recv...\r\n");
    //vam_add_event_queue(p_vam, VAM_MSG_RCPRX, datalen, (uint32_t)databuf, rxinfo);
    rcp_parse_msg(p_vam, rxinfo, databuf, datalen);
    return 0;
}


int rcp_send_bsm(vam_envar_t *p_vam)
{
    int result = 0;
    rcp_msg_basic_safty_t *p_bsm;
    vam_stastatus_t *p_local = &p_vam->local;
    wnet_txbuf_t *txbuf;
    wnet_txinfo_t *txinfo;
    vam_stastatus_t current;
    int len = sizeof(rcp_msg_basic_safty_t);
	
    txbuf = wnet_get_txbuf();
    if (txbuf == NULL) {
        return -1;
    }

    vam_get_local_current_status(&current);
    p_local = &current;

    p_bsm = (rcp_msg_basic_safty_t *)WNET_TXBUF_DATA_PTR(txbuf);

    p_bsm->header.msg_id.hops = 1;
    p_bsm->header.msg_id.id = RCP_MSG_ID_BSM;

    p_bsm->header.msg_count = p_vam->tx_bsm_msg_cnt++;
    memcpy(p_bsm->header.temporary_id, p_local->pid, RCP_TEMP_ID_LEN);
    if (p_vam->working_param.bsm_hops > 1){
        memcpy(p_bsm->forward_id, p_local->pid, RCP_TEMP_ID_LEN);
    }
    p_bsm->dsecond = cv_ntohs(osal_get_systime());

    p_bsm->position.lon = encode_longitude(p_local->pos.longitude);
    p_bsm->position.lat = encode_latitude(p_local->pos.latitude);
    p_bsm->position.elev = encode_elevation(p_local->pos.elevation);
    p_bsm->position.accu.semi_major = encode_semimajor_axis_accuracy(p_local->pos_accuracy.semi_major_accu);
    p_bsm->position.accu.semi_major_orientation = encode_semimajor_axis_orientation(p_local->pos_accuracy.semi_major_orientation);
    p_bsm->position.accu.semi_minor = encode_semiminor_axis_accuracy(p_local->pos_accuracy.semi_minor_accu);

    p_bsm->motion.transmission_state = p_local->transmission_state;
    p_bsm->motion.heading = encode_angle(p_local->dir);
    p_bsm->motion.speed = encode_absolute_velocity(p_local->speed);
    p_bsm->motion.steering_wheel_angle = encode_steer_wheel_angle(p_local->steer_wheel_angle);
    p_bsm->motion.acce.lon = encode_acceleration(p_local->acce_set.longitudinal);
    p_bsm->motion.acce.lat = encode_acceleration(p_local->acce_set.lateral);
    p_bsm->motion.acce.vert = encode_vertical_acceleration(p_local->acce_set.vertical);
    p_bsm->motion.acce.yaw = encode_yawrate(p_local->acce_set.yaw_rate);

    encode_brake_sytem_status(&p_local->brake_stat, &p_bsm->brakes);

    p_bsm->size.length = encode_vehicle_length(p_local->vec_size.vec_length);
    p_bsm->size.width = encode_vehicle_width(p_local->vec_size.vec_width);

    if(p_vam->flag & VAM_FLAG_TX_BSM_ALERT)
    {
        p_bsm->header.msg_id.hops = p_vam->working_param.bsm_hops;
        /* need to send part2 safetyextenrion */
        p_bsm->safetyExt.events = encode_vehicle_alert(p_vam->local.alert_mask);
    }
    else
    {
        len -= sizeof(vehicle_safety_ext_t);
    }


    txinfo = WNET_TXBUF_INFO_PTR(txbuf);
    memset(txinfo, 0, sizeof(wnet_txinfo_t));
    memcpy(txinfo->dest.dsmp.addr, "\xFF\xFF\xFF\xFF\xFF\xFF", MACADDR_LENGTH);
    txinfo->dest.dsmp.aid = 0x00000020;
    txinfo->protocol = WNET_TRANS_PROT_DSMP;
    txinfo->encryption = WNET_TRANS_ENCRYPT_NONE;
    txinfo->prority = WNET_TRANS_RRORITY_NORMAL;
    txinfo->timestamp = osal_get_systimestamp();

    result = wnet_send(txinfo, (uint8_t *)p_bsm, len);
    
    wnet_release_txbuf(txbuf);

    return result;
    
}

int rcp_send_evam(vam_envar_t *p_vam)
{
    int result = 0;
    rcp_msg_emergency_vehicle_alert_t *p_evam;
    vam_stastatus_t *p_local = &p_vam->local;
    wnet_txbuf_t *txbuf;
    wnet_txinfo_t *txinfo;
    vam_stastatus_t current;
	
    txbuf = wnet_get_txbuf();
    if (txbuf == NULL) {
        return -1;
    }

    vam_get_local_current_status(&current);
    p_local = &current;

    p_evam = (rcp_msg_emergency_vehicle_alert_t *)WNET_TXBUF_DATA_PTR(txbuf);

    p_evam->msg_id.hops = p_vam->working_param.evam_hops;
    p_evam->msg_id.id = RCP_MSG_ID_EVAM;
    memcpy(p_evam->temporary_id, p_local->pid, RCP_TEMP_ID_LEN);

    if (p_vam->working_param.evam_hops > 1){
        memcpy(p_evam->forward_id, p_local->pid, RCP_TEMP_ID_LEN);
    }

    p_evam->time_stamp = cv_ntohs(osal_get_systime());
    p_evam->rsa.msg_count = p_vam->tx_evam_msg_cnt++;
    p_evam->rsa.position.lon = encode_longitude(p_local->pos.longitude);
    p_evam->rsa.position.lat = encode_latitude(p_local->pos.latitude);
    p_evam->rsa.position.elev = encode_elevation(p_local->pos.elevation);
    p_evam->rsa.position.heading = encode_angle(p_local->dir);
    p_evam->rsa.position.speed.transmissionState = TRANS_STATE_Forward;
    p_evam->rsa.position.speed.speed = encode_absolute_velocity(p_local->speed);
    //TBD
    p_evam->rsa.typeEvent = encode_itiscode(p_local->alert_mask, p_evam->rsa.description); 
    

    txinfo = WNET_TXBUF_INFO_PTR(txbuf);
//    memset(txinfo, 0, sizeof(wnet_txinfo_t));
    memcpy(txinfo->dest.dsmp.addr, "\xFF\xFF\xFF\xFF\xFF\xFF", MACADDR_LENGTH);
    txinfo->dest.dsmp.aid = 0x00000020;
    txinfo->protocol = WNET_TRANS_PROT_DSMP;
    txinfo->encryption = WNET_TRANS_ENCRYPT_NONE;
    txinfo->prority = WNET_TRANS_RRORITY_EMERGENCY;
    txinfo->timestamp = osal_get_systimestamp();

    result = wnet_send(txinfo, (uint8_t *)p_evam, sizeof(rcp_msg_emergency_vehicle_alert_t));

    wnet_release_txbuf(txbuf);

    return result;
}



int rcp_send_rsa(vam_envar_t *p_vam)
{
    int result = 0;
    rcp_msg_roadside_alert_t *p_rsa;
    vam_stastatus_t *p_local = &p_vam->local;
    
    wnet_txbuf_t *txbuf;
    wnet_txinfo_t *txinfo;

    txbuf = wnet_get_txbuf();
    
    if (txbuf == NULL) {
        osal_printf("get txbuf failed line%d", __LINE__);
        return -1;
    }

    /* The RSU position is fixed */
#if 0
    vam_stastatus_t current;
    vam_get_local_current_status(&current);
    p_local = &current;
#endif

    p_rsa = (rcp_msg_roadside_alert_t *)WNET_TXBUF_DATA_PTR(txbuf);

    p_rsa->msg_id.hops = p_vam->working_param.bsm_hops;
    p_rsa->msg_id.id = RCP_MSG_ID_RSA;
    p_rsa->msg_count = p_vam->tx_rsa_msg_cnt++;
    vam_active_rsa(RSA_TYPE_CURVE);
    p_rsa->typeEvent = encode_itiscode(p_local->alert_mask, p_rsa->description);
    p_rsa->time_stamp = cv_ntohs(osal_get_systime());
#if 0
    p_local->pos.lon = 132.327144*3.1415926/180.0;
    p_local->pos.lat = 40.0*3.1415926/180.0;
#endif
    p_rsa->position.lon = encode_longitude(p_local->pos.longitude);
    p_rsa->position.lat = encode_latitude(p_local->pos.latitude);
    p_rsa->position.elev = encode_elevation(p_local->pos.elevation);
    p_rsa->position.heading = encode_angle(p_local->dir);
    p_rsa->position.speed.speed = encode_absolute_velocity(p_local->speed);

    txinfo = WNET_TXBUF_INFO_PTR(txbuf);
//    memset(txinfo, 0, sizeof(wnet_txinfo_t));
    memcpy(txinfo->dest.dsmp.addr, "\xFF\xFF\xFF\xFF\xFF\xFF", MACADDR_LENGTH);
    txinfo->dest.dsmp.aid = 0x00000020;
    txinfo->protocol = WNET_TRANS_PROT_DSMP;
    txinfo->encryption = WNET_TRANS_ENCRYPT_NONE;
    txinfo->prority = WNET_TRANS_RRORITY_EMERGENCY;
    txinfo->timestamp = osal_get_systimestamp();

    result = wnet_send(txinfo, (uint8_t *)p_rsa, sizeof(rcp_msg_roadside_alert_t));
    if (result) 
    {
        osal_printf("wnet_send failed line%d", __LINE__);
    }
    wnet_release_txbuf(txbuf);
    
    return result;
}


int rcp_send_forward_msg(wnet_txbuf_t *txbuf)
{
    wnet_txinfo_t *txinfo;
    rcp_msgid_t *p_msgid;
    rcp_msg_basic_safty_t *p_bsm;
    rcp_msg_emergency_vehicle_alert_t *p_evam;

    vam_envar_t *p_vam = &p_cms_envar->vam;
    
    txinfo = WNET_TXBUF_INFO_PTR(txbuf);
    memset(txinfo, 0, sizeof(wnet_txinfo_t));
    memcpy(txinfo->dest.dsmp.addr, "\xFF\xFF\xFF\xFF\xFF\xFF", MACADDR_LENGTH);
    txinfo->dest.dsmp.aid = 0x00000020;
    txinfo->protocol = WNET_TRANS_PROT_DSMP;
    txinfo->encryption = WNET_TRANS_ENCRYPT_NONE;
    txinfo->prority = WNET_TRANS_RRORITY_NORMAL;//WNET_TRANS_RRORITY_EMERGENCY;
    txinfo->timestamp = osal_get_systimestamp();

    /* modify the forward_id of msgdata */
    p_msgid = (rcp_msgid_t *)(WNET_TXBUF_DATA_PTR(txbuf));
    if (RCP_MSG_ID_BSM == p_msgid->id){
        p_bsm = (rcp_msg_basic_safty_t *)WNET_TXBUF_DATA_PTR(txbuf);
        memcpy(p_bsm->forward_id, p_vam->local.pid, RCP_TEMP_ID_LEN);
    }
    else if(RCP_MSG_ID_EVAM == p_msgid->id){
        p_evam = (rcp_msg_emergency_vehicle_alert_t *)WNET_TXBUF_DATA_PTR(txbuf);
        memcpy(p_evam->forward_id, p_vam->local.pid, RCP_TEMP_ID_LEN);    
    }
    else {
        return -1;
    }
    
    return wnet_send(txinfo, WNET_TXBUF_DATA_PTR(txbuf), txbuf->data_len);
}
wnet_txbuf_t *rcp_create_forward_msg(uint8_t left_hops, uint8_t *pdata, uint32_t length)
{
    rcp_msgid_t *p_msg;
    wnet_txbuf_t *txbuf = NULL;

    p_msg = (rcp_msgid_t *)pdata;
    p_msg->hops = left_hops;

    txbuf = wnet_get_txbuf();
    if (!txbuf) {
        return NULL;
    }

    memcpy(WNET_TXBUF_DATA_PTR(txbuf), pdata, length);
    txbuf->data_len = length;
    
    return txbuf;
}







//////////////////////////////////////////////////////////////
//all below just for test
//////////////////////////////////////////////////////////////

void timer_send_rsa_callback(void* parameter)
{
    vam_envar_t *p_vam = (vam_envar_t *)parameter;   
    rcp_send_rsa(p_vam);
}
void test_rsa(int flag)
{
    vam_envar_t *p_vam = &p_cms_envar->vam;
    osal_printf("rsatype = %d , %d\r\n", RSA_TYPE_SPEED_RESTRICTION, RSA_TYPE_MAX);
    if(flag && !p_vam->timer_send_rsa){
        vam_stop();  
        p_vam->timer_send_rsa = osal_timer_create("tm_rsa", timer_send_rsa_callback, p_vam, 1000, \
                                        TIMER_INTERVAL|TIMER_STOPPED, TIMER_PRIO_NORMAL);
        osal_timer_start(p_vam->timer_send_rsa);
    }
    else{
        if(p_vam->timer_send_rsa){
            osal_timer_stop(p_vam->timer_send_rsa);
        }
    }   
}


osal_timer_t *timer_test_bsm_rx;
void timer_test_bsm_rx_callback(void* parameter)
{
    vam_stastatus_t sta;
    vam_stastatus_t *p_local;

    rcp_msg_basic_safty_t test_bsm_rx;
    rcp_msg_basic_safty_t *p_bsm;

    vam_envar_t *p_vam = &p_cms_envar->vam;

    p_local = &sta;
    p_local->pos.latitude = 40.0; //39.5427f;
    p_local->pos.longitude = 120.0;//116.2317f;
    p_local->dir = 90.0;//
    p_local->pid[0] = 0x02;
    p_local->pid[1] = 0x04;
    p_local->pid[2] = 0x06;
    p_local->pid[3] = 0x08;


    p_bsm = (rcp_msg_basic_safty_t *)&test_bsm_rx;   
    /* construct a fake message */
    p_bsm->header.msg_id.hops = 1;
    p_bsm->header.msg_id.id = RCP_MSG_ID_BSM;
    p_bsm->header.msg_count = 0;
    memcpy(p_bsm->header.temporary_id, p_local->pid, RCP_TEMP_ID_LEN);
    p_bsm->dsecond = osal_get_systemtime();

    p_bsm->position.lon = encode_longitude(p_local->pos.longitude);
    p_bsm->position.lat = encode_latitude(p_local->pos.latitude);
    p_bsm->position.elev = encode_elevation(p_local->pos.elevation);
    p_bsm->position.accu.semi_major = encode_semimajor_axis_accuracy(p_local->pos_accuracy.semi_major_accu);
    p_bsm->position.accu.semi_major_orientation = encode_semimajor_axis_orientation(p_local->pos_accuracy.semi_major_orientation);
    p_bsm->position.accu.semi_minor = encode_semiminor_axis_accuracy(p_local->pos_accuracy.semi_minor_accu);

    p_bsm->motion.transmission_state = p_local->transmission_state;
    p_bsm->motion.heading = encode_angle(p_local->dir);
    p_bsm->motion.speed = encode_absolute_velocity(p_local->speed);
    p_bsm->motion.steering_wheel_angle = encode_steer_wheel_angle(p_local->steer_wheel_angle);
    p_bsm->motion.acce.lon = encode_acceleration(p_local->acce_set.longitudinal);
    p_bsm->motion.acce.lat = encode_acceleration(p_local->acce_set.lateral);
    p_bsm->motion.acce.vert = encode_vertical_acceleration(p_local->acce_set.vertical);
    p_bsm->motion.acce.yaw = encode_yawrate(p_local->acce_set.yaw_rate);
    
    encode_brake_sytem_status(&p_local->brake_stat, &p_bsm->brakes);

    p_bsm->size.length = encode_vehicle_length(p_local->vec_size.vec_length);
    p_bsm->size.width = encode_vehicle_width(p_local->vec_size.vec_width);

    rcp_parse_bsm(p_vam, NULL, (uint8_t *)p_bsm, (sizeof(rcp_msg_basic_safty_t) - sizeof(vehicle_safety_ext_t)));
}

void tb1(void)
{
    timer_test_bsm_rx = osal_timer_create("tm-tb",timer_test_bsm_rx_callback,NULL,\
        2400, TIMER_INTERVAL|TIMER_STOPPED, TIMER_PRIO_NORMAL);

    osal_timer_start(timer_test_bsm_rx);
}

void stop_tb1(void)
{
	osal_timer_stop(timer_test_bsm_rx);
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(test_rsa, test sending rsa);
FINSH_FUNCTION_EXPORT(tb1, test bsm receiving);
FINSH_FUNCTION_EXPORT(stop_tb1, stop test bsm receiving);
#endif
