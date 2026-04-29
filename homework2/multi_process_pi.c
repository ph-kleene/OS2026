#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <math.h>

#define NUM_CHILDREN      1000
#define TRIALS_PER_CHILD  255

int main(void)
{
    int total_hits    = 0;
    int total_trials  = NUM_CHILDREN * TRIALS_PER_CHILD;
    int completed     = 0;

    printf("=== Monte Carlo Pi (Multi-Process) ===\n");
    printf("Children: %d, Trials per child: %d\n", NUM_CHILDREN, TRIALS_PER_CHILD);
    printf("Total trials: %d\n\n", total_trials);
    fflush(stdout);  /* prevent children inheriting buffered output */

    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            /* Child: run random trials and return hit count via exit() */
            srand48((long)(time(NULL) ^ (getpid() << 16)));

            int hits = 0;
            for (int j = 0; j < TRIALS_PER_CHILD; j++) {
                double x = drand48();
                double y = drand48();
                if (x * x + y * y <= 1.0)
                    hits++;
            }
            exit(hits); /* hit count fits in 8-bit exit code (max 255) */
        }
    }

    /* Parent: wait for all children and aggregate results */
    while (completed < NUM_CHILDREN) {
        int status;
        pid_t pid = wait(&status);
        if (pid < 0) {
            perror("wait");
            continue;
        }
        if (WIFEXITED(status)) {
            total_hits += WEXITSTATUS(status);
            completed++;
        }
    }

    double pi    = 4.0 * total_hits / total_trials;
    double error = fabs(pi - M_PI);

    printf("Estimated pi = %.6f\n", pi);
    printf("Actual pi    = %.6f\n", M_PI);
    printf("Error        = %.6f\n", error);
    printf("Hits: %d / %d\n", total_hits, total_trials);

    return 0;
}
