#define _GNU_SOURCE
#include "user_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>

#define NUM_THREADS         20
#define TRIALS_PER_THREAD   50000

typedef struct {
    int           id;
    int           trials;
    int          *results;  /* shared array, slot per thread — no locking needed */
    unsigned int  seed;
} worker_arg_t;

static void worker(void *raw)
{
    worker_arg_t *a = (worker_arg_t *)raw;
    unsigned int  seed = a->seed;

    int hits = 0;
    for (int i = 0; i < a->trials; i++) {
        double x = (double)rand_r(&seed) / RAND_MAX;
        double y = (double)rand_r(&seed) / RAND_MAX;
        if (x * x + y * y <= 1.0)
            hits++;
    }
    a->results[a->id] = hits;
}

int main(void)
{
    thread_scheduler_init();

    worker_arg_t args[NUM_THREADS];
    int          results[NUM_THREADS];
    int          tids[NUM_THREADS];
    int          total_trials = NUM_THREADS * TRIALS_PER_THREAD;

    printf("=== Monte Carlo Pi (User-Threads) ===\n");
    printf("Threads: %d, Trials per thread: %d\n", NUM_THREADS, TRIALS_PER_THREAD);
    printf("Total trials: %d\n\n", total_trials);

    /* Build distinct seeds from high-res time */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned int base_seed = (unsigned int)(tv.tv_sec ^ (tv.tv_usec << 12));

    /* Spawn worker threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].id      = i;
        args[i].trials  = TRIALS_PER_THREAD;
        args[i].results = results;
        args[i].seed    = base_seed ^ (unsigned int)(i * 2654435761U);
        tids[i] = thread_create(worker, &args[i]);
        if (tids[i] < 0) {
            fprintf(stderr, "thread_create failed at %d\n", i);
            return 1;
        }
    }

    /* Join all workers and aggregate */
    int total_hits = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_join(tids[i]);
        total_hits += results[i];
        thread_destroy(tids[i]);
    }

    double pi    = 4.0 * total_hits / total_trials;
    double error = fabs(pi - M_PI);

    printf("Estimated pi = %.6f\n", pi);
    printf("Actual pi    = %.6f\n", M_PI);
    printf("Error        = %.6f\n", error);
    printf("Hits: %d / %d\n", total_hits, total_trials);

    return 0;
}
