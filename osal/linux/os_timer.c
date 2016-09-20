/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : os_timer.c
 @brief  : timer 
 @author : wanglei
 @history:
           2015-8-25    wanglei    Created file
           ...
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sys/prctl.h>

#include "os_timer.h"

/* Number of threads used.  */
#define THREAD_MAXNODES	16

/* The limit is not published if we are compiled with kernel timer support.
   But we still compiled in this implementation with its limit unless built
   to require the kernel support.  */
#ifndef TIMER_MAX
#define TIMER_MAX 256
#endif

#ifndef DELAYTIMER_MAX
#define DELAYTIMER_MAX	2147483647
#endif

#ifndef __set_errno
#define __set_errno(e) (errno = (e))
#endif

/* Double linked list.  */
struct list_links
{
  struct list_links *next;
  struct list_links *prev;
};

/* Forward declaration.  */
struct timer_node;

/* Definitions for an internal thread of the POSIX timer implementation.  */
struct thread_node
{
  struct list_links links;
  pthread_attr_t attr;
  pthread_t id;
  unsigned int exists;
  struct list_links timer_queue;
  pthread_cond_t cond;
  struct timer_node *current_timer;
  pthread_t captured;
  clockid_t clock_id;
};

/* Internal representation of a timer.  */
struct timer_node
{
  struct list_links links;
  struct sigevent event;
  clockid_t clock;
  int type;
  int millisec;
  struct itimerspec value;
  struct timespec expirytime;
  pthread_attr_t attr;
  unsigned int abstime;
  unsigned int armed;
  enum {
    TIMER_FREE, TIMER_INUSE, TIMER_DELETED
  } inuse;
  struct thread_node *thread;
  pid_t creator_pid;
  int refcount;
  int overrun_count;
};

/* Return pointer to timer structure corresponding to ID.  */
#define timer_id2ptr(timerid) ((struct timer_node *) (void *) timerid)
#define timer_ptr2id(timerid) ((timer_t) (void *) timerid)

/* Array containing the descriptors for the used threads.  */
static struct thread_node thread_array[THREAD_MAXNODES];

/* Static array with the structures for all the timers.  */
struct timer_node __timer_array[TIMER_MAX];

/* Global lock to protect operation on the lists.  */
pthread_mutex_t __timer_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Variable to protext initialization.  */
pthread_once_t __timer_init_once_control = PTHREAD_ONCE_INIT;

/* Nonzero if initialization of timer implementation failed.  */
int __timer_init_failed;

/* Node for the thread used to deliver signals.  */
struct thread_node __timer_signal_thread_rclk;

/* Lists to keep free and used timers and threads.  */
struct list_links timer_free_list;
struct list_links thread_free_list;
struct list_links thread_active_list;

static inline void zero_timespec(struct timespec * ts){
    ts->tv_sec = 0;
    ts->tv_nsec = 0;
}

static inline void zero_itimerspec(struct itimerspec * its)
{
    zero_timespec(&its->it_value);
    zero_timespec(&its->it_interval);
}

static inline void ms2ts(int ms, struct timespec * ts)
{
    ts->tv_sec = ms / 1000;
    ts->tv_nsec = (ms % 1000) * 1000000;
}

/* List handling functions.  */
static inline void
list_init (struct list_links *list)
{
  list->next = list->prev = list;
}

static inline void
list_append (struct list_links *list, struct list_links *newp)
{
  newp->prev = list->prev;
  newp->next = list;
  list->prev->next = newp;
  list->prev = newp;
}

static inline void
list_insbefore (struct list_links *list, struct list_links *newp)
{
  list_append (list, newp);
}

/*
 * Like list_unlink_ip, except that calling it on a node that
 * is already unlinked is disastrous rather than a noop.
 */

static inline void
list_unlink (struct list_links *list)
{
  struct list_links *lnext = list->next, *lprev = list->prev;

  lnext->prev = lprev;
  lprev->next = lnext;
}

static inline struct list_links *
list_first (struct list_links *list)
{
  return list->next;
}

static inline struct list_links *
list_null (struct list_links *list)
{
  return list;
}

static inline struct list_links *
list_next (struct list_links *list)
{
  return list->next;
}

static inline int
list_isempty (struct list_links *list)
{
  return list->next == list;
}

/* Timespec helper routines.  */
static inline int
__attribute ((always_inline))
timespec_compare (const struct timespec *left, const struct timespec *right)
{
  if (left->tv_sec < right->tv_sec)
    return -1;
  if (left->tv_sec > right->tv_sec)
    return 1;

  if (left->tv_nsec < right->tv_nsec)
    return -1;
  if (left->tv_nsec > right->tv_nsec)
    return 1;

  return 0;
}

