/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : os_pthread.c
 @brief  : pthread os adapt layer
 @author : wanglei
 @history:
           2015-8-25    wanglei    Created file
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
#include <limits.h>
#include "error.h"
#include "os_core.h"
#include "os_pthread.h"
/*************************************************
  �궨��
*************************************************/
#define TASK_COMM_LEN 				16
#define CFG_THREAD_MAX				32			//���֧���߳���
/*************************************************
  ��̬ȫ�ֱ�������
*************************************************/

typedef struct {
    char * name;
    int    prio;
    void * (*start_routine)(void *);
    void * arg;
} thread_info_t;

void os_blockallsigs(void)
{
    sigset_t set;
    (void) sigfillset(&set);
    (void) sigprocmask(SIG_BLOCK, &set, NULL);
}

void os_unblocksig(int sig)
{
    sigset_t set;
    (void) sigemptyset(&set);
    (void) sigaddset(&set, sig);
    (void) pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}

void os_procselfexepath(char * path, size_t len)
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

void os_procselfexename(char * name, size_t len)
{
    char path[PATH_MAX];

    if ((NULL == name) || (0 == len)) {
        return;
    }

    os_procselfexepath(path, sizeof(path));
    (void) strncpy(name, basename(path), len - 1);
    name[len - 1] = '\0';
}

