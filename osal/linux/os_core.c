/*****************************************************************************
 copyright(c) beijing carsmart technology co., ltd.
 all rights reserved.
 
 @file   : os_core.c
 @brief  : os api 
 @author : wanglei
 @history:
           2015-7-30    wanglei    created file
           ...
******************************************************************************/

#include "os_core.h"
#include "cv_cms_def.h"
int os_mutex_create(pthread_mutex_t *mutex)
{
    int ret = 0;
    pthread_mutexattr_t mutex_attr ;

    if(mutex == NULL)
        return -1;

    ret = pthread_mutexattr_init(&mutex_attr);
    if (0 != ret) {
        (void) fprintf(stderr, "pthread_mutexattr_init(): %s\r\n", strerror(ret));
        return -1;
    }

    /* dnot shared between processes */
    ret = pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_PRIVATE);
    if (0 != ret) {
        (void) fprintf(stderr, "pthread_mutexattr_setpshared(): %s\r\n", strerror(ret));
        goto attr_failed;
    }

    ret = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
    if (0 != ret) {
        (void) fprintf(stderr, "pthread_mutexattr_setptype(): %s\r\n", strerror(ret));
        goto attr_failed;
    }

    ret = pthread_mutex_init(mutex, &mutex_attr);
    if (0 != ret) {
        (void) fprintf(stderr, "pthread_mutex_init(): %s\r\n", strerror(ret));
    }

attr_failed:
    pthread_mutexattr_destroy(&mutex_attr);        
    return ret;
}

int  os_mutex_lock(pthread_mutex_t * mutex)
{
    int ret = 0;
    ret = pthread_mutex_lock(mutex);
    if (0 != ret) {
        (void) fprintf(stderr, "pthread_mutex_lock(): %s\r\n", strerror(ret));
    }
    
    return ret;
}

int os_mutex_trylock(pthread_mutex_t * mutex)
{
    int ret = 0;
    if (mutex == NULL){
        return ret;
    }
    ret = pthread_mutex_trylock(mutex);
    return ret; 
}


int os_mutex_unlock(pthread_mutex_t * mutex)
{
    int ret = 0;
    ret = pthread_mutex_unlock(mutex);
    if (0 != ret) {
        (void) fprintf(stderr, "pthread_mutex_unlock(): %s\r\n", strerror(ret));
    }
    return ret;
}


int os_mutex_destroy(pthread_mutex_t * mutex)
{
    int ret = 0;
    ret = pthread_mutex_destroy(mutex);
    if (0 != ret) {
        (void) fprintf(stderr, "pthread_mutex_destroy(): %s\r\n", strerror(ret));
    }
    return ret;
}

int os_sem_init(sem_t * sem, int pshared, int init_cnt)
{
    int ret = 0;
    ret = sem_init(sem , pshared , init_cnt);
    if (0 != ret) {
        (void) fprintf(stderr, "sem_init(): %s\r\n", strerror(ret));
    }
    return ret;
}

int os_sem_destroy(sem_t *sem)
{
    int ret = 0;
    ret = sem_destroy(sem);
    if (0 != ret) {
        (void) fprintf(stderr, "sem_destroy(): %s\r\n", strerror(ret));
    }
    return ret;
}

int os_sem_wait(sem_t* sem, int timeout)
{
    int ret = 0;
    
    if(sem == NULL)
        return ret;

    if(timeout == 0)
    {
        ret = sem_trywait(sem);
    }
    else if(timeout == -1)
    {
        ret = sem_wait(sem);
    }
    else
    {
        struct timeval current_time;
        struct timespec ts;

        gettimeofday(&current_time, 0);
        ts.tv_sec = current_time.tv_sec;
        ts.tv_sec += timeout/1000;
        if((timeout%1000)*1000 + current_time.tv_usec >= 1000000)
        {
            ts.tv_sec += 1;
            ts.tv_nsec = ((timeout%1000)*1000 + current_time.tv_usec)%1000000;
            ts.tv_nsec *= 1000;
        }
        else
        {
            ts.tv_nsec = (timeout%1000)*1000 + current_time.tv_usec;
            ts.tv_nsec *= 1000;
        }

        while ((ret = sem_timedwait(sem, &ts)) == -1 && errno == EINTR){
            continue;
        }
    }
    
    return ret;
}

int  os_sem_post(sem_t *sem)
{
    int ret = 0;
    if (sem == NULL) {
        return ret;
    }
    ret = sem_post(sem);
    if (0 != ret) {
        (void) fprintf(stderr, "sem_post(): %s\r\n", strerror(ret));
    }
    return ret;
}

int os_spinlock_init(pthread_spinlock_t  *spinlock, int pshared)
{
    return pthread_spin_init(spinlock, pshared);
}

int os_spinlock_lock(pthread_spinlock_t *lock)
{
    return pthread_spin_lock(lock);
}

int os_spinlock_trylock(pthread_spinlock_t *lock)
{
    int ret = -1;
    if (lock){
        ret = pthread_spin_trylock(lock);
    }
    return ret;
}

int os_spinlock_unlock(pthread_spinlock_t *lock)
{
    return pthread_spin_unlock(lock);
}

int os_spinlock_destroy(pthread_spinlock_t *lock)
{
    int ret;
    ret = pthread_spin_destroy(lock);
    return ret;
}

uint64_t os_tick_get(void)
{
    clock_t ticks;
    ticks = times(NULL);
    return (unsigned long long) ticks;
}

int os_get_time(struct os_time *t)
{
	int res;
	struct timeval tv;
	res = gettimeofday(&tv, NULL);
	t->sec = tv.tv_sec;
	t->usec = tv.tv_usec;
	return res;
}