static inline void
timespec_add (struct timespec *sum, const struct timespec *left,
	      const struct timespec *right)
{
  sum->tv_sec = left->tv_sec + right->tv_sec;
  sum->tv_nsec = left->tv_nsec + right->tv_nsec;

  if (sum->tv_nsec >= 1000000000)
    {
      ++sum->tv_sec;
      sum->tv_nsec -= 1000000000;
    }
}

static inline void
timespec_sub (struct timespec *diff, const struct timespec *left,
	      const struct timespec *right)
{
  diff->tv_sec = left->tv_sec - right->tv_sec;
  diff->tv_nsec = left->tv_nsec - right->tv_nsec;

  if (diff->tv_nsec < 0)
    {
      --diff->tv_sec;
      diff->tv_nsec += 1000000000;
    }
}


/* We need one of the list functions in the other modules.  */
static inline void
list_unlink_ip (struct list_links *list)
{
  struct list_links *lnext = list->next, *lprev = list->prev;

  lnext->prev = lprev;
  lprev->next = lnext;

  /* The suffix ip means idempotent; list_unlink_ip can be called
   * two or more times on the same node.
   */

  list->next = list;
  list->prev = list;
}

/* Functions build on top of the list functions.  */
static inline struct thread_node *
thread_links2ptr (struct list_links *list)
{
  return (struct thread_node *) ((char *) list
				 - offsetof (struct thread_node, links));
}

static inline struct timer_node *
timer_links2ptr (struct list_links *list)
{
  return (struct timer_node *) ((char *) list
				- offsetof (struct timer_node, links));
}


/* Initialize a newly allocated thread structure.  */
static void
thread_init (struct thread_node *thread, const pthread_attr_t *attr, clockid_t clock_id)
{
  if (attr != NULL)
    thread->attr = *attr;
  else
    {
      pthread_attr_init (&thread->attr);
      pthread_attr_setdetachstate (&thread->attr, PTHREAD_CREATE_DETACHED);
    }

  thread->exists = 0;
  list_init (&thread->timer_queue);
  pthread_cond_init (&thread->cond, 0);
  thread->current_timer = 0;
  thread->captured = pthread_self ();
  thread->clock_id = clock_id;
}


/* Initialize the global lists, and acquire global resources.  Error
   reporting is done by storing a non-zero value to the global variable
   timer_init_failed.  */
static void
init_module (void)
{
  int i;

  list_init (&timer_free_list);
  list_init (&thread_free_list);
  list_init (&thread_active_list);

  for (i = 0; i < TIMER_MAX; ++i)
    {
      list_append (&timer_free_list, &__timer_array[i].links);
      __timer_array[i].inuse = TIMER_FREE;
    }

  for (i = 0; i < THREAD_MAXNODES; ++i)
    list_append (&thread_free_list, &thread_array[i].links);

  thread_init (&__timer_signal_thread_rclk, 0, CLOCK_REALTIME);
}


/* This is a handler executed in a child process after a fork()
   occurs.  It reinitializes the module, resetting all of the data
   structures to their initial state.  The mutex is initialized in
   case it was locked in the parent process.  */
static void
reinit_after_fork (void)
{
  init_module ();
  pthread_mutex_init (&__timer_mutex, 0);
}


/* Called once form pthread_once in timer_init. This initializes the
   module and ensures that reinit_after_fork will be executed in any
   child process.  */
static void
__timer_init_once (void)
{
  init_module ();
  pthread_atfork (0, 0, reinit_after_fork);
}


/* Deinitialize a thread that is about to be deallocated.  */
static void
thread_deinit (struct thread_node *thread)
{
  assert (list_isempty (&thread->timer_queue));
  pthread_cond_destroy (&thread->cond);
}


/* Allocate a thread structure from the global free list.  Global
   mutex lock must be held by caller.  The thread is moved to
   the active list. */
static struct thread_node *
__timer_thread_alloc (const pthread_attr_t *desired_attr, clockid_t clock_id)
{
  struct list_links *node = list_first (&thread_free_list);

  if (node != list_null (&thread_free_list))
    {
      struct thread_node *thread = thread_links2ptr (node);
      list_unlink (node);
      thread_init (thread, desired_attr, clock_id);
      list_append (&thread_active_list, node);
      return thread;
    }

  return 0;
}


/* Return a thread structure to the global free list.  Global lock
   must be held by caller.  */
