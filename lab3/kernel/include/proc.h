#ifndef __PROC_H__
#define __PROC_H__

#include "common.h"
#include "x86.h"

#define KSTACK_SIZE 4096
#define SLOT_SIZE   (128 * 1024)
#define SEG_LIMIT   (SLOT_SIZE - 1)

// Process states
#define UNUSED   0
#define RUNNABLE 1
#define RUNNING  2
#define BLOCKED  3

#define VFORK_STACK_OFFSET 32768

// Syscall numbers removed - moved to syscall.h

#define MAX_PCB 5

struct PCB {
    struct TrapFrame *tf;       // Context on kernel stack
    uint8_t kstack[KSTACK_SIZE]; // Private kernel stack
    int pid;
    int ppid;
    int state;
    int sleep_ticks;
    uint32_t mem_base;
    uint32_t mem_limit;
    int vfork_count;
};

extern struct PCB pcb[MAX_PCB];
extern struct PCB *current;

void schedule();
int do_fork();
int do_vfork();
void do_exec(uint32_t sec, uint32_t num);
void do_exit();
void do_sleep(int ticks);
int do_getpid();
int do_getppid();

#endif
