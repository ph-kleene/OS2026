#include "common.h"
#include "x86.h"
#include "device.h"

void initTimer();

void kEntry(void) {
	// Disable interrupts during init
	initSerial();
	initIdt(); 
	initIntr(); 
	initTimer();
	initSeg(); 
	initVga(); 

	kprintf("Kernel initialized.\n");

	// Kernel splash message
	const char *msg = "Kernel Start...";
	volatile uint16_t *vga = (volatile uint16_t *)0xb8000;
	for (int i = 0; msg[i] != '\0'; i++) {
		vga[i] = (uint16_t)msg[i] | (0x0a << 8);
	}

	kprintf("Loading application module...\n");
	loadUMain(); 

	while(1);
	assert(0);
}
