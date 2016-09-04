/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : osal_mutex.c
 @brief  : pthread mutex api
 @author : wanglei
 @history:
           2015-8-25    wanglei    Created file
           ...
******************************************************************************/
#include "osal_mutex.h"

osal_mutex_t * osal_mutex_create(const char* name)
{
    int ret = 0;
    name = name;
    osal_mutex_t * mutex = NULL;
    mutex = (osal_mutex_t *)os_malloc(sizeof(osal_mutex_t));
    if (mutex == NULL)
        return NULL;
    memset(mutex, 0, sizeof(osal_mutex_t));

    ret = os_mutex_create(mutex);
    if (ret){    
        os_free(mutex);
        return NULL;
    }
    return mutex;
}

osal_status_t osal_mutex_delete(osal_mutex_t *mutex)
{
    int ret;
    ret = -1;
    if(mutex)
    {  
        ret = os_mutex_destroy(mutex);
        os_free(mutex);
    }
    return (ret == OSAL_EOK) ? OSAL_STATUS_SUCCESS: OSAL_STATUS_ERROR_UNDEFINED; 
}

osal_status_t osal_mutex_lock(osal_mutex_t *mutex)
{
    int ret;
    if (mutex == NULL)
        return OSAL_STATUS_EMPTY;

    ret = os_mutex_lock(mutex);
    return (ret == OSAL_EOK) ? OSAL_STATUS_SUCCESS: OSAL_STATUS_ERROR_UNDEFINED; 
}


osal_status_t osal_mutex_unlock(osal_mutex_t *mutex)
{   
    int ret;
    if (mutex == NULL)
        return OSAL_STATUS_EMPTY;

    ret = os_mutex_unlock(mutex);
    return (ret == OSAL_EOK) ? OSAL_STATUS_SUCCESS: OSAL_STATUS_ERROR_UNDEFINED; 
}

osal_status_t osal_mutex_trylock(osal_mutex_t *mutex)
{
    int ret;
    if (mutex == NULL)
        return OSAL_STATUS_EMPTY;

    ret = os_mutex_trylock(mutex);
    return (ret == OSAL_EOK) ? OSAL_STATUS_SUCCESS: OSAL_STATUS_ERROR_UNDEFINED; 
}


