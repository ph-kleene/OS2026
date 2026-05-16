#include "proc.h"
#include "device.h"
#include "syscall.h"

struct PCB pcb[MAX_PCB];
struct PCB *current = NULL;
struct Semaphore semaphores[MAX_SEM];

// --- Process Scheduling (Round-Robin) ---
void schedule() {
    struct PCB *next = NULL;
    struct PCB *idle_candidate = &pcb[0];
    int found_user = 0;

    // Round-Robin scan: start from current->pid + 1
    int start = (current->pid + 1) % MAX_PCB;
    for (int i = 0; i < MAX_PCB; i++) {
        int idx = (start + i) % MAX_PCB;
        if ((pcb[idx].state == RUNNABLE || pcb[idx].state == RUNNING) && idx != current->pid) {
            if (idx > 0) {
                // Prefer user processes (PID > 0)
                next = &pcb[idx];
                found_user = 1;
                break;
            } else {
                // Idle is a fallback candidate
                idle_candidate = &pcb[idx];
            }
        }
    }

    // If no user process found, fall back to Idle
    if (!found_user) {
        if (idle_candidate->state == RUNNABLE || idle_candidate->state == RUNNING) {
            next = idle_candidate;
        }
    }

    if (next && next != current) {
        // Save current state if it was RUNNING
        if (current->state == RUNNING) {
            current->state = RUNNABLE;
        }
        // Mark next process as RUNNING
        next->state = RUNNING;
        // Reset time slice to 5 ticks (50ms)
        next->time_count = 5;
        // Update current pointer
        current = next;
        // Update TSS.esp0 to point to the new process's kernel stack top
        extern TSS tss;
        tss.esp0 = (uint32_t)next->kstack + KSTACK_SIZE;
        // Switch GDT base for address space isolation
        update_user_seg_base(next->mem_base, next->mem_limit);
    }
}

int do_fork() {
    int child_idx = -1;
    for (int i = 1; i < MAX_PCB; i++) {
        if (pcb[i].state == UNUSED) { child_idx = i; break; }
    }
    if (child_idx == -1) return -1;

    struct PCB *child = &pcb[child_idx];
    struct PCB *parent = current;

    uint8_t *src = (uint8_t *)parent->mem_base;
    uint8_t *dst = (uint8_t *)child->mem_base;
    for (uint32_t i = 0; i < SLOT_SIZE; i++) dst[i] = src[i];

    uint32_t offset = (uint32_t)parent->tf - (uint32_t)parent->kstack;
    child->tf = (struct TrapFrame *)((uint32_t)child->kstack + offset);
    for (int i = 0; i < KSTACK_SIZE; i++) child->kstack[i] = parent->kstack[i];

    child->tf->eax = 0;
    child->pid = child_idx;
    child->ppid = parent->pid;
    child->state = RUNNABLE;
    child->sleep_ticks = 0;
    child->time_count = 5;
    child->mem_limit = parent->mem_limit;
    child->vfork_count = 0;
    child->next = NULL;

    schedule();
    return child->pid;
}

int do_vfork() {
    int child_idx = -1;
    for (int i = 1; i < MAX_PCB; i++) {
        if (pcb[i].state == UNUSED) { child_idx = i; break; }
    }
    if (child_idx == -1) return -1;

    struct PCB *parent = current;
    parent->vfork_count++;
    if (parent->vfork_count > 1) {
        parent->vfork_count--;
        return -1;
    }

    struct PCB *child = &pcb[child_idx];
    child->mem_base = parent->mem_base;
    child->mem_limit = parent->mem_limit;

    uint32_t offset = (uint32_t)parent->tf - (uint32_t)parent->kstack;
    child->tf = (struct TrapFrame *)((uint32_t)child->kstack + offset);
    for (int i = 0; i < KSTACK_SIZE; i++) child->kstack[i] = parent->kstack[i];

    child->tf->eax = 0;
    child->tf->esp_user = parent->tf->esp_user - VFORK_STACK_OFFSET;
    child->tf->ebp = parent->tf->ebp - VFORK_STACK_OFFSET;

    // Stack Copy: Ensure child inherits the environment to return from vfork.
    uint32_t stack_len = SEG_LIMIT - parent->tf->esp_user;
    if (stack_len > 0 && stack_len < VFORK_STACK_OFFSET) {
        uint32_t *src_stack = (uint32_t *)(parent->mem_base + parent->tf->esp_user);
        uint32_t *dst_stack = (uint32_t *)(child->mem_base + child->tf->esp_user);
        
        for (uint32_t i = 0; i < stack_len / 4; i++) {
            uint32_t val = src_stack[i];
            if (val >= parent->tf->esp_user && val < SEG_LIMIT) {
                dst_stack[i] = val - VFORK_STACK_OFFSET;
            } else {
                dst_stack[i] = val;
            }
        }
        
        uint32_t remainder = stack_len % 4;
        if (remainder > 0) {
            uint8_t *src_byte = (uint8_t *)src_stack;
            uint8_t *dst_byte = (uint8_t *)dst_stack;
            for (uint32_t i = stack_len - remainder; i < stack_len; i++) {
                dst_byte[i] = src_byte[i];
            }
        }
    }

    child->pid = child_idx;
    child->ppid = parent->pid;
    child->state = RUNNABLE;
    child->sleep_ticks = 0;
    child->time_count = 5;
    child->vfork_count = 0;
    child->next = NULL;

    schedule();
    return child->pid;
}

