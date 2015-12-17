/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : os_timer.c
 @brief  : timer 
 @author : wanglei
 @history:
           2015-8-25    wanglei    Created file
           ...
******************************************************************************/

#include "osal_timer.h"


/**
 * ������ʱ��
 flag: TIMER_ONESHOT  һ�ζ�ʱ�� 
       TIMER_INTERVAL  ���ڶ�ʱ��
       TIMER_STOPPED   ��ʱ��Ĭ�ϴ���ֹͣ״̬
 */
osal_timer_t *osal_timer_create(const char* name, void (*handler)(void *), void * arg, int millisec, int flag, int prio)
{
    int ret;
    osal_timer_t *ptimerid;
    timer_t timerid;

    ret = os_timer_add(prio, millisec, flag, handler, arg, &timerid);
    if (ret){
        return NULL;
    }
    ptimerid = (osal_timer_t *)malloc(sizeof(osal_timer_t));
    *ptimerid = timerid;
    return ptimerid;
}

/**
 * ɾ����ʱ��
 *
 * @timerid : Ҫɾ���Ķ�ʱ��ID��
 *
 * ��ע
 * 1. ɾ��ǰ���Բ��ж϶�ʱ��ID�Ƿ�Ϊ0��
 * 2. ����os_timer_delɾ����ʱ����һ��Ҫ��ʽ�Ľ����涨ʱ��ID�ı�����Ϊ0���Ա���
 *    ��ʱ�����ظ�ɾ����
 */
osal_status_t osal_timer_delete(osal_timer_t *timer)
{
    int ret;
    timer_t timerid;
    if (timer == NULL){
        return OSAL_STATUS_EMPTY;
    }
    timerid = *timer;
    ret = os_timer_del(timerid);
    free(timer);
    return (ret == OSAL_EOK) ? OSAL_STATUS_SUCCESS: OSAL_STATUS_ERROR_UNDEFINED; 
}

/**
 * ������ʱ��
 *
 * @timerid : Ҫ�����Ķ�ʱ��ID��
 *
 * ��ע
 * 1. �������һ���Ѿ������Ķ�ʱ������os_timer_start����ʱ���������ã�������
      ��ʱ�����ܵ�ͬ��os_timer_reset��
 * 2. ���ζ�ʱ����ʱ����Ե��øú���ʹ��ʱ�����¼�ʱ��
 */
osal_status_t osal_timer_start(osal_timer_t *timer)
{
    int ret;
    timer_t timerid;
    if (timer == NULL){
        return OSAL_STATUS_EMPTY;
    }
    timerid = *timer;
    ret = os_timer_start(timerid);
    return (ret == OSAL_EOK) ? OSAL_STATUS_SUCCESS: OSAL_STATUS_ERROR_UNDEFINED; 
}

/**
 * ֹͣ��ʱ��
**/
osal_status_t osal_timer_stop(osal_timer_t *timer)
{

    int ret;
    timer_t timerid;
    if (timer == NULL){
        return OSAL_STATUS_EMPTY;
    }
    timerid = *timer;
    ret = os_timer_stop(timerid);
    return (ret == OSAL_EOK) ? OSAL_STATUS_SUCCESS: OSAL_STATUS_ERROR_UNDEFINED; 
}

/*
** ���ö�ʱ��
*/
osal_status_t osal_timer_reset(osal_timer_t *timer)
{
    int ret;
    timer_t timerid;
    if (timer == NULL){
        return OSAL_STATUS_EMPTY;
    }
    timerid = *timer;
    ret = os_timer_start(timerid);
    return (ret == OSAL_EOK) ? OSAL_STATUS_SUCCESS: OSAL_STATUS_ERROR_UNDEFINED; 
}

/*
** �޸Ķ�ʱ����ʱʱ��
*/
osal_status_t osal_timer_change(osal_timer_t *timer, int millisec)
{
    int ret = 0;
    timer_t timerid = *timer;
    
    ret |= os_timer_stop(timerid);
    ret |= os_timer_settime(timerid, millisec);
    ret |= os_timer_start(timerid);
    
    return (ret == OSAL_EOK) ? OSAL_STATUS_SUCCESS: OSAL_STATUS_ERROR_UNDEFINED; 
}
