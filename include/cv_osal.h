/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : cv_osal.h
 @brief  : This is the OS adapter layer realization.
 @author : wangyf
 @history:
           2014-11-12    wangyf    port from project 'cuckoo'. 
           ...
******************************************************************************/
#ifndef __CV_OSAL_H__
#define __CV_OSAL_H__

#include "osal_cmn.h"
#include "osal_api.h"

/**
 * Basic difinitions
 */
#ifndef NULL
#define NULL (0)
#endif

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

typedef uint8_t bool_t;

#ifndef RELEASE
#define OS_ASSERT(EX)                                                         \
if (!(EX))                                                                    \
{                                                                             \
    volatile char dummy = 0;                                                  \
    osal_printf("(%s) assert failed at %s:%d \n", #EX, __FUNCTION__, __LINE__);\
    while (dummy == 0);                                                       \
}
#else
#define OS_ASSERT(EX)
#endif

#define osal_assert(c) OS_ASSERT(c)

/**
 * [Util]
*/
static inline void osal_delay(int usec)
{
    os_delay(usec);
}

static inline uint32_t osal_get_systemtime(void)
{
    struct os_time t;
    if (os_get_reltime(&t)){
        return t.sec*1000 + t.usec/1000;
    }
    return 0;
}

/**
 * Notice: The follow IRQ functions are not able to be nested.
 */
//static rt_base_t cpu_sr;
static inline void osal_enter_critical(osal_mutex_t *mutex)
{
    //cpu_sr = rt_hw_interrupt_disable();
    if (mutex != NULL){
        osal_mutex_lock(mutex);
    }
}

static inline void osal_leave_critical(osal_mutex_t *mutex)
{
    //rt_hw_interrupt_enable(cpu_sr);
    if (mutex != NULL){
        osal_mutex_unlock(mutex);
    }
}

static inline void *osal_malloc(uint32_t size)
{
    return os_malloc(size);
}

static inline void osal_free(void *pointer)
{
    os_free(pointer);
}


#endif /* __CV_OSAL_H__ */

