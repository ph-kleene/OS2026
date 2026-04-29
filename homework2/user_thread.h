#ifndef USER_THREAD_H
#define USER_THREAD_H

#define THREAD_STACK_SIZE   (128 * 1024)  /* stack per thread */
#define SCHED_STACK_SIZE    (128 * 1024)  /* scheduler's own stack */
#define MAX_THREADS         128           /* max concurrent threads */
#define TIME_SLICE_US       20000         /* 20 ms time slice */

/*
 * User-level thread API.
 *
 * -- thread_create   spawn a new thread running func(arg), return tid
 * -- thread_join     block until thread tid finishes
 * -- thread_yield    voluntarily give up the CPU
 * -- thread_destroy  free resources of a FINISHED thread
 * -- thread_self     return caller's tid
 */

int  thread_create(void (*func)(void *), void *arg);
int  thread_join(int tid);
void thread_yield(void);
void thread_destroy(int tid);
int  thread_self(void);

/* One-time initialisation (sets up scheduler, timer, signal handler).
 * Must be called before any other thread_* function. */
void thread_scheduler_init(void);

#endif
