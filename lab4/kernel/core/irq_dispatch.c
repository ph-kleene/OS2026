#include "common.h"
#include "x86.h"
#include "device.h"
#include "proc.h"
#include "syscall.h"

static void GProtectFaultHandle(struct TrapFrame *tf);
static void syscallHandle(struct TrapFrame *tf);
static void syscallWrite(struct TrapFrame *tf);
static void syscallNow(struct TrapFrame *tf);
static void timerHandle(struct TrapFrame *tf);

static void syscallFork(struct TrapFrame *tf);
static void syscallVfork(struct TrapFrame *tf);
static void syscallExec(struct TrapFrame *tf);
static void syscallExit(struct TrapFrame *tf);
static void syscallSleep(struct TrapFrame *tf);
static void syscallGetPid(struct TrapFrame *tf);
static void syscallGetPpid(struct TrapFrame *tf);
static void syscallWait(struct TrapFrame *tf);
static void syscallSemInit(struct TrapFrame *tf);
static void syscallSemDestroy(struct TrapFrame *tf);
static void syscallSemPost(struct TrapFrame *tf);
static void syscallSemWait(struct TrapFrame *tf);

static int ticks = 0;

void irqHandle(struct TrapFrame *tf) {
	if (current) current->tf = tf;

	switch(tf->irq) {
		case -1:
			if (tf->err_code != 0) {
				GProtectFaultHandle(tf);
			}
			break;
		case 0xd:
			GProtectFaultHandle(tf);
			break;
		case 0x20:
			timerHandle(tf);
			break;
		case 0x80:
			syscallHandle(tf);
			break;
		default:
			kprintf("Unhandled IRQ %x, ERR %x\n", tf->irq, tf->err_code);
			assert(0);
	}
}

static void GProtectFaultHandle(struct TrapFrame *tf){
	kprintf("\n--- General Protection Fault Caught by Kernel ---\n");
	kprintf("Application tried to access restricted memory or performed invalid operation.\n");
	kprintf("Trap Frame Info:\n");
	kprintf("  IRQ: %x, Error Code: %x\n", tf->irq, tf->err_code);
	kprintf("  EIP: %x, CS: %x, CPL: %d\n", tf->eip, tf->cs, tf->cs & 3);
	kprintf("  DS: %x, ES: %x, SS: %x\n", tf->ds, tf->es, tf->ss_user);
	
	if ((tf->cs & 3) == 3) {
		kprintf("Confirmed: Exception occurred in USER MODE (CPL 3).\n");
		kprintf("Segmentation protection is ACTIVE.\n");
	}

	kprintf("Halting system.\n");
	while(1);
}

static void syscallHandle(struct TrapFrame *tf) {
	/*
	kprintf("Syscall %d from CPL %d, DS=%x\n", tf->eax, tf->cs & 3, tf->ds);
	*/
	// kprintf("S%d ", tf->eax);
	switch(tf->eax) {
		case SYS_WRITE: syscallWrite(tf); break;
		case SYS_FORK:  syscallFork(tf); break;
		case SYS_VFORK: syscallVfork(tf); break;
		case SYS_NOW:   syscallNow(tf); break;
		case SYS_EXEC:  syscallExec(tf); break;
		case SYS_EXIT:  syscallExit(tf); break;
		case SYS_SLEEP: syscallSleep(tf); break;
		case SYS_GETPID: syscallGetPid(tf); break;
		case SYS_GETPPID: syscallGetPpid(tf); break;
		case SYS_WAIT: syscallWait(tf); break;
		case SYS_SEM_INIT: syscallSemInit(tf); break;
		case SYS_SEM_DESTROY: syscallSemDestroy(tf); break;
		case SYS_SEM_POST: syscallSemPost(tf); break;
		case SYS_SEM_WAIT: syscallSemWait(tf); break;
		default: break;
	}
}

