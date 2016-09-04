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
*	o‘那y:	osal_get_systemtime
*	1|?邦:	??豕??米赤3?a?迆???‘那㊣??
*	2?那y:	?T
*	﹞米??:	o芍??那y
*	?米?¯:	>0			- o芍??那y
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
*	o‘那y:	osal_get_systimestamp
*	1|?邦:	???米赤3那㊣??℅a???a米㊣?～?那﹞??車那y
*	2?那y:	?T
*	﹞米??:	??那y
*	?米?¯:	>0			- ﹞??車那y
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
*	o‘那y:	osal_get_systime
*	1|?邦:	???米赤3那㊣??℅a???a﹞??車?迆o芍??那y那㊣??那y?Y
*	2?那y:	?T
*	﹞米??:	﹞??車?迆o芍??那y
*	?米?¯:
*			0 - 60000			- ㊣赤那??y3㏒那y?Y
*			60000 - 60999		- ㊣赤那?豕辰??
*			61000 - 65534		- ?∟芍?那y?Y
*			65535				- ?TD∫那y?Y
 ******************************************************************************/
static inline uint16_t osal_get_systime(void)
{
	uint16_t val;
	struct tm *t;
	struct timeval tv;
	gettimeofday(&tv, NULL);

	t = gmtime(&tv.tv_sec);

	//豕辰??
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