int os_get_reltime(struct os_reltime *t)
{
    int res;
    clockid_t clock_id = CLOCK_MONOTONIC;
	struct timespec ts;
    res = clock_gettime(clock_id, &ts);
    if (res == 0) {
    	t->sec = ts.tv_sec;
    	t->usec = ts.tv_nsec / 1000;
    	return 0;
    }
}

static void usleep_ex(unsigned usecs)
{
    struct timeval t;

    t.tv_sec = usecs/1000000;
    t.tv_usec = usecs%1000000;
    (void)select(1, NULL, NULL, NULL, &t);
}

void os_delay(int usec)
{
    usleep_ex(usec);

    return;
}

void os_sleep(int msec)
{
    msec = (msec <=10)?1:(msec-10);
    usleep_ex(msec*1000);
    return;
}



int os_current_thread(void)
{
    pid_t pid = 0;

    pid = syscall(SYS_gettid);
    return (int)pid;
}

#define MQ_NAME_LEN 32
int os_queue_create
    (
    mqd_t 	  *queue_id,
    const char *queue_name,
    unsigned int 	  queue_depth,
    unsigned int 	  data_size,
    unsigned int 	  flags
    )
{
    struct mq_attr attrs;
    char  mq_name[MQ_NAME_LEN+1] = {0};
    mqd_t id = -1;

    if(queue_name == NULL || queue_id == NULL){
        return -1;
    }
    
    snprintf(mq_name,MQ_NAME_LEN,"/%s",queue_name);
    
    attrs.mq_flags = flags;     
    attrs.mq_maxmsg = queue_depth; 
    attrs.mq_msgsize = data_size; 

    id = mq_open(
            mq_name, 
            O_RDWR | O_CREAT, S_IRWXU | S_IRWXG, 
            &attrs);

    if(id == -1){
        printf("mq_open error %d %s\r\n", errno, strerror(errno));
        return -1;
    }

    *queue_id = (unsigned int)id;
    
    return 0;
}


int os_queue_delete(mqd_t queue_id)
{
    mq_close(queue_id);
    return 0;
}

/* return receive len */
int os_queue_recv(mqd_t queue_id, void *data, uint32_t size, uint32_t *size_copied)
{
    int len = 0 ;

    if (data == NULL || size_copied == NULL){
        return -1;
    }

    len = mq_receive(queue_id, data, size, NULL);
   
    if(len < 0){
        (void) fprintf(stderr, "mq_receive(): %s\r\n", strerror(errno)); 
        return -1;
    }
    
    *size_copied = len;

    return 0;
}


int os_queue_send(mqd_t queue_id, void *data, unsigned int size, uint32_t	priority)
{
    int ret = -1;
    ret = mq_send(queue_id, data, size, priority);  
    if (0 != ret) {
        (void) fprintf(stderr, "mq_send(): %s\r\n", strerror(errno)); 
    }
    return ret;
}

static inline void os_millisec_2_timespec(
        uint32_t timeout,
        struct timespec  * ts)
{
    struct timeval current_time;
    if(ts == NULL){
        return;
    }
#if 1
    gettimeofday(&current_time, 0);
    ts->tv_sec = current_time.tv_sec;
    
    ts->tv_sec += timeout/1000;
    if((timeout%1000)*1000 + current_time.tv_usec >= 1000000)
    {
        ts->tv_sec += 1;
        ts->tv_nsec = ((timeout%1000)*1000 + current_time.tv_usec)%1000000;
        ts->tv_nsec *= 1000;
    }
    else
    {
        ts->tv_nsec = (timeout%1000)*1000 + current_time.tv_usec;
        ts->tv_nsec *= 1000;
    }
#else
    ts->tv_sec = 0;
    ts->tv_nsec = timeout*1000*1000;
#endif
}

int os_mq_timedrecv(mqd_t id, void * buf, uint32_t msg_len, uint32_t timeout)
{
    struct timespec ts;
    int    ret;

    if(buf == NULL){
        return -1;
    }

    os_millisec_2_timespec(timeout,&ts);

    while ((ret = mq_timedreceive(id, buf, msg_len, 0, &ts)) < 0 && errno == EINTR){
        continue;
    }

    if (ret < 0) {
        (void) fprintf(stderr, "mq_timedreceive(): %s\r\n", strerror(errno)); 
        return -1;
    } 
    
    return ret;
}

int os_mq_timedsend(mqd_t qid, void *data, uint32_t msg_len, uint32_t priority, uint32_t timeout)
{
    struct timespec ts;
    int    ret;
    struct mq_attr attr;
    sys_msg_t msg;
    if(data == NULL){
        return -1;
    }
    
    mq_getattr(qid, &attr);
    os_millisec_2_timespec(timeout, &ts);    
#if 0
    if(attr.mq_curmsgs > attr.mq_maxmsg/10){
        

        msg = *(sys_msg_t *)data;

        printf("new msg  is %x\n",msg.id);

        
        printf("msg max is %d,num is %d\n",attr.mq_maxmsg,attr.mq_curmsgs);

        }
    #endif
#if 0
    while ((ret = mq_timedsend(qid, data, msg_len, priority, &ts)) < 0 && errno == EINTR){
        continue;
    }
#else
    ret = mq_timedsend(qid, data, msg_len, priority, &ts);
#endif

    if (ret < 0) {
        (void) fprintf(stderr, "mq_timedsend(): %s\r\n", strerror(errno)); 
        
        return -1;
    } 
    return ret;
}
