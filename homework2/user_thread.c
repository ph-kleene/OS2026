#define _GNU_SOURCE
#include "user_thread.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

/* ----- thread states ----- */
typedef enum {
    TS_READY,
    TS_RUNNING,
    TS_BLOCKED,   /* waiting in thread_join */
    TS_FINISHED
} thread_state_t;

/* ----- thread control block ----- */
typedef struct thread {
    int             tid;
    thread_state_t  state;
    ucontext_t      ctx;
    void           *stack;
    void          (*func)(void *);
    void           *arg;
    struct thread  *joiner;   /* thread that called join on me  */
    struct thread  *next;     /* ready-queue link                */
} thread_t;

/* ----- global scheduler state ----- */
static thread_t  *current        = NULL;
static thread_t  *ready_head     = NULL;
static thread_t  *ready_tail     = NULL;
static thread_t  *main_th        = NULL;
static ucontext_t sched_ctx;
static char       sched_stack[SCHED_STACK_SIZE];
static int        next_tid       = 1;
static int        init_done      = 0;
static thread_t  *all_threads[MAX_THREADS];
static int        num_threads    = 0;

/* ----- helpers ----- */
static void enqueue(thread_t *t)
{
    t->next = NULL;
    if (!ready_head) {
        ready_head = ready_tail = t;
    } else {
        ready_tail->next = t;
        ready_tail = t;
    }
}

static thread_t *dequeue(void)
{
    if (!ready_head) return NULL;
    thread_t *t = ready_head;
    ready_head = t->next;
    if (!ready_head) ready_tail = NULL;
    t->next = NULL;
    return t;
}

static thread_t *find_thread(int tid)
{
    for (int i = 0; i < num_threads; i++)
        if (all_threads[i]->tid == tid)
            return all_threads[i];
    return NULL;
}

/* ----- forward decls ----- */
static void thread_wrapper(thread_t *t);
static void sched_loop(void);

/* ----- SIGALRM handler -----
 *
 * Uses SA_SIGINFO so the third argument (uap) supplies the ucontext_t of the
 * *interrupted* code.  We copy that into the current thread's ctx, mark the
 * thread ready, and transfer control to the scheduler with setcontext().
 *
 * The handler never returns — setcontext() does not save the handler's own
 * context, which is fine because the preempted state is already preserved.
 */
static void sigalrm_handler(int sig, siginfo_t *info, void *uap)
{
    (void)sig;   /* unused */
    (void)info;  /* unused */

    /* Re-arm the interval timer in case the system doesn't auto-repeat */
    ualarm(TIME_SLICE_US, TIME_SLICE_US);

    if (!current || current->state != TS_RUNNING)
        return; /* nothing to preempt */

    /* Save the kernel-provided snapshot of the interrupted context */
    current->ctx = *(ucontext_t *)uap;
    current->state = TS_READY;
    enqueue(current);

    /* Jump to scheduler (does not save handler's own context) */
    setcontext(&sched_ctx);
}

/* ----- scheduler loop -----
 *
 * Round-robin: dequeue next ready thread, mark it RUNNING, swap to it.
 * When swapcontext() returns here, the thread yielded / was preempted /
 * finished, so we inspect its new state and loop.
 */
static void sched_loop(void)
{
    while (1) {
        thread_t *next = dequeue();
        if (!next) {
            /* No ready threads — return to main thread if it exists */
            if (main_th && main_th->state == TS_BLOCKED) {
                /* Main is waiting for workers; keep spinning until one
                 * finishes and wakes main.  In a production library we'd
                 * block on a signal here, but busy-wait is simple. */
                /* Nothing ready — brief sleep so signals can arrive */
                struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000 };
                nanosleep(&ts, NULL);
                continue;
            }
            /* Nothing left to run — exit scheduler and go back to whoever
             * swapped us in (should be main exiting the program). */
            break;
        }

        current = next;
        current->state = TS_RUNNING;

        /* Ensure SIGALRM is unblocked in the target context, in case it
         * was saved while the signal was masked. */
        sigdelset(&current->ctx.uc_sigmask, SIGALRM);

        swapcontext(&sched_ctx, &current->ctx);

        /* ----- execution resumes here after the thread yields / is preempted ----- */

        if (current->state == TS_FINISHED) {
            /* thread_wrapper already woke the joiner; just clean up reference */
            current = NULL;
        }
        /* If still READY or BLOCKED it is already in the right
         * queue (done by the preemption handler / thread_yield). */
    }
}

