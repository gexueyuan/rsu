/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : osal_spinlock.h
 @brief  : osal_spinlock.c header file
 @author : wanglei
 @history:
           2015-8-14    wanglei    Created file
           ...
******************************************************************************/

#ifndef _OSAL_SPINLOCK_H_
#define _OSAL_SPINLOCK_H_

#include "os_core.h"
#include "osal_cmn.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum
{
    SPINLOCK_CMN = 0,  /*spin_lock*/
    SPINLOCK_BH  = 1,  /*spin_lock_bh*/
    SPINLOCK_IRQ = 2,  /*spin_lock_irq*/
    SPINLOCK_SMP = 3,  /*spin_lock_irqsave*/
} OSAL_SPINLOCK_LEVEL;

int osal_spinlock_init(osal_spinlock_t *spinlock_id , OSAL_SPINLOCK_LEVEL level);
int osal_spin_lock(osal_spinlock_t *spinlock_id);
int osal_spin_unlock(osal_spinlock_t *spinlock_id);
int osal_spinlock_destroy(osal_spinlock_t *spinlock_id);

#ifdef  __cplusplus
}
#endif

#endif

