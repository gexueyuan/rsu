/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : rc_pthread.c
 @brief  : thread api
 @author : wanglei
 @history:
           2015-12-1    wanglei    Created file
           ...
******************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <signal.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <linux/limits.h>

#include "rc_pthread.h"

#define TASK_COMM_LEN 16

typedef struct {
    char * name;
    int    prio;
    void * (*start_routine)(void *);
    void * arg;
} thread_info_t;

void rc_blockallsigs(void)
{
    sigset_t set;
    (void) sigfillset(&set);
    (void) sigprocmask(SIG_BLOCK, &set, NULL);
}

void rc_unblocksig(int sig)
{
    sigset_t set;
    (void) sigemptyset(&set);
    (void) sigaddset(&set, sig);
    (void) pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}

void rc_procselfexepath(char * path, size_t len)
{
    if ((NULL == path) || (0 == len)) {
        return;
    }

    bzero(path, len);
    if (readlink("/proc/self/exe", path, len) < 0) {
        (void) strncpy(path, "unknown", len - 1);
    }
    path[len - 1] = '\0';
}

void rc_procselfexename(char * name, size_t len)
{
    char path[PATH_MAX];

    if ((NULL == name) || (0 == len)) {
        return;
    }

    rc_procselfexepath(path, sizeof(path));
    (void) strncpy(name, basename(path), len - 1);
    name[len - 1] = '\0';
}

void rc_setprocparam(pid_t pid, const char * name, int prio)
{
    const char * __name;
    char self_name[TASK_COMM_LEN], self_name_prefixed[TASK_COMM_LEN];
    struct sched_param sched;

    if ((NULL != name) && ('\0' != name[0])) {
        __name = name;
    } else {
        rc_procselfexename(self_name, sizeof(self_name));
        (void) snprintf(self_name_prefixed, sizeof(self_name_prefixed), "%s%s", TK_NAME_PREFIX, self_name);
        __name = self_name_prefixed;
    }
    if (prctl(PR_SET_NAME, (unsigned long) __name) < 0) {
        (void) fprintf(stderr, "prctl(): %s, name = %s\r\n", strerror(errno), __name);
    }

    sched.sched_priority = prio;
    if (sched_setscheduler(pid, SCHED_RR, &sched) < 0) {
        (void) fprintf(stderr, "sched_setscheduler(): %s, pid = %d, prio = %d\r\n", strerror(errno), pid, prio);
    }
}

static void free_thread_info(thread_info_t * info)
{
    if (NULL != info->name) {
        free(info->name);
        info->name = NULL;
    }
    free(info);
}

static void * start_routine_wrapper(void * arg)
{
    thread_info_t info;

    (void) memcpy(&info, arg, sizeof(info));
    (void) rc_setprocparam(0, info.name, info.prio);

    (void) pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    (void) pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    free_thread_info((thread_info_t *) arg);

    return info.start_routine(info.arg);
}

static int pthread_create_internel(pthread_t * thread, const char * name, int prio, int state, void * (*start_routine)(void *), void * arg)
{
    int ret;
    pthread_t tid, * ptid = thread;
    thread_info_t * info;
    struct sched_param sched;
    pthread_attr_t attr;

    ret = pthread_attr_init(&attr);
    if (0 != ret) {
        (void) fprintf(stderr, "pthread_attr_init(): %s\r\n", strerror(ret));
        return -1;
    }

    ret = pthread_attr_setschedpolicy(&attr, SCHED_RR);
    if (0 != ret) {
        (void) fprintf(stderr, "pthread_attr_setschedpolicy(): %s\r\n", strerror(ret));
        (void) pthread_attr_destroy(&attr);
        return -1;
    }

    ret = pthread_attr_setdetachstate(&attr, state);
    if (0 != ret) {
        (void) fprintf(stderr, "pthread_attr_setdetachstate(): %s\r\n", strerror(ret));
        (void) pthread_attr_destroy(&attr);
        return -1;
    }

    sched.sched_priority = prio;
    ret = pthread_attr_setschedparam(&attr, &sched);
    if (0 != ret) {
        (void) fprintf(stderr, "pthread_attr_setschedparam(): %s\r\n", strerror(ret));
        (void) pthread_attr_destroy(&attr);
        return -1;
    }

    info = malloc(sizeof(*info));
    if (NULL == info) {
        (void) fprintf(stderr, "malloc(): No enough memory\r\n");
        (void) pthread_attr_destroy(&attr);
        return -1;
    }
    info->name = NULL;
    if (NULL != name) {
        info->name = strdup(name);
    }
    info->prio = prio;
    info->start_routine = start_routine;
    info->arg = arg;

    if (NULL == ptid) {
        ptid = &tid;
    }
    ret = pthread_create(ptid, &attr, start_routine_wrapper, info);
    if (0 != ret) {
        (void) fprintf(stderr, "pthread_create(): %s\r\n", strerror(ret));
        (void) pthread_attr_destroy(&attr);
        free_thread_info(info);
        return -1;
    }

    return 0;
}

int rc_pthread_create(pthread_t * thread, const char * name, int prio, void * (*start_routine)(void *), void * arg)
{
    return pthread_create_internel(thread, name, prio, PTHREAD_CREATE_DETACHED, start_routine, arg);
}

int rc_pthread_create_joinable(pthread_t * thread, const char * name, int prio, void * (*start_routine)(void *), void * arg)
{
    return pthread_create_internel(thread, name, prio, PTHREAD_CREATE_JOINABLE, start_routine, arg);
}