/* ----- thread wrapper -----
 *
 * Runs on the new thread's own stack.  Calls the user function, then marks
 * the thread FINISHED, wakes the joiner, and swaps to the scheduler forever.
 */
static void thread_wrapper(thread_t *t)
{
    t->func(t->arg);

    t->state = TS_FINISHED;

    /* Wake up the thread that is joining us */
    if (t->joiner) {
        t->joiner->state = TS_READY;
        enqueue(t->joiner);
    }

    /* Hand control back to the scheduler — never returns */
    swapcontext(&t->ctx, &sched_ctx);
}

/* ----- public API --------------------------------------------------- */

void thread_scheduler_init(void)
{
    if (init_done) return;

    /* Wrap the main thread */
    main_th = calloc(1, sizeof(thread_t));
    if (!main_th) { perror("calloc"); exit(EXIT_FAILURE); }
    main_th->tid   = 0;
    main_th->state = TS_RUNNING;
    main_th->stack = NULL; /* uses the process's own stack */
    current = main_th;

    all_threads[num_threads++] = main_th;

    /* Build the scheduler context on its own stack */
    if (getcontext(&sched_ctx) == -1) { perror("getcontext"); exit(EXIT_FAILURE); }
    sched_ctx.uc_stack.ss_sp    = sched_stack;
    sched_ctx.uc_stack.ss_size  = sizeof(sched_stack);
    sched_ctx.uc_link           = NULL;
    makecontext(&sched_ctx, sched_loop, 0);

    /* Install SIGALRM handler with SA_SIGINFO for the third argument */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = sigalrm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    if (sigaction(SIGALRM, &sa, NULL) == -1) { perror("sigaction"); exit(EXIT_FAILURE); }

    /* Start 20-ms periodic timer */
    ualarm(TIME_SLICE_US, TIME_SLICE_US);

    init_done = 1;
}

int thread_create(void (*func)(void *), void *arg)
{
    if (!init_done || !func || num_threads >= MAX_THREADS) return -1;

    thread_t *t = calloc(1, sizeof(thread_t));
    if (!t) return -1;
    t->stack = malloc(THREAD_STACK_SIZE);
    if (!t->stack) { free(t); return -1; }

    t->tid   = next_tid++;
    t->state = TS_READY;
    t->func  = func;
    t->arg   = arg;

    if (getcontext(&t->ctx) == -1) { free(t->stack); free(t); return -1; }
    t->ctx.uc_stack.ss_sp   = t->stack;
    t->ctx.uc_stack.ss_size = THREAD_STACK_SIZE;
    t->ctx.uc_link          = NULL;   /* wrapper never returns via uc_link */
    makecontext(&t->ctx, (void (*)(void))thread_wrapper, 1, t);

    /* Ensure SIGALRM is unblocked in the newborn thread */
    sigdelset(&t->ctx.uc_sigmask, SIGALRM);

    enqueue(t);
    all_threads[num_threads++] = t;

    return t->tid;
}

int thread_join(int tid)
{
    if (!init_done || tid == current->tid) return -1;

    thread_t *target = find_thread(tid);
    if (!target) return -1;

    if (target->state == TS_FINISHED)
        return 0; /* already done */

    /* Block current thread, set up joiner relationship */
    current->state  = TS_BLOCKED;
    target->joiner  = current;

    /* Yield to scheduler — we will resume here when target finishes */
    swapcontext(&current->ctx, &sched_ctx);

    return 0;
}

void thread_yield(void)
{
    if (!init_done) return;

    /* raise(SIGALRM) triggers the same handler that the timer uses.
     * The handler will save our context, put us back on the ready
     * queue, and switch to the scheduler — pure round-robin. */
    raise(SIGALRM);
}

void thread_destroy(int tid)
{
    if (!init_done || tid == 0) return; /* never destroy main */

    thread_t *target = find_thread(tid);
    if (!target || target->state != TS_FINISHED) return;

    /* Remove from all_threads */
    for (int i = 0; i < num_threads; i++) {
        if (all_threads[i] == target) {
            all_threads[i] = all_threads[--num_threads];
            break;
        }
    }

    free(target->stack);
    free(target);
}

int thread_self(void)
{
    if (!init_done || !current) return -1;
    return current->tid;
}
