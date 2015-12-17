/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : os_pthread.h
 @brief  : os_pthread.h header file
 @author : wanglei
 @history:
           2015-8-25    wanglei    Created file
           ...
******************************************************************************/


#ifndef __OSAL_PTHREAD_H__
#define __OSAL_PTHREAD_H__

#include "os_core.h"
#include "osal_cmn.h"
#include "os_pthread.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* 是否保存原函数返回值为指针，还是直接返回osal_task_t */
osal_task_t * osal_task_create(const char * name, void *(*start_routine)(void *), void * arg, 
                                     int stack_size, int prio);
osal_task_t *osal_task_create_joinable(const char * name, int prio, int stack_size, void * (*start_routine)(void *), void * arg);

osal_status_t osal_task_del(osal_task_t *task);

void osal_blockallsigs(void);
void osal_unblocksig(int sig);


#ifdef  __cplusplus
}
#endif

#endif
