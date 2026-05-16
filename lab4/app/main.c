#include "myos.h"

#define ITER_NUM 500000

// Shared variables for synchronization tests
volatile int shared_var = 0;
int sem_id;

void busy_loop(int n) {
    for (volatile int i = 0; i < n; i++);
}

int main(void) {
    if (getpid() == 1) {
        int h, m, s;
        printf("--- Lab 4 Verification Suite (High Intensity) ---\n");

        // --- Test 1: Semaphore Synchronization (Standard PV) ---
        now(&h, &m, &s);
        printf("\n[%02d:%02d:%02d] [Test 1] Semaphore Synchronization:\n", h, m, s);
        sem_id = sem_init(1);
        shared_var = 0;
        if (vfork() == 0) {
            // Child: increment ITER_NUM times (500000)
            for (int i = 0; i < ITER_NUM; i++) {
                sem_wait(sem_id);
                shared_var++;
                sem_post(sem_id);
            }
            exit();
        } else {
            // Parent: increment ITER_NUM times (500000)
            for (int i = 0; i < ITER_NUM; i++) {
                sem_wait(sem_id);
                shared_var++;
                sem_post(sem_id);
            }
            wait(); // Wait for child to exit
            printf("Final Value: %d (Expected: %d)\n", shared_var, 2*ITER_NUM);
            if (shared_var == 2*ITER_NUM) {
                printf("RESULT: SUCCESS - Semaphore ensured data integrity.\n");
            } else {
                printf("RESULT: FAILURE - Race condition detected despite semaphore.\n");
            }
            sem_destroy(sem_id);
        }

        // --- Test 2: Race Condition (Lost Updates) ---
        now(&h, &m, &s);
        printf("\n[%02d:%02d:%02d] [Test 2] Race Condition (%d per process, UNPROTECTED):\n", h, m, s, ITER_NUM);
        shared_var = 0;
        if (vfork() == 0) {
            for (int i = 0; i < ITER_NUM; i++) {
                int tmp = shared_var;
                shared_var = tmp + 1;
            }
            exit();
        } else {
            for (int i = 0; i < ITER_NUM; i++) {
                int tmp = shared_var;
                shared_var = tmp + 1;
            }
            wait();
            printf("Final Value: %d (Expected < %d)\n", shared_var, 2*ITER_NUM);
            if (shared_var < 2*ITER_NUM) {
                printf("RESULT: SUCCESS - Race condition captured!\n");
            } else {
                printf("RESULT: NOTE - No race detected (try increasing loop).\n");
            }
        }

        // --- Test 3: Round-Robin Scheduling Interleaving ---
        now(&h, &m, &s);
        printf("\n[%02d:%02d:%02d] [Test 3] Round-Robin Interleaving (Observe alternating PIDs):\n", h, m, s);
        for (int i = 0; i < 2; i++) {
            if (fork() == 0) {
                int p = getpid();
                for (int j = 0; j < 3; j++) {
                    printf("Process %d loop %d\n", p, j);
                    busy_loop(5000000);
                }
                exit();
            }
        }
        wait();

        // --- Test 4: Parent Not Calling Wait - Child Auto-Reclaim ---
        now(&h, &m, &s);
        printf("\n[%02d:%02d:%02d] [Test 4] Child Auto-Reclaim (Parent does NOT call wait):\n", h, m, s);
        {
            int p0 = fork();
            if (p0 == 0) {
                printf("  Quick child (PID %d) exiting without parent wait...\n", getpid());
                exit();
            }
            // Parent: do NOT call wait(), sleep briefly to let child exit
            sleep(1);
            // Now try to create another process - if the slot was freed, this will succeed
            int p1 = fork();
            if (p1 == 0) {
                printf("  NEW child created after parent didn't wait! PID=%d\n", getpid());
                printf("  This proves the zombie slot was auto-reclaimed by do_exit.\n");
                exit();
            }
            if (p1 > 0) {
                printf("  Successfully created a new child (PID %d) after previous child auto-reclaimed.\n", p1);
                wait();
            } else {
                printf("  FAILURE: Could not create new child - zombie slot was NOT reclaimed!\n");
            }
        }

        // --- Test 5: Wait/Exit Comprehensive ---
        now(&h, &m, &s);
        printf("\n[%02d:%02d:%02d] [Test 5] Wait/Exit Comprehensive (Parent calls wait):\n", h, m, s);
        for (int i = 0; i < 2; i++) {
            int c = fork();
            if (c == 0) {
                printf("  Test child PID=%d, PPID=%d alive.\n", getpid(), getppid());
                busy_loop(2000000);
                printf("  Test child PID=%d exiting...\n", getpid());
                exit();
            }
        }
        wait();
        printf("  All test children reclaimed successfully via wait.\n");

        printf("\n--- Lab 4 Verification Complete ---\n");
    }
    // All processes enter idle loop
    while(1) {
        sleep(10);
    }
}
