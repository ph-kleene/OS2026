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
	// kprintf("S"); // Tiny debug output
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
    /* TODO: 实现timerHandle——递减休眠计数器
     * - ticks计数器++
     * - 遍历所有PCB
     * - 若进程state==BLOCKED且sleep_ticks>0，则递减
     * - 若sleep_ticks减至0，则state改为RUNNABLE
     * - 调用schedule()进行调度
     */
	(void)tf;
	ticks++;
	for (int i = 0; i < MAX_PCB; i++) {
		if (pcb[i].state == BLOCKED && pcb[i].sleep_ticks > 0) {
			pcb[i].sleep_ticks--;
			if (pcb[i].sleep_ticks == 0) {
				pcb[i].state = RUNNABLE;
			}
		}
	}
	schedule();
}

static void syscallFork(struct TrapFrame *tf) {
    /* TODO 1: 实现syscallFork
     * - 调用do_fork()
     * - 将返回值写入tf->eax
     */
	tf->eax = do_fork();
}

static void syscallVfork(struct TrapFrame *tf) {
    /* TODO 2: 实现syscallVfork
     * - 调用do_vfork()
     * - 将返回值写入tf->eax
     */
	tf->eax = do_vfork();
}

static void syscallExec(struct TrapFrame *tf) {
    /* TODO 3: 实现syscallExec
     * - 从tf->ebx/tf->ecx获取参数(sec, num)
     * - 调用do_exec()
     */
	do_exec(tf->ebx, tf->ecx);
}

static void syscallExit(struct TrapFrame *tf) {
    /* TODO 4: 实现syscallExit
     * - 调用do_exit()
     */
	(void)tf;
	do_exit();
}

static void syscallSleep(struct TrapFrame *tf) {
    /* TODO 5: 实现syscallSleep
     * - 从tf->ebx获取ticks参数
     * - 调用do_sleep()
     */
	do_sleep(tf->ebx);
}

static void syscallGetPid(struct TrapFrame *tf) {
    /* TODO 6: 实现syscallGetPid
     * - 调用do_getpid()
     * - 将返回值写入tf->eax
     */
	tf->eax = do_getpid();
}

static void syscallGetPpid(struct TrapFrame *tf) {
    /* TODO 7: 实现syscallGetPpid
     * - 调用do_getppid()
     * - 将返回值写入tf->eax
     */
	tf->eax = do_getppid();
}
