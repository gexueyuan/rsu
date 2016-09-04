/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : osal_sem.h
 @brief  : osal_sem.c header file
 @author : wanglei
 @history:
           2015-8-25    wanglei    Created file
           ...
******************************************************************************/

#ifndef _OSAL_SEM_H_
#define _OSAL_SEM_H_

#include "os_core.h"
#include "osal_cmn.h"

#define OSAL_NO_WAIT                 (0)
#define OSAL_WAITING_FOREVER         (-1)

osal_sem_t *     osal_sem_create(const char* name, uint32_t count);
osal_status_t    osal_sem_delete(osal_sem_t *sem_id);
osal_status_t    osal_sem_wait(osal_sem_t sem_id, uint32_t timeout_msec);
osal_status_t    osal_sem_post(osal_sem_t sem_id);
osal_status_t 	 osal_sem_take(osal_sem_t *sem, int32_t wait_time);
osal_status_t    osal_sem_release(osal_sem_t *sem);
#endif

