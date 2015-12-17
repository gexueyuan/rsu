/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : osal_pthread.c
 @brief  : pthread os adapt layer
 @author : wanglei
 @history:
           2015-8-25    wanglei    Created file
           ...
******************************************************************************/

#include "osal_thread.h"
#include <limits.h>

void osal_blockallsigs(void)
{
    os_blockallsigs();
}

void osal_unblocksig(int sig)
{
    os_unblocksig(sig);
}

osal_task_t * osal_task_create(const char * name, void *(*start_routine)(void *), void * arg, 
                                     int stack_size, int prio)
{
    int ret;
    osal_task_t *thread;
    thread = (osal_task_t *)malloc(sizeof(osal_task_t));
    if (thread == NULL){
        return NULL;
    }

    ret = os_pthread_create(thread, name, prio, stack_size, start_routine, arg);
    if (ret){
        return NULL;
    }
    return thread;
}
osal_task_t * osal_task_create_joinable(const char * name, int prio, int stack_size, void * (*start_routine)(void *), void * arg)
{
    int ret;
    osal_task_t *thread;
    thread = (osal_task_t *)malloc(sizeof(osal_task_t));
    if (thread == NULL){
        return NULL;
    }
    ret = os_pthread_create_joinable(thread, name, prio, stack_size, start_routine, arg);
    if (ret){
        return NULL;
    }
    return thread;
}

osal_status_t osal_task_del(osal_task_t *task)
{
    free(task);
    return OSAL_STATUS_SUCCESS; 
}
