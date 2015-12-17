/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : rc_pthread.c
 @brief  : rc_pthread.h header file
 @author : wanglei
 @history:
           2015-12-1    wanglei    Created file
           ...
******************************************************************************/

#ifndef __RC_PTHREAD_H
#define __RC_PTHREAD_H

#include <pthread.h>

#ifdef  __cplusplus
extern "C" {
#endif

#define TK_CLS_LOW              (10)
#define TK_CLS_BELOW_NORMAL     (20)
#define TK_CLS_NORMAL           (30)
#define TK_CLS_ABOVE_NORMAL     (40)
#define TK_CLS_HIGH             (48)

#define TK_PRIO_LOW             (-2)
#define TK_PRIO_NORMAL          (0)
#define TK_PRIO_HIGH            (2)

#define TK_PRIO(cls, prio)      ((cls) + (prio))
#define TK_PRIO_DEFAULT         (TK_PRIO(TK_CLS_NORMAL, TK_PRIO_NORMAL))

#define TK_NAME_PREFIX          "tk_"

extern void rc_blockallsigs(void);
extern void rc_unblocksig(int sig);
extern void rc_procselfexepath(char * path, size_t len);
extern void rc_procselfexename(char * name, size_t len);
extern void rc_setprocparam(pid_t pid, const char * name, int prio);
extern int rc_pthread_create(pthread_t * thread, const char * name, int prio, void * (*start_routine)(void *), void * arg);
extern int rc_pthread_create_joinable(pthread_t * thread, const char * name, int prio, void * (*start_routine)(void *), void * arg);

#ifdef  __cplusplus
}
#endif

#endif