void os_setprocparam(const char * name)
{
    const char * __name;
    char self_name[TASK_COMM_LEN], self_name_prefixed[TASK_COMM_LEN];
//    struct sched_param sched;

    if ((NULL != name) && ('\0' != name[0])) {
        __name = name;
    } else {
        os_procselfexename(self_name, sizeof(self_name));
        (void) snprintf(self_name_prefixed, sizeof(self_name_prefixed), "%s%s", TK_NAME_PREFIX, self_name);
        __name = self_name_prefixed;
    }
    if (prctl(PR_SET_NAME, (unsigned long) __name) < 0) {
        (void) fprintf(stderr, "prctl(): %s, name = %s\r\n", strerror(errno), __name);
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
    (void) os_setprocparam(info.name);

    (void) pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    (void) pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    free_thread_info((thread_info_t *) arg);

    return info.start_routine(info.arg);
}

static int pthread_create_internel(pthread_t * thread, const char * name, int prio, int state, int stack_size,
              void * (*start_routine)(void *), void * arg)
{
    int ret;
    pthread_t tid, * ptid = thread;
    thread_info_t * info;
    struct sched_param sched;
    pthread_attr_t  attr;
//    unsigned int local_stack_size = 0;

    ret = pthread_attr_init(&attr);
    if (0 != ret) {
        (void) fprintf(stderr, "pthread_attr_init(): %s\r\n", strerror(ret));
        return -1;
    }

//    if (stack_size != 0) {
//        if (stack_size < PTHREAD_STACK_MIN)
//            local_stack_size = PTHREAD_STACK_MIN;
//        else
//            local_stack_size = stack_size;
//
//        /* PTHREAD_STACK_MIN == 16384, 16K, default 8M */
//        ret = pthread_attr_setstacksize(&attr, (size_t)local_stack_size);
//        if (0 != ret) {
//            (void) fprintf(stderr, "pthread_attr_setstacksize(): %s\r\n", strerror(ret));
//            (void) pthread_attr_destroy(&attr);
//            return -1;
//        }
//    }

    /* Defaut: PTHREAD_CREATE_JOINABLE */
    ret = pthread_attr_setdetachstate(&attr, state);
    if (0 != ret) {
        (void) fprintf(stderr, "pthread_attr_setdetachstate(): %s\r\n", strerror(ret));
        (void) pthread_attr_destroy(&attr);
        return -1;
    }

    /* Defaut: PTHREAD_INHERIT_SCHED, SCHED_OTHER, prio=0 */
    /* PTHREAD_EXPLICIT_SCHED:ʹ�ýṹpthread_attr_tָ���ĵ����㷨 */
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (0 != ret) {
        (void) fprintf(stderr, "pthread_attr_setinheritsched(): %s\r\n", strerror(ret));
        (void) pthread_attr_destroy(&attr);
        return -1;
    }

    ret = pthread_attr_setschedpolicy(&attr, SCHED_RR);
    if (0 != ret) {
        (void) fprintf(stderr, "pthread_attr_setschedpolicy(): %s\r\n", strerror(ret));
        (void) pthread_attr_destroy(&attr);
        return -1;
    }  

    /* sched_priority(1-99): sched_get_priority_min(SCHED_RR)-sched_get_priority_max(SCHED_RR) */  
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

/******************************************************************************
*	����:	thread_create
*	����:	�����߳�
*	����:	id				-	�̺߳�
 			function		-	�̺߳���
 			arg				-	�̺߳�������
 			mode			-	�߳�ģʽ����ͨ��ʵʱ��
 			prio			-	���ȼ���1-99��
*	����:	0				-	�ɹ�
 			-ERR_NOINIT		-	ģ��δ��ʼ��
 			-ERR_INVAL		-	��������ȷ
 			-ERR_BUSY		-	�߳��Ѵ���
 			-ERR_NOMEM		-	�ڴ治��
*	˵��:	ֻ��ʵʱģʽ֧�����ȼ���99Ϊ������ȼ�����ͨģʽ���ȼ��̶�Ϊ0��
 ******************************************************************************/
int thread_create(uint8_t id, void *(*function)(void *), void *arg, uint8_t mode, uint8_t prio)
{
    int ret = 0;
    pthread_t tid;

    if (id > CFG_THREAD_MAX - 1) {
        ret = -ERR_INVAL;
        goto error;
    }

    ret = thread_create_base (&tid, function, arg, mode, prio);
    if (ret) {
    	goto error;
    }

error:
    return ret;
}

/******************************************************************************
*	����:	thread_cancel
*	����:	��ֹ�߳�
*	����:	id				-	�̺߳�
*	����:	0				-	�ɹ�
 			-ERR_NODEV		-	�޴��߳�
 			-ERR_INVAL		-	��������
*	˵��:
 ******************************************************************************/
int thread_cancel(pthread_t tid)
{
    int ret = 0;

    ret = pthread_cancel (tid);
    if (ret) {
        ret = -ERR_NODEV;
        goto error;
    }

error:
    return ret;
}

/******************************************************************************
*	����:	thread_setpriority
*	����:	�����߳����ȼ�
*	����:	tid				-	�̺߳�
 			prio			-	���ȼ�
*	����:	0				-	�ɹ�
 			-ERR_NODEV		-	�޴��߳�
 			-ERR_INVAL		-	��������
*	˵��:	���ȼ�Ϊ1-99��99Ϊ������ȼ���
 ******************************************************************************/
int thread_setpriority(pthread_t tid, uint8_t prio)
{
    int ret = 0;

//�������ȼ�
    ret = thread_setpriority_base(tid, prio);
    if (ret) {
    	goto error;
    }

error:
    return ret;
}

/******************************************************************************
*	����:	thread_create_base
*	����:	�����߳�
*	����:	tid				-	ϵͳ�̺߳�
 			function		-	�̺߳���
 			arg				-	�̺߳�������
 			mode			-	�߳�ģʽ����ͨ��ʵʱ��
 			prio			-	���ȼ���1-99��
*	����:	0				-	�ɹ�
 			-ERR_INVAL		-	��������ȷ
 			-ERR_NOMEM		-	�ڴ治��
*	˵��:	ֻ��ʵʱģʽ֧�����ȼ���99Ϊ������ȼ�����ͨģʽ���ȼ��̶�Ϊ0��
 ******************************************************************************/
int thread_create_base(pthread_t *tid, void *(*function)(void *), void *arg, uint8_t mode, uint8_t prio)
{
    int ret = 0;
    pthread_attr_t attr;
    struct sched_param param;

//��ʼ���߳�����
    ret = pthread_attr_init(&attr);
    if (ret) {
        ret = -ERR_NOMEM;
        goto error;
    }
//�����߳�Ϊ����״̬
    ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (ret) {
        ret = -ERR_INVAL;
        goto error;
    }
//�������ȼ�
    if (THREAD_MODE_NORMAL == mode) {
        if (0 != prio) {
            ret = -ERR_INVAL;
            goto error;
        }
    }
    else if (THREAD_MODE_REALTIME == mode) {
        if ((prio < 1) || (prio > 99)) {			//1��ʵʱ�߳�������ȼ���99��������ȼ�
            ret = -ERR_INVAL;
            goto error;
        }
        ret = pthread_attr_setschedpolicy(&attr, SCHED_RR);
        if (ret) {
            ret = -ERR_INVAL;
            goto error;
        }
        param.sched_priority = prio;
        ret = pthread_attr_setschedparam(&attr, &param);
        if (ret) {
            ret = -ERR_INVAL;
            goto error;
        }
    }
    else {
        ret = -ERR_INVAL;
        goto error;
    }
//�����߳�
    ret = pthread_create (tid, &attr, function, arg);
    if (ret) {
        ret = -ERR_NOMEM;
        goto error;
    }
error:
    return ret;
}

/******************************************************************************
*	����:	thread_setpriority_base
*	����:	�����߳����ȼ�
*	����:	tid				-	ϵͳ�̺߳�
 			prio			-	���ȼ�
*	����:	0				-	�ɹ�
 			-ERR_INVAL		-	��������
*	˵��:	���ȼ�Ϊ1-99��99Ϊ������ȼ���
 ******************************************************************************/
int thread_setpriority_base(pthread_t tid, uint8_t prio)
{
    int ret = 0;
    struct sched_param param;

    //������Ч���ж�
	if ((prio < 1) || (prio > 99)) {			//1��ʵʱ�߳�������ȼ���99��������ȼ�
		ret = -ERR_INVAL;
		goto error;
	}
	//�������ȼ�
	param.sched_priority = prio;
	ret = pthread_setschedparam(tid, SCHED_RR, &param);
	if(ret){
		ret = -ERR_INVAL;
		goto error;
	}
error:
    return ret;
}

/******************************************************************************
*	����:	thread_gettid_base
*	����:	�õ���ǰϵͳ�̺߳�
*	����:	void
*	����:	*				-	ϵͳ�̺߳�
*	˵��:
 ******************************************************************************/
int thread_gettid_base(void)
{
    return pthread_self();
}


int os_pthread_create(pthread_t * thread, const char * name, int prio, int stack_size, 
             void * (*start_routine)(void *), void * arg)
{
//    return pthread_create_internel(thread, name, prio, PTHREAD_CREATE_DETACHED, stack_size, start_routine, arg);
	if(prio == 0)
		return thread_create_base(thread, start_routine, arg, THREAD_MODE_NORMAL, prio);
	else
		return thread_create_base(thread, start_routine, arg, THREAD_MODE_REALTIME, prio);
}

int os_pthread_create_joinable(pthread_t * thread, const char * name, int prio, int stack_size, 
            void * (*start_routine)(void *), void * arg)
{
    return pthread_create_internel(thread, name, prio, PTHREAD_CREATE_JOINABLE, stack_size, start_routine, arg);
}