static void
__timer_thread_dealloc (struct thread_node *thread)
{
  thread_deinit (thread);
  list_unlink (&thread->links);
  list_append (&thread_free_list, &thread->links);
}


/* Each of our threads which terminates executes this cleanup
   handler. We never terminate threads ourselves; if a thread gets here
   it means that the evil application has killed it.  If the thread has
   timers, these require servicing and so we must hire a replacement
   thread right away.  We must also unblock another thread that may
   have been waiting for this thread to finish servicing a timer (see
   timer_delete()).  */
static int __timer_thread_start(struct thread_node * thread);
static void
thread_cleanup (void *val)
{
  if (val != NULL)
    {
      struct thread_node *thread = val;

      /* How did the signal thread get killed?  */
      assert (thread != &__timer_signal_thread_rclk);

      pthread_mutex_lock (&__timer_mutex);

      thread->exists = 0;

      /* We are no longer processing a timer event.  */
      thread->current_timer = 0;

      if (list_isempty (&thread->timer_queue))
	__timer_thread_dealloc (thread);
      else
	(void) __timer_thread_start (thread);

      pthread_mutex_unlock (&__timer_mutex);

      /* Unblock potentially blocked timer_delete().  */
      pthread_cond_broadcast (&thread->cond);
    }
}

/* Handle a timer which is supposed to go off now.  */
static void thread_expire_timer(struct thread_node * self, struct timer_node * timer)
{
    self->current_timer = timer; /* Lets timer_delete know timer is running. */

    pthread_mutex_unlock(&__timer_mutex);

    if (SIGEV_THREAD == timer->event.sigev_notify) {
        ((void (*)(void *)) timer->event.sigev_notify_function)((void *) timer->event.sigev_value.sival_ptr);
    }

    /* Notice that the timer may be has been deleted in the timer expire handler,
       so DO NOT dereference the timer pointer anymore. */

    pthread_mutex_lock(&__timer_mutex);

    self->current_timer = 0;

    pthread_cond_broadcast(&self->cond);
}

/* Enqueue a timer in wakeup order in the thread's timer queue.
   Returns 1 if the timer was inserted at the head of the queue,
   causing the queue's next wakeup time to change. */
static int
__timer_thread_queue_timer (struct thread_node *thread,
			    struct timer_node *insert)
{
  struct list_links *iter;
  int athead = 1;

  for (iter = list_first (&thread->timer_queue);
       iter != list_null (&thread->timer_queue);
        iter = list_next (iter))
    {
      struct timer_node *timer = timer_links2ptr (iter);

      if (timespec_compare (&insert->expirytime, &timer->expirytime) < 0)
	  break;
      athead = 0;
    }

  list_insbefore (iter, &insert->links);
  return athead;
}

/* Thread function; executed by each timer thread. The job of this
   function is to wait on the thread's timer queue and expire the
   timers in chronological order as close to their scheduled time as
   possible.  */
static void * thread_func(void * arg)
{
    int policy = 0;
    struct thread_node * self = arg;
    struct list_links * first;
    struct timer_node * timer;
    struct sched_param sched = {.sched_priority = 0};
    struct timespec now, abstime;

    pthread_attr_getschedpolicy(&self->attr, &policy);
    pthread_attr_getschedparam(&self->attr, &sched);
    sched_setscheduler(0, policy, &sched);
    prctl(PR_SET_NAME, (unsigned long) "tk_timer");

    /* Register cleanup handler, in case rogue application terminates
       this thread.  (This cannot happen to __timer_signal_thread, which
       doesn't invoke application callbacks). */

    pthread_cleanup_push(thread_cleanup, self);

    pthread_mutex_lock(&__timer_mutex);

    for (;;) {
        timer = NULL;

        /* While the timer queue is not empty, inspect the first node.  */
        first = list_first(&self->timer_queue);
        if (first != list_null(&self->timer_queue)) {
            timer = timer_links2ptr(first);

            /* This assumes that the elements of the list of one thread
               are all for the same clock.  */
            clock_gettime(timer->clock, &now);

            for (;;) {
                /* If the timer is due or overdue, remove it from the queue.
                   If it's a periodic timer, re-compute its new time and
                   requeue it.  Either way, perform the timer expiry. */
                if (timespec_compare(&now, &timer->expirytime) < 0) {
                    break;
                }

                list_unlink_ip(first);

                if ((0 != timer->value.it_interval.tv_sec) ||
                    (0 != timer->value.it_interval.tv_nsec)) {
                    timer->overrun_count = 0;
                    timespec_add(&timer->expirytime, &timer->expirytime, &timer->value.it_interval);
                    while (timespec_compare(&timer->expirytime, &now) < 0) {
                        timespec_add(&timer->expirytime, &timer->expirytime, &timer->value.it_interval);
                        if (timer->overrun_count < DELAYTIMER_MAX) {
                            ++timer->overrun_count;
                        }
                    }
                    __timer_thread_queue_timer(self, timer);
                }

                thread_expire_timer(self, timer);

                /* DO NOT dereference the timer pointer anymore, because it
                   may be has been deleted in the timer expire handler. */
                timer = NULL;

                first = list_first(&self->timer_queue);
                if (first == list_null(&self->timer_queue)) {
                    break;
                }

                timer = timer_links2ptr(first);
            }
        }

        /* If the queue is not empty, wait until the expiry time of the
           first node.  Otherwise wait indefinitely.  Insertions at the
           head of the queue must wake up the thread by broadcasting
           this condition variable.  */
        if (timer != NULL) {
            if (CLOCK_MONOTONIC == timer->clock) {
                clock_gettime(CLOCK_MONOTONIC, &now);
                clock_gettime(CLOCK_REALTIME, &abstime);
                timespec_sub(&now, &timer->expirytime, &now);
                timespec_add(&abstime, &abstime, &now);
                pthread_cond_timedwait(&self->cond, &__timer_mutex, &abstime);
            } else {
                pthread_cond_timedwait(&self->cond, &__timer_mutex, &timer->expirytime);
            }
        } else {
            pthread_cond_wait(&self->cond, &__timer_mutex);
        }
    }

    pthread_mutex_unlock(&__timer_mutex);
    pthread_cleanup_pop(1);

    return NULL;
}

