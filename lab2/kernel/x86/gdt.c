#include "x86.h"
#include "device.h"

SegDesc gdt[NR_SEGMENTS];
TSS tss;

// Functions defined in cpu_setup.asm
void switch_to_user(uint32_t entry, uint32_t user_ds, uint32_t user_cs, uint32_t user_esp);
void refresh_kernel_segments(uint16_t data_sel, uint16_t tss_sel);

void initSeg() {
	// Setup segments
	gdt[SEG_KCODE] = SEG_BYTE(STA_X | STA_R, 0,        0xfffff, DPL_KERN);
	gdt[SEG_KDATA] = SEG_BYTE(STA_W,         0,        0xfffff, DPL_KERN);
	gdt[SEG_UCODE] = SEG_BYTE(STA_X | STA_R, APP_BASE, 0x1ffff, DPL_USER);
	gdt[SEG_UDATA] = SEG_BYTE(STA_W,         APP_BASE, 0x1ffff, DPL_USER);
	gdt[SEG_TSS] = SEGTSS(STS_T32A,      &tss, sizeof(TSS)-1, DPL_KERN);
	gdt[SEG_TSS].s = 0;
	setGdt(gdt, sizeof(gdt));

	// Init TSS values
	tss.esp0 = 0x10000; // Kernel stack top (within kernel segment)
	tss.ss0 = KSEL(SEG_KDATA);

	// Load registers via assembly helper
	refresh_kernel_segments(KSEL(SEG_KDATA), KSEL(SEG_TSS));
}

uint32_t getSegBase(uint32_t selector) {
	uint32_t index = selector >> 3;
	if (index >= NR_SEGMENTS) return 0;
	SegDesc *d = &gdt[index];
	return d->base_15_0 | (d->base_23_16 << 16) | (d->base_31_24 << 24);
}

// Function defined in user_entry.asm
void switch_to_user(uint32_t entry, uint32_t user_ds, uint32_t user_cs, uint32_t user_esp);

// TODO: 实现 enterUserSpace 函数
void enterUserSpace(uint32_t entry) {
	switch_to_user(entry, USEL(SEG_UDATA), USEL(SEG_UCODE), 0x20000);
}

void loadUMain(void) {
	uint32_t app_paddr = APP_BASE;
	
	// Load raw app binary (from sector 129, max 64KB)
	for (int i = 0; i < 128; i++) {
		readSect((void*)(app_paddr + i*512), 129+i);
	}

	// For raw binary linked at 0x0 with segment base APP_BASE, 
	// the entry point is virtual address 0.
	enterUserSpace(0x0);
}
