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
 * 创建定时器
 flag: TIMER_ONESHOT  一次定时器 
       TIMER_INTERVAL  周期定时器
       TIMER_STOPPED   定时器默认处于停止状态
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
 * 删除定时器
 *
 * @timerid : 要删除的定时器ID。
 *
 * 备注
 * 1. 删除前可以不判断定时器ID是否为0；
 * 2. 调用os_timer_del删除定时器后一定要显式的将保存定时器ID的变量置为0，以避免
 *    定时器的重复删除；
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
 * 启动定时器
 *
 * @timerid : 要操作的定时器ID。
 *
 * 备注
 * 1. 如果对于一个已经启动的定时器调用os_timer_start，则定时器将被重置，即重新
      计时，功能等同于os_timer_reset；
 * 2. 单次定时器超时后可以调用该函数使定时器重新计时；
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
 * 停止定时器
**/
osal_status_t osal_timer_stop(osal_timer_t *timer)
{
    int ret;
    timer_t timerid;

    
    if(timer == NULL)
    {
        return OSAL_STATUS_EMPTY;
    }
    timerid = *timer;
    ret = os_timer_stop(timerid);
    return (ret == OSAL_EOK) ? OSAL_STATUS_SUCCESS: OSAL_STATUS_ERROR_UNDEFINED; 
}

/*
** 重置定时器
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
** 修改定时器超时时间
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