/* Start a thread and associate it with the given thread node.  Global
   lock must be held by caller.  */
static int
__timer_thread_start (struct thread_node *thread)
{
  int retval = 1;

  assert (!thread->exists);
  thread->exists = 1;

  if (pthread_create (&thread->id, &thread->attr, thread_func, thread) != 0)
    {
      thread->exists = 0;
      retval = -1;
    }

  return retval;
}


static void
__timer_thread_wakeup (struct thread_node *thread)
{
  pthread_cond_broadcast (&thread->cond);
}


/* Compare two pthread_attr_t thread attributes for exact equality.
   Returns 1 if they are equal, otherwise zero if they are not equal
   or contain illegal values.  This version is NPTL-specific for
   performance reason.  One could use the access functions to get the
   values of all the fields of the attribute structure.  */
static int
thread_attr_compare (const pthread_attr_t *left, const pthread_attr_t *right)
{
  int lpolicy = 0, rpolicy = 0;
  struct sched_param lsched = {.sched_priority = 0}, rsched = {.sched_priority = 0};

  (void) pthread_attr_getschedpolicy(left, &lpolicy);
  (void) pthread_attr_getschedpolicy(right, &rpolicy);

  (void) pthread_attr_getschedparam(left, &lsched);
  (void) pthread_attr_getschedparam(right, &rsched);

  if ((lpolicy == rpolicy) &&
      (lsched.sched_priority == rsched.sched_priority)) {
    return 1;
  }

  return 0;
}


/* Search the list of active threads and find one which has matching
   attributes.  Global mutex lock must be held by caller.  */
static struct thread_node *
__timer_thread_find_matching (const pthread_attr_t *desired_attr,
			      clockid_t desired_clock_id)
{
  struct list_links *iter = list_first (&thread_active_list);

  while (iter != list_null (&thread_active_list))
    {
      struct thread_node *candidate = thread_links2ptr (iter);

      if (thread_attr_compare (desired_attr, &candidate->attr)
	  && desired_clock_id == candidate->clock_id)
	return candidate;

      iter = list_next (iter);
    }

  return NULL;
}


/* Grab a free timer structure from the global free list.  The global
   lock must be held by the caller.  */
static struct timer_node *
__timer_alloc (void)
{
  struct list_links *node = list_first (&timer_free_list);

  if (node != list_null (&timer_free_list))
    {
      struct timer_node *timer = timer_links2ptr (node);
      list_unlink_ip (node);
      timer->inuse = TIMER_INUSE;
      timer->refcount = 1;
      return timer;
    }

  return NULL;
}


/* Return a timer structure to the global free list.  The global lock
   must be held by the caller.  */
static void
__timer_dealloc (struct timer_node *timer)
{
  assert (timer->refcount == 0);
  list_unlink_ip (&timer->links);
  timer->thread = NULL;	/* Break association between timer and thread.  */
  timer->inuse = TIMER_FREE;
  list_append (&timer_free_list, &timer->links);
}


