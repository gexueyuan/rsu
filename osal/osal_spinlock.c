/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : osal_spinlock.c
 @brief  : spinlock 
 @author : wanglei
 @history:
           2015-12-9    wanglei    Created file
           ...
******************************************************************************/
#include "osal_spinlock.h"

int osal_spinlock_init(osal_spinlock_t *spinlock_id , OSAL_SPINLOCK_LEVEL level)
{
    os_spinlock_init(spinlock_id, level);
    if(*spinlock_id == 0)
        return OSAL_ERROR;

    return OSAL_EOK;
}

int osal_spin_lock(osal_spinlock_t *spinlock_id)
{
    os_spinlock_lock(spinlock_id);
    return OSAL_EOK;
}

int osal_spin_unlock(osal_spinlock_t *spinlock_id)
{
    os_spinlock_unlock(spinlock_id);
    return OSAL_EOK;
}

int osal_spinlock_destroy(osal_spinlock_t * spinlock_id)
{
    os_spinlock_destroy(spinlock_id);
    return OSAL_EOK;
}


