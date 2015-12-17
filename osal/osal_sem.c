/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : osal_sem.c
 @brief  : Semaphore api
 @author : wanglei
 @history:
           2015-8-25    wanglei    Created file
           ...
******************************************************************************/
#include "osal_sem.h"

osal_sem_t *osal_sem_create(const char* name, uint32_t count)
{
    osal_sem_t * psem = NULL;
    psem = (osal_sem_t *)malloc(sizeof(osal_sem_t));
    if (psem == NULL)
        return NULL;
    
    memset(psem, 0, sizeof(osal_sem_t));

    if (os_sem_init(psem, 0, count)){  
        free(psem);
        return NULL;
    }
    return psem;
}

osal_status_t osal_sem_delete(osal_sem_t *sem)
{
    int error;
    error = os_sem_destroy(sem);
    os_free(sem);
    return (error == OSAL_EOK) ? OSAL_STATUS_SUCCESS: OSAL_STATUS_ERROR_UNDEFINED; 
}

osal_status_t osal_sem_take(osal_sem_t *sem, int32_t wait_time)
{
    int error;
    osal_status_t status;
    
    error = os_sem_wait(sem, wait_time);
    if (error == OSAL_EOK) {
        status = OSAL_STATUS_SUCCESS;
    }
    else if (error == -1) {
        status = OSAL_STATUS_ERROR_UNDEFINED;
    }
    else {
        status = OSAL_STATUS_TIMEOUT;
    }

    return status;
}

osal_status_t osal_sem_release(osal_sem_t *sem)
{
    int error;
    error = os_sem_post(sem);
    return (error == OSAL_EOK) ? OSAL_STATUS_SUCCESS: OSAL_STATUS_ERROR_UNDEFINED; 
}