/* Thread cancellation handler which unlocks a mutex.  */
static void
__timer_mutex_cancel_handler (void *arg)
{
  pthread_mutex_unlock (arg);
}

/* Check whether timer is valid; global mutex must be held. */
static inline int
timer_valid (struct timer_node *timer)
{
  return timer && timer->inuse == TIMER_INUSE;
}

static inline void
timer_addref (struct timer_node *timer)
{
  timer->refcount++;
}

static inline void
timer_delref (struct timer_node *timer)
{
  if (--timer->refcount == 0)
    __timer_dealloc (timer);
}

/* Create new per-process timer using CLOCK.  */
static int
__timer_create (clockid_t clock_id,struct sigevent *evp,timer_t *timerid)
{
  int retval = -1;
  struct timer_node *newtimer = NULL;
  struct thread_node *thread = NULL;

  if (0
#if defined _POSIX_CPUTIME && _POSIX_CPUTIME >= 0
      || clock_id == CLOCK_PROCESS_CPUTIME_ID
#endif
#if defined _POSIX_THREAD_CPUTIME && _POSIX_THREAD_CPUTIME >= 0
      || clock_id == CLOCK_THREAD_CPUTIME_ID
#endif
      )
    {
      /* We don't allow timers for CPU clocks.  At least not in the
	 moment.  */
      __set_errno (ENOTSUP);
      return -1;
    }

  pthread_once (&__timer_init_once_control, __timer_init_once);

  if (__timer_init_failed)
    {
      __set_errno (ENOMEM);
      return -1;
    }

  pthread_mutex_lock (&__timer_mutex);

  newtimer = __timer_alloc ();
  if (__builtin_expect (newtimer == NULL, 0))
    {
      __set_errno (EAGAIN);
      goto unlock_bail;
    }

  if (evp != NULL)
    newtimer->event = *evp;
  else
    {
      newtimer->event.sigev_notify = SIGEV_SIGNAL;
      newtimer->event.sigev_signo = SIGALRM;
      newtimer->event.sigev_value.sival_ptr = (void *) timer_ptr2id (newtimer);
      newtimer->event.sigev_notify_function = 0;
    }

  newtimer->event.sigev_notify_attributes = &newtimer->attr;
  newtimer->creator_pid = getpid ();

  switch (__builtin_expect (newtimer->event.sigev_notify, SIGEV_SIGNAL))
    {
    case SIGEV_NONE:
    case SIGEV_SIGNAL:
      /* We have a global thread for delivering timed signals.
	 If it is not running, try to start it up.  */
      thread = &__timer_signal_thread_rclk;
      if (! thread->exists)
	{
	  if (__builtin_expect (__timer_thread_start (thread),
				1) < 0)
	    {
	      __set_errno (EAGAIN);
	      goto unlock_bail;
            }
        }
      break;

    case SIGEV_THREAD:
      /* Copy over thread attributes or set up default ones.  */
      if (evp->sigev_notify_attributes)
	newtimer->attr = *(pthread_attr_t *) evp->sigev_notify_attributes;
      else
	pthread_attr_init (&newtimer->attr);

      /* Ensure thread attributes call for deatched thread.  */
      pthread_attr_setdetachstate (&newtimer->attr, PTHREAD_CREATE_DETACHED);

      /* Try to find existing thread having the right attributes.  */
      thread = __timer_thread_find_matching (&newtimer->attr, clock_id);

      /* If no existing thread has these attributes, try to allocate one.  */
      if (thread == NULL)
	thread = __timer_thread_alloc (&newtimer->attr, clock_id);

      /* Out of luck; no threads are available.  */
      if (__builtin_expect (thread == NULL, 0))
	{
	  __set_errno (EAGAIN);
	  goto unlock_bail;
	}

      /* If the thread is not running already, try to start it.  */
      if (!thread->exists && (__timer_thread_start(thread) < 0))
	{
	  __set_errno (EAGAIN);
	  goto unlock_bail;
	}
      break;

    default:
      __set_errno (EINVAL);
      goto unlock_bail;
    }

  newtimer->clock = clock_id;
  newtimer->abstime = 0;
  newtimer->armed = 0;
  newtimer->thread = thread;

  *timerid = timer_ptr2id (newtimer);
  retval = 0;

  if (__builtin_expect (retval, 0) == -1)
    {
    unlock_bail:
      if (thread != NULL)
	__timer_thread_dealloc (thread);
      if (newtimer != NULL)
	{
	  timer_delref (newtimer);
	  __timer_dealloc (newtimer);
	}
    }

  pthread_mutex_unlock (&__timer_mutex);

  return retval;
}

