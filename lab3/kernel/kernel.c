#include "common.h"
#include "x86.h"
#include "device.h"
#include "proc.h"
#include "syscall.h"

void initTimer();

static void idle_write(const char *s) {
	int len = 0;
	while(s[len]) len++;
	asm volatile("int $0x80" : : "a"(SYS_WRITE), "b"(s), "c"(len));
}

static inline int idle_fork() {
	int ret;
	asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_FORK));
	return ret;
}

static inline int idle_vfork() {
	int ret;
	asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_VFORK));
	return ret;
}

static inline int idle_exec(uint32_t sec, uint32_t num) {
	int ret;
	asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_EXEC), "b"(sec), "c"(num));
	return ret;
}

void idle_main() {
	idle_write("Idle process (PID 0) started.\r\n");
	if (idle_vfork() == 0) {
		idle_write("Child process (PID 1) starting exec...\r\n");
		idle_exec(129, 128); // Load from sector 129
	}
	while(1);
}

void kEntry(void) {
	// Disable interrupts during init
	initSerial();
	initIdt(); 
	initIntr(); 
	initTimer();
	initSeg(); 
	initVga(); 

	kprintf("Kernel initialized.\n");

	// Initialize PCBs
	/* TODO: 遍历MAX_PCB个进程控制块，初始化各字段默认值
	 * - 设置state为UNUSED
	 * - 设置pid为i
	 * - 设置ppid为-1
	 * - 设置mem_limit为SEG_LIMIT
	 * - Slot 0的mem_base为0，其他槽位为0x20000 + (i-1) * SLOT_SIZE
	 */
	for (int i = 0; i < MAX_PCB; i++) {
		pcb[i].tf = NULL;
		pcb[i].pid = i;
		pcb[i].ppid = -1;
		pcb[i].state = UNUSED;
		pcb[i].sleep_ticks = 0;
		pcb[i].mem_limit = SEG_LIMIT;
		pcb[i].mem_base = (i == 0) ? 0 : (0x20000 + (i - 1) * SLOT_SIZE);
		pcb[i].vfork_count = 0;
	}

	// Setup Idle process (pcb[0])
	struct PCB *p = &pcb[0];
	p->state = RUNNING;
	current = p;
	
	// Construct initial TrapFrame on kstack
	p->tf = (struct TrapFrame *)((uint32_t)p->kstack + KSTACK_SIZE - sizeof(struct TrapFrame));
	p->tf->cs = USEL(SEG_UCODE);
	p->tf->ds = p->tf->es = p->tf->fs = p->tf->gs = USEL(SEG_UDATA);
	p->tf->ss_user = USEL(SEG_UDATA);
	p->tf->esp_user = 0x10000;
	p->tf->eip = (uint32_t)idle_main;
	p->tf->eflags = 0x202; // IF=1

	kprintf("Entering user space (Idle)...\n");
	
	extern TSS tss;
	tss.esp0 = (uint32_t)p->kstack + KSTACK_SIZE;

	asm volatile(
		"movl %0, %%esp\n"
		"popal\n"
		"popl %%gs\n"
		"popl %%fs\n"
		"popl %%es\n"
		"popl %%ds\n"
		"addl $8, %%esp\n"
		"iret"
		: : "r"(p->tf) : "memory"
	);

	while(1);
	assert(0);
}
