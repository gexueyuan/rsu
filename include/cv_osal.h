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

/******************************************************************************
*	o����y:	osal_get_systemtime
*	1|?��:	??��??�̨�3?a?��???������??
*	2?��y:	?T
*	����??:	o��??��y
*	?��?��:	>0			- o��??��y
 ******************************************************************************/
static inline uint32_t osal_get_systemtime(void)
{
	int ret;
    struct os_reltime t;
    ret = os_get_reltime(&t);
    if(ret == 0){
        return t.sec*1000 + t.usec/1000;
    }else{
    	return 0;
    }
}
/******************************************************************************
*	o����y:	osal_get_systimestamp
*	1|?��:	???�̨�3����??��a???a�̡�?��?����??����y
*	2?��y:	?T
*	����??:	??��y
*	?��?��:	>0			- ��??����y
 ******************************************************************************/
static inline uint32_t osal_get_systimestamp(void)
{
	time_t ts;
	struct tm *t;
	uint32_t minutes;

	time(&ts);
	t = gmtime(&ts);

	minutes = t->tm_yday * 60 * 24 + t->tm_hour * 60 + t->tm_min;

	return minutes;
}
/******************************************************************************
*	o����y:	osal_get_systime
*	1|?��:	???�̨�3����??��a???a��??��?��o��??��y����??��y?Y
*	2?��y:	?T
*	����??:	��??��?��o��??��y
*	?��?��:
*			0 - 60000			- ������??y3�꨺y?Y
*			60000 - 60999		- ������?����??
*			61000 - 65534		- ?�訢?��y?Y
*			65535				- ?TD�쨺y?Y
 ******************************************************************************/
static inline uint16_t osal_get_systime(void)
{
	uint16_t val;
	struct tm *t;
	struct timeval tv;
	gettimeofday(&tv, NULL);

	t = gmtime(&tv.tv_sec);

	//����??
	val = t->tm_sec * 1000 + (tv.tv_usec / 1000);

	return val;
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