/* Delete timer TIMERID.  */
static int __timer_delete(timer_t timerid)
{
    struct thread_node * thread;
    struct timer_node * timer;
    int retval = -1;

    pthread_mutex_lock(&__timer_mutex);

    timer = timer_id2ptr (timerid);
    if (!timer_valid(timer)) {
        /* Invalid timer ID or the timer is not in use. */
        __set_errno (EINVAL);
    } else {
        if (timer->armed && (NULL != timer->thread)) {
            thread = timer->thread;

            /* If thread is cancelled while waiting for handler to terminate,
               the mutex is unlocked and timer_delete is aborted. */
            pthread_cleanup_push(__timer_mutex_cancel_handler, &__timer_mutex);

            /* If timer is currently being serviced, wait for it to finish. */
            while ((thread->current_timer == timer) && (thread->id != pthread_self())) {
                pthread_cond_wait(&thread->cond, &__timer_mutex);
            }

            pthread_cleanup_pop(0);
        }

        /* Remove timer from whatever queue it may be on and deallocate it. */
        timer->inuse = TIMER_DELETED;
        list_unlink_ip(&timer->links);
        timer_delref(timer);

        retval = 0;
    }

    pthread_mutex_unlock(&__timer_mutex);

    return retval;
}

#if (0)
/* Get expiration overrun for timer TIMERID.  */
static int
__timer_getoverrun (timerid)
     timer_t timerid;
{
  struct timer_node *timer;
  int retval = -1;

  pthread_mutex_lock (&__timer_mutex);

  if (! timer_valid (timer = timer_id2ptr (timerid)))
    __set_errno (EINVAL);
  else
    retval = timer->overrun_count;

  pthread_mutex_unlock (&__timer_mutex);

  return retval;
}

/* Get current value of timer TIMERID and store it in VLAUE.  */
static int
__timer_gettime (timerid, value)
     timer_t timerid;
     struct itimerspec *value;
{
  struct timer_node *timer;
  struct timespec now, expiry;
  int retval = -1, armed = 0, valid;
  clock_t clock = 0;

  pthread_mutex_lock (&__timer_mutex);

  timer = timer_id2ptr (timerid);
  valid = timer_valid (timer);

  if (valid) {
    armed = timer->armed;
    expiry = timer->expirytime;
    clock = timer->clock;
    value->it_interval = timer->value.it_interval;
  }

  pthread_mutex_unlock (&__timer_mutex);

  if (valid)
    {
      if (armed)
	{
	  clock_gettime (clock, &now);
	  if (timespec_compare (&now, &expiry) < 0)
	    timespec_sub (&value->it_value, &expiry, &now);
	  else
	    {
	      value->it_value.tv_sec = 0;
	      value->it_value.tv_nsec = 0;
	    }
	}
      else
	{
	  value->it_value.tv_sec = 0;
	  value->it_value.tv_nsec = 0;
	}

      retval = 0;
    }
  else
    __set_errno (EINVAL);

  return retval;
}
#endif

/* Set timer TIMERID to VALUE, returning old value in OVLAUE.  */
static int
__timer_settime (timer_t timerid, int flags, const struct itimerspec *value, struct itimerspec *ovalue)
{
  struct timer_node *timer;
  struct thread_node *thread = NULL;
  struct timespec now;
  int have_now = 0, need_wakeup = 0;
  int retval = -1;

  timer = timer_id2ptr (timerid);
  if (timer == NULL)
    {
      __set_errno (EINVAL);
      goto bail;
    }

  if (value->it_interval.tv_nsec < 0
      || value->it_interval.tv_nsec >= 1000000000
      || value->it_value.tv_nsec < 0
      || value->it_value.tv_nsec >= 1000000000)
    {
      __set_errno (EINVAL);
      goto bail;
    }

  /* Will need to know current time since this is a relative timer;
     might as well make the system call outside of the lock now! */

  if ((flags & TIMER_ABSTIME) == 0)
    {
      clock_gettime (timer->clock, &now);
      have_now = 1;
    }

  pthread_mutex_lock (&__timer_mutex);
  timer_addref (timer);

  /* One final check of timer validity; this one is possible only
     until we have the mutex, because it accesses the inuse flag. */

  if (! timer_valid(timer))
    {
      __set_errno (EINVAL);
      goto unlock_bail;
    }

  if (ovalue != NULL)
    {
      ovalue->it_interval = timer->value.it_interval;

      if (timer->armed)
	{
	  if (! have_now)
	    {
	      pthread_mutex_unlock (&__timer_mutex);
	      clock_gettime (timer->clock, &now);
	      have_now = 1;
	      pthread_mutex_lock (&__timer_mutex);
	      timer_addref (timer);
	    }

	  timespec_sub (&ovalue->it_value, &timer->expirytime, &now);
	}
      else
	{
	  ovalue->it_value.tv_sec = 0;
	  ovalue->it_value.tv_nsec = 0;
	}
    }

  timer->value = *value;

  list_unlink_ip (&timer->links);
  timer->armed = 0;

  thread = timer->thread;

  /* A value of { 0, 0 } causes the timer to be stopped. */
  if ((value->it_value.tv_sec != 0) ||
      __builtin_expect (value->it_value.tv_nsec != 0, 1))
    {
      if ((flags & TIMER_ABSTIME) != 0)
	/* The user specified the expiration time.  */
	timer->expirytime = value->it_value;
      else
	timespec_add (&timer->expirytime, &now, &value->it_value);

      /* Only need to wake up the thread if timer is inserted
	 at the head of the queue. */
      if (thread != NULL)
	need_wakeup = __timer_thread_queue_timer (thread, timer);
      timer->armed = 1;
    }

  retval = 0;

unlock_bail:
  timer_delref (timer);
  pthread_mutex_unlock (&__timer_mutex);

bail:
  if (thread != NULL && need_wakeup)
    __timer_thread_wakeup (thread);

  return retval;
}

