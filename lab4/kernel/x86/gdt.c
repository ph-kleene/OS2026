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
	gdt[SEG_UCODE] = SEG_BYTE(STA_X | STA_R, 0,        SEG_LIMIT, DPL_USER); // Default
	gdt[SEG_UDATA] = SEG_BYTE(STA_W,         0,        SEG_LIMIT, DPL_USER); // Default
	gdt[SEG_TSS] = SEGTSS(STS_T32A,      &tss, sizeof(TSS)-1, DPL_KERN);
	gdt[SEG_TSS].s = 0;
	setGdt(gdt, sizeof(gdt));

	// Init TSS values
	tss.esp0 = 0x10000; // Within kernel limit
	tss.ss0 = KSEL(SEG_KDATA);

	// Load registers via assembly helper
	refresh_kernel_segments(KSEL(SEG_KDATA), KSEL(SEG_TSS));
}

void update_user_seg_base(uint32_t base, uint32_t limit) {
	gdt[SEG_UCODE] = SEG_BYTE(STA_X | STA_R, base, limit, DPL_USER);
	gdt[SEG_UDATA] = SEG_BYTE(STA_W,         base, limit, DPL_USER);
}

uint32_t getSegBase(uint32_t selector) {
	uint32_t index = selector >> 3;
	if (index >= NR_SEGMENTS) return 0;
	SegDesc *d = &gdt[index];
	return d->base_15_0 | (d->base_23_16 << 16) | (d->base_31_24 << 24);
}
