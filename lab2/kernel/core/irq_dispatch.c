#include "common.h"
#include "x86.h"
#include "device.h"

static void GProtectFaultHandle(struct TrapFrame *tf);
static void syscallHandle(struct TrapFrame *tf);
static void syscallWrite(struct TrapFrame *tf);
static void syscallNow(struct TrapFrame *tf);
static void timerHandle(struct TrapFrame *tf);
static void syscallZeroTimeTicks(struct TrapFrame *tf);
static void syscallGetTimeTicks(struct TrapFrame *tf);

void irqHandle(struct TrapFrame *tf) {
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

// TODO 1: 实现 syscallHandle 系统调用分发函数
static void syscallHandle(struct TrapFrame *tf) {
	switch (tf->eax) {
		case 0:
			syscallWrite(tf);
			break;
		case 2:
			syscallNow(tf);
			break;
		case 3:
			syscallZeroTimeTicks(tf);
			break;
		case 4:
			syscallGetTimeTicks(tf);
			break;
		default:
			tf->eax = -1;
			break;
	}
}

// TODO 2: 实现 syscallWrite 写屏功能
static void syscallWrite(struct TrapFrame *tf) {
	uint32_t base = getSegBase(tf->ds);
	char *src = (char *)(base + tf->ecx);
	int len = (int)tf->edx;
	volatile uint16_t *vmem = (volatile uint16_t *)0xb8000;

	for (int i = 0; i < len; i++) {
		char ch = src[i];
		if (ch == '\n') {
			displayRow++;
			displayCol = 0;
		} else if (ch == '\r') {
			displayCol = 0;
		} else {
			vmem[displayRow * 80 + displayCol] = ((uint16_t)0x0f << 8) | (uint8_t)ch;
			displayCol++;
		}

		if (displayCol >= 80) {
			displayCol = 0;
			displayRow++;
		}
		if (displayRow >= 25) {
			scrollScreen();
			displayRow = 24;
		}
	}

	updateCursor(displayRow, displayCol);
	tf->eax = len;
}

// TODO 3: 实现 syscallNow 获取实时时钟
static void syscallNow(struct TrapFrame *tf){
	uint32_t base = getSegBase(tf->ds);
	int *hh = (int *)(base + tf->ecx);
	int *mm = (int *)(base + tf->edx);
	int *ss = (int *)(base + tf->ebx);

	unsigned char h = readRTC(0x04);
	unsigned char m = readRTC(0x02);
	unsigned char s = readRTC(0x00);

	*hh = ((h >> 4) & 0xf) * 10 + (h & 0xf);
	*mm = ((m >> 4) & 0xf) * 10 + (m & 0xf);
	*ss = ((s >> 4) & 0xf) * 10 + (s & 0xf);
	tf->eax = 0;
}

static int ticks = 0;
static int tickCount = 0;

// TODO 4: 实现 timerHandle 时钟中断处理
static void timerHandle(struct TrapFrame *tf) {
	(void)tf;
	ticks++;
	tickCount++;
	if (tickCount >= 100) {
		tickCount = 0;
	}
	asm volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0x20));
}

static void syscallZeroTimeTicks(struct TrapFrame *tf) {
	// kprintf("Syscall: ZeroTimeTicks\n");
    ticks = 0;
}

static void syscallGetTimeTicks(struct TrapFrame *tf) {
    tf->eax = ticks;
}
