/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : os_timer.h
 @brief  : os_timer.h header file
 @author : wanglei
 @history:
           2015-8-25    wanglei    Created file
           ...
******************************************************************************/
#ifndef __OSAL_TIMER_H__
#define __OSAL_TIMER_H__

#include "os_timer.h"
#include "os_core.h"
#include "osal_cmn.h"

#ifdef  __cplusplus
extern "C" {
#endif

osal_timer_t *osal_timer_create(const char* name, void (*handler)(void *), void * arg, int millisec, int flag, int prio);
osal_status_t osal_timer_delete(osal_timer_t *timer);
osal_status_t osal_timer_start(osal_timer_t *timer);
osal_status_t osal_timer_stop(osal_timer_t *timer);
osal_status_t osal_timer_reset(osal_timer_t *timer);
osal_status_t osal_timer_change(osal_timer_t *timer, int millisec);


#ifdef  __cplusplus
}
#endif

#endif