/**
 * 创建定时器
 *
 * @prio     : 定时器线程的优先级，尽量使用rc_timer.h中所定义的优先级。
 *             不同优先级的定时任务由不同优先级的线程负责维护，相同优先级的定
 *             时任务由相同的线程负责维护，因此尽量合并定时器的优先级以减少线
 *             程数，目前每个进程内最多支持同时创建16个不同优先级的定时器线程，
 *             每个进程最多可创建256个定时器。
 * @millisec : 定时器超时时间，单位毫秒(ms)，指定多长时间后调用handler指定的
 *             超时处理函数，该参数必须大于0。
 * @flag     : 定时器创建标志位:
 *             TIMER_ONESHOT  : 指定创建的定时器为单次定时器
 *             TIMER_INTERVAL : 指定创建的定时器为周期定时器，缺省定时器类型，
 *                              可不指定该标志位
 *             TIMER_STOPPED  : 指定创建的定时器处于停止状态，
 *                              需要后续调用rc_timer_start后才开始计时
 * @handler  : 定时器超时处理函数。
 * @arg      : 定时器超时处理函数参数。
 * @timerid  : 用来保存新创建的定时器ID。
 *
 * 示例
 * 1. 创建1秒周期定时器:
 *    timer_t timerid;
 *    rc_timer_add(TIMER_PRIO_NORMAL, 1000, 0, handler, NULL, &timerid);
 * 2. 创建1秒缺省暂停周期定时器:
 *    timer_t timerid;
 *    rc_timer_add(TIMER_PRIO_NORMAL, 1000, TIMER_STOPPED, handler, NULL, &timerid);
 * 3. 创建1秒单次定时器:
 *    timer_t timerid;
 *    rc_timer_add(TIMER_PRIO_NORMAL, 1000, TIMER_ONESHOT, handler, NULL, &timerid);
 * 4. 创建1秒缺省暂停单次定时器:
 *    timer_t timerid;
 *    rc_timer_add(TIMER_PRIO_NORMAL, 1000, TIMER_ONESHOT | TIMER_STOPPED, handler, NULL, &timerid);
 *
 * 备注
 * 1. 单次定时器并不会在超时后自动删除，必须显式调用rc_timer_del删除，
 *    以防止定时器资源泄漏，删除时机一般为在超时处理函数中或在定时器超时前；
 * 2. 定时器超时处理函数中可以删除定时器本身；
 * 3. 定时器创建失败时timerid所指向的内存内容不会被修改；
 * 4. 定时器ID变量仅应由rc_timer_add赋值或在用rc_timer_del删除定时器后显式赋值
 *    为0；
 * 5. 调用rc_timer_del删除定时器后一定要显式的将保存定时器ID的变量置为0，以避免
 *    定时器的重复删除；
 * 6. 在创建新的定时器前最好判断所使用的timerid是否已经创建过，以避免之前的定时
 *    器资源泄漏；
 */
