/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : os_core.h
 @brief  : linux api 
 @author : wanglei
 @history:
           2015-12-9    wanglei    Created file
           ...
******************************************************************************/
#ifndef _OS_CORE_H_
#define _OS_CORE_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <dlfcn.h>
#include <getopt.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <linux/socket.h>
#include <asm/ioctl.h>
#include <errno.h>
#include <syslog.h>
#include <mqueue.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/uio.h>


#include <netpacket/packet.h>
#include <linux/filter.h>
#include <net/if.h>

/* linux */
#include <endian.h>
#include <byteswap.h>


typedef long long		int64_t;
typedef unsigned long long uint64_t;

typedef pthread_t             osal_task_t;
typedef pthread_mutex_t       osal_mutex_t;
typedef pthread_spinlock_t    osal_spinlock_t;
typedef sem_t                 osal_sem_t;
typedef mqd_t                 osal_mqd_t;
typedef timer_t               osal_timer_t;

#define  os_malloc(len)   malloc(len)
#define  os_free(ptr)     free(ptr)
#define  os_printf        printf

typedef long os_time_t;

struct os_time {
	os_time_t sec;
	os_time_t usec;
};

struct os_reltime {
	os_time_t sec;
	os_time_t usec;
};


int os_mutex_create(pthread_mutex_t *mutex);
int os_mutex_destroy(pthread_mutex_t *mutex);
int os_mutex_lock(pthread_mutex_t *mutex);
int os_mutex_trylock(pthread_mutex_t *mutex);
int os_mutex_unlock(pthread_mutex_t *mutex);

int os_sem_init(sem_t * sem, int pshared, int init_cnt);
int os_sem_destroy(sem_t *sem);
int os_sem_wait(sem_t* sem, int timeout);
int os_sem_post(sem_t* sem);

int os_spinlock_init(pthread_spinlock_t  *spinlock, int pshared);
int os_spinlock_lock(pthread_spinlock_t *lock);
int os_spinlock_unlock(pthread_spinlock_t *lock);
int os_spinlock_destroy(pthread_spinlock_t *lock);

int os_get_reltime(struct os_reltime *t);
uint64_t os_tick_get(void);
void os_delay(int usec);
void os_sleep(int msec);


int os_pthread_create(pthread_t * thread, const char * name, int prio, int stack_size, 
                void * (*start_routine)(void *), void * arg);
int os_current_thread(void);
int os_queue_create(mqd_t *queue_id, const char *queue_name,
                        unsigned int queue_depth, unsigned int data_size, unsigned int flags);
int os_queue_delete(mqd_t queue_id);
int os_queue_recv(mqd_t queue_id, void *data, uint32_t size, uint32_t *size_copied);
int os_queue_send(mqd_t queue_id, void *data, unsigned int size, uint32_t	priority);
int os_mq_timedrecv(mqd_t id, void * buf, uint32_t msg_len, uint32_t timeout);
int os_mq_timedsend(mqd_t qid, void *data, uint32_t msg_len, uint32_t priority, uint32_t timeout);

#endif