static void syscallWrite(struct TrapFrame *tf) {
	// kprintf("Syscall: Write\n");
	char *str = (char*)tf->ebx;
	int size = tf->ecx;
	volatile uint16_t *vga = (volatile uint16_t *)0xb8000;

	uint32_t base = getSegBase(tf->ds);
	char *kernel_str = (char *)(base + (uint32_t)str);
    
    // kprintf("W%d ", size);

	for (int i = 0; i < size; i++) {
		char character = kernel_str[i];
		putChar(character); // Also to serial

		if(character == '\r'){
			displayCol = 0;
		} else if(character != '\n'){
			vga[displayRow * 80 + displayCol] = character | (0x0c << 8);
			displayCol++;
			if(displayCol == 80){
				displayCol = 0;
				displayRow++;
				if(displayRow == 25){
					scrollScreen();
					displayRow = 24;
					displayCol = 0;
				}
			}
		} else {
			displayCol = 0;
			displayRow++;
			if(displayRow == 25){
				scrollScreen();
				displayRow = 24;
				displayCol = 0;
			}
		}
	}
	updateCursor(displayRow, displayCol);
}

static void syscallNow(struct TrapFrame *tf){
	// kprintf("Syscall: Now\n");
	int *hh = (int*)tf->ebx;
	int *mm = (int*)tf->ecx;
	int *ss = (int*)tf->edx;

	uint32_t h = readRTC(0x04);
	uint32_t m = readRTC(0x02);
	uint32_t s = readRTC(0x00);

	uint32_t hour = (h >> 4) * 10 + (h & 0x0f);
	uint32_t minute = (m >> 4) * 10 + (m & 0x0f);
	uint32_t second = (s >> 4) * 10 + (s & 0x0f);

	uint32_t base = getSegBase(tf->ds);
	*(int *)(base + (uint32_t)hh) = hour;
	*(int *)(base + (uint32_t)mm) = minute;
	*(int *)(base + (uint32_t)ss) = second;
}

static void timerHandle(struct TrapFrame *tf) {
    ticks++;
    // Wake up sleeping processes
    for (int i = 0; i < MAX_PCB; i++) {
        if (pcb[i].state == BLOCKED && pcb[i].sleep_ticks > 0) {
            pcb[i].sleep_ticks--;
            if (pcb[i].sleep_ticks == 0) {
                pcb[i].state = RUNNABLE;
            }
        }
    }

    // Time slice preemption: decrement current process's time_count
    if (current->state == RUNNING) {
        current->time_count--;
        if (current->time_count <= 0) {
            schedule();
            return;
        }
    }

    // Always trigger schedule() so newly woken RUNNABLE processes get a chance
    schedule();
}

static void syscallFork(struct TrapFrame *tf) {
    tf->eax = do_fork();
}

static void syscallVfork(struct TrapFrame *tf) {
    tf->eax = do_vfork();
}

static void syscallExec(struct TrapFrame *tf) {
    do_exec(tf->ebx, tf->ecx);
}

static void syscallExit(struct TrapFrame *tf) {
    do_exit();
}

static void syscallSleep(struct TrapFrame *tf) {
    do_sleep(tf->ebx);
}

static void syscallGetPid(struct TrapFrame *tf) {
    tf->eax = do_getpid();
}

static void syscallGetPpid(struct TrapFrame *tf) {
    tf->eax = do_getppid();
}

static void syscallWait(struct TrapFrame *tf) {
    tf->eax = do_wait();
}

static void syscallSemInit(struct TrapFrame *tf) {
    tf->eax = do_sem_init(tf->ebx);
}

static void syscallSemDestroy(struct TrapFrame *tf) {
    tf->eax = do_sem_destroy(tf->ebx);
}

static void syscallSemPost(struct TrapFrame *tf) {
    tf->eax = do_sem_post(tf->ebx);
}

static void syscallSemWait(struct TrapFrame *tf) {
    tf->eax = do_sem_wait(tf->ebx);
}