int os_timer_add(int prio, int millisec, int flag, void (*handler)(void *), void * arg, timer_t * timerid)
{
    timer_t tid = 0;
    struct timer_node * timer;
    struct sigevent sev;
    struct itimerspec its;
    pthread_attr_t attr;
    struct sched_param sched;

    if ((millisec <= 0) || (NULL == handler) || (NULL == timerid)) {
        return -1;
    }

    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    sched.sched_priority = prio;
    pthread_attr_setschedparam(&attr, &sched);

    (void) memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = (void (*)(sigval_t)) handler;
    sev.sigev_notify_attributes = &attr;
    sev.sigev_value.sival_ptr = arg;
    if (0 != __timer_create(CLOCK_MONOTONIC, &sev, &tid)) {
        return -1;
    }

    pthread_mutex_lock(&__timer_mutex);
    timer = timer_id2ptr(tid);
    if (!timer_valid(timer)) {
        pthread_mutex_unlock(&__timer_mutex);
        return -1;
    }
    timer->type = (flag & TIMER_ONESHOT) ? TIMER_ONESHOT : TIMER_INTERVAL;
    timer->millisec = millisec;
    pthread_mutex_unlock(&__timer_mutex);

    if (flag & TIMER_STOPPED) {
        its.it_value.tv_sec = 0;
        its.it_value.tv_nsec = 0;
    } else {
        ms2ts(millisec, &its.it_value);
    }

    if (flag & TIMER_ONESHOT) {
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
    } else {
        its.it_interval.tv_sec = its.it_value.tv_sec;
        its.it_interval.tv_nsec = its.it_value.tv_nsec;
    }

    if (0 != __timer_settime(tid, flag, &its, NULL)) {
        return -1;
    }

    *timerid = tid;

    return 0;
}

/**
 * 删除定时器
 *
 * @timerid : 要删除的定时器ID。
 *
 * 备注
 * 1. 删除前可以不判断定时器ID是否为0；
 * 2. 调用rc_timer_del删除定时器后一定要显式的将保存定时器ID的变量置为0，以避免
 *    定时器的重复删除；
 */
int os_timer_del(timer_t timerid)
{
    return __timer_delete(timerid);
}

/**
 * 启动定时器
 *
 * @timerid : 要操作的定时器ID。
 *
 * 备注
 * 1. 如果对于一个已经启动的定时器调用rc_timer_start，则定时器将被重置，即重新
      计时，功能等同于rc_timer_reset；
 * 2. 单次定时器超时后可以调用该函数使定时器重新计时；
 */
int os_timer_start(timer_t timerid)
{
    struct timer_node * timer;
    struct itimerspec its;

    pthread_mutex_lock(&__timer_mutex);

    timer = timer_id2ptr(timerid);
    if (!timer_valid(timer)) {
        pthread_mutex_unlock(&__timer_mutex);
        return -1;
    }

    ms2ts(timer->millisec, &its.it_value);
    if (TIMER_ONESHOT == timer->type) {
        zero_timespec(&its.it_interval);
    } else {
        its.it_interval.tv_sec = its.it_value.tv_sec;
        its.it_interval.tv_nsec = its.it_value.tv_nsec;
    }

    pthread_mutex_unlock(&__timer_mutex);

    return __timer_settime(timerid, 0, &its, NULL);
}

/**
 * 停止定时器
 *
 * @timerid : 要操作的定时器ID。
 *
 * 备注
 * 1. 可对同一定时器连续多次调用停止操作；
 */
int os_timer_stop(timer_t timerid)
{
    struct itimerspec its;

    zero_itimerspec(&its);

    return __timer_settime(timerid, 0, &its, NULL);
}

/**
 * 重置定时器
 *
 * @timerid : 要操作的定时器ID。
 *
 * 备注
 * 1. 功能等同于rc_timer_start；
 */
int os_timer_reset(timer_t timerid)
{
    return os_timer_start(timerid);
}

/**
 * 修改定时器超时时间
 *
 * @timerid : 要操作的定时器ID。
 *
 * 备注
 * 1. 修改定时器超时时间后直到调用rc_timer_start后新的超时时间才会生效。
      虽然非必要，但为了语义更清晰，一般在修改定时器时间之前调用os_timer_stop
      先停止定时器，如:
      int ret = 0;
      ret |= os_timer_stop(timerid);
      ret |= os_timer_settime(timerid, 2000);
      ret |= os_timer_start(timerid);
 */
int os_timer_settime(timer_t timerid, int millisec)
{
    struct timer_node * timer;

    if (millisec <= 0) {
        return -1;
    }

    pthread_mutex_lock(&__timer_mutex);

    timer = timer_id2ptr(timerid);
    if (!timer_valid(timer)) {
        pthread_mutex_unlock(&__timer_mutex);
        return -1;
    }
    timer->millisec = millisec;

    pthread_mutex_unlock(&__timer_mutex);

    return 0;
}