void do_exec(uint32_t sec, uint32_t num) {
    if (current->pid > 0) {
        uint32_t correct_base = 0x20000 + (current->pid - 1) * SLOT_SIZE;
        if (current->mem_base != correct_base) {
            current->mem_base = correct_base;
            if (current->ppid >= 0 && current->ppid < MAX_PCB) pcb[current->ppid].vfork_count--;
        }
    }
    for (uint32_t i = 0; i < num; i++) readSect((void *)(current->mem_base + i * 512), sec + i);
    current->mem_limit = SEG_LIMIT;
    update_user_seg_base(current->mem_base, current->mem_limit);
    current->tf->eip = 0;
    current->tf->esp_user = SEG_LIMIT + 1; 
    current->tf->eax = 0;
}

void do_exit() {
    // 1. Set current process state to ZOMBIE
    current->state = ZOMBIE;

    // 2. Orphan handling: redirect all children's ppid to idle (PID 0)
    for (int i = 0; i < MAX_PCB; i++) {
        if (pcb[i].ppid == current->pid) {
            pcb[i].ppid = 0;
        }
    }

    // 3. Notify and possibly wake up the parent
    int parent_is_waiting = 0;
    if (current->ppid >= 0 && current->ppid < MAX_PCB) {
        struct PCB *parent = &pcb[current->ppid];
        if (parent->state == WAIT_CHILD) {
            parent_is_waiting = 1;
            // Check if this is the last active child of the parent
            int has_active_children = 0;
            for (int i = 0; i < MAX_PCB; i++) {
                if (pcb[i].ppid == parent->pid &&
                    pcb[i].state != UNUSED &&
                    pcb[i].state != ZOMBIE) {
                    has_active_children = 1;
                    break;
                }
            }
            // If no more active children, wake up the parent
            if (!has_active_children) {
                parent->state = RUNNABLE;
            }
        }
    }

    // 4. vfork memory recovery
    if (current->pid > 0) {
        uint32_t correct_base = 0x20000 + (current->pid - 1) * SLOT_SIZE;
        if (current->mem_base != correct_base) {
            // This was a vfork child sharing parent memory
            current->mem_base = correct_base;
            if (current->ppid >= 0 && current->ppid < MAX_PCB) {
                pcb[current->ppid].vfork_count--;
            }
        }
    }

    // 5. Self-reclaim if parent is NOT waiting
    //    If parent is WAIT_CHILD, stay ZOMBIE for the parent to collect.
    if (!parent_is_waiting) {
        current->state = UNUSED;
    }

    // 6. Trigger scheduler
    schedule();
}

int do_wait() {
    // Check if any child processes exist
    int has_children = 0;
    for (int i = 0; i < MAX_PCB; i++) {
        if (pcb[i].ppid == current->pid && pcb[i].state != UNUSED) {
            has_children = 1;
            break;
        }
    }
    if (!has_children) return -1;

    // Check if any child is still active (not ZOMBIE)
    int has_active = 0;
    for (int i = 0; i < MAX_PCB; i++) {
        if (pcb[i].ppid == current->pid &&
            pcb[i].state != UNUSED &&
            pcb[i].state != ZOMBIE) {
            has_active = 1;
            break;
        }
    }

    if (has_active) {
        // Block current process, wait for children to exit
        current->state = WAIT_CHILD;
        // Mesa retry: rewind eip so int 0x80 is re-executed
        current->tf->eip -= 2;
        schedule();
        return SYS_WAIT;
    }

    // All children are ZOMBIE - clean up
    for (int i = 0; i < MAX_PCB; i++) {
        if (pcb[i].ppid == current->pid && pcb[i].state == ZOMBIE) {
            pcb[i].state = UNUSED;
        }
    }
    return 0;
}

int do_sem_init(int value) {
    for (int i = 0; i < MAX_SEM; i++) {
        if (!semaphores[i].used) {
            semaphores[i].used = 1;
            semaphores[i].value = value;
            semaphores[i].wait_queue = NULL;
            return i;
        }
    }
    return -1;
}

int do_sem_destroy(int sem_id) {
    if (sem_id < 0 || sem_id >= MAX_SEM) return -1;
    if (!semaphores[sem_id].used) return -1;
    semaphores[sem_id].used = 0;
    semaphores[sem_id].wait_queue = NULL;
    return 0;
}

int do_sem_wait(int sem_id) {
    // Parameter validation
    if (sem_id < 0 || sem_id >= MAX_SEM) return -1;
    if (!semaphores[sem_id].used) return -1;

    // Resource available: decrement and return
    if (semaphores[sem_id].value > 0) {
        semaphores[sem_id].value--;
        return 0;
    }

    // Resource unavailable: block current process (FIFO enqueue)
    struct PCB **p = &semaphores[sem_id].wait_queue;
    while (*p != NULL) {
        p = &((*p)->next);
    }
    *p = current;
    current->next = NULL;
    current->state = BLOCKED;
    // Mesa retry: rewind eip to re-execute int 0x80 upon wakeup
    current->tf->eip -= 2;
    schedule();
    return SYS_SEM_WAIT;
}

int do_sem_post(int sem_id) {
    // Parameter validation
    if (sem_id < 0 || sem_id >= MAX_SEM) return -1;
    if (!semaphores[sem_id].used) return -1;

    // Increment value unconditionally
    semaphores[sem_id].value++;

    // FIFO wakeup: dequeue and wake the head of wait_queue
    if (semaphores[sem_id].wait_queue != NULL) {
        struct PCB *woken = semaphores[sem_id].wait_queue;
        semaphores[sem_id].wait_queue = woken->next;
        woken->state = RUNNABLE;
        woken->next = NULL;
        // Do not call schedule() here - let the current process continue
        // The woken process will be scheduled by Round-Robin timer preemption
    }

    return 0;
}

void do_sleep(int ticks) {
    if (ticks > 0) {
        current->sleep_ticks = ticks;
        current->state = BLOCKED;
    }
    schedule();
}

int do_getpid() { return current->pid; }
int do_getppid() { return current->ppid; }
