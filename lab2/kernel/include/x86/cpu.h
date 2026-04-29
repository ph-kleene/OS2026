#ifndef __X86_CPU_H__
#define __X86_CPU_H__

#include "common.h"

/* Put CPU to sleep until next interrupt */
static inline void waitForInterrupt() {
	asm volatile("hlt");
}

/* Load IDTR */
static inline void saveIdt(void *addr, uint32_t size) {
	static volatile uint16_t data[3];
	data[0] = size - 1;
	data[1] = (uint32_t)addr;
	data[2] = ((uint32_t)addr) >> 16;
	asm volatile("lidt (%0)" : : "r"(data));
}

/* Enable external interrupts */
static inline void enableInterrupt(void) {
	asm volatile("sti");
}

/* Disable external interrupts */
static inline void disableInterrupt(void) {
	asm volatile("cli");
}

#define NR_IRQ    256

#endif
