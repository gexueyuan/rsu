/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : os_timer.h
 @brief  : os_timer.h header file
 @author : wanglei
 @history:
           2015-8-25    wanglei    Created file
           ...
******************************************************************************/
#ifndef __OS_TIMER_H__
#define __OS_TIMER_H__

#include <time.h>
#include "os_pthread.h"

#ifdef  __cplusplus
extern "C" {
#endif


/* `os_timer_add'flag参数宏定义 */
#define TIMER_ONESHOT       (TIMER_ABSTIME << 1)    /* 一次定时器 */
#define TIMER_INTERVAL      (TIMER_ABSTIME << 2)    /* 周期定时器 */
#define TIMER_STOPPED       (TIMER_ABSTIME << 3)    /* 定时器默认处于停止状态 */

#define TIMER_PRIO_LOW      (TK_PRIO(TK_CLS_LOW,    TK_PRIO_NORMAL))
#define TIMER_PRIO_NORMAL   (TK_PRIO(TK_CLS_NORMAL, TK_PRIO_NORMAL))
#define TIMER_PRIO_HIGH     (TK_PRIO(TK_CLS_HIGH,   TK_PRIO_NORMAL))

extern int os_timer_add(int prio, int millisec, int flag, void (*handler)(void *), void * arg, timer_t * timerid);
extern int os_timer_del(timer_t timerid);
extern int os_timer_start(timer_t timerid);
extern int os_timer_stop(timer_t timerid);
extern int os_timer_reset(timer_t timerid);
extern int os_timer_settime(timer_t timerid, int millisec);

#ifdef  __cplusplus
}
#endif


#endif
