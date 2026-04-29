#ifndef __X86_MEMORY_H__
#define __X86_MEMORY_H__

#define DPL_KERN                0
#define DPL_USER                3

// Application segment type bits
#define STA_X       0x8         // Executable segment
#define STA_W       0x2         // Writeable (non-executable segments)
#define STA_R       0x2         // Readable (executable segments)

// System segment type bits
#define STS_T32A    0x9         // Available 32-bit TSS
#define STS_IG32    0xE         // 32-bit Interrupt Gate
#define STS_TG32    0xF         // 32-bit Trap Gate

// GDT entries
#define NR_SEGMENTS      7           // GDT size
#define SEG_KCODE   1           // Kernel code
#define SEG_KDATA   2           // Kernel data/stack
#define SEG_UCODE   3           // User code
#define SEG_UDATA   4           // User data/stack
#define SEG_TSS     5           // Global unique task state segement

#define APP_BASE    0x00020000  // Physical base address of user application

// Selectors
#define KSEL(desc) (((desc) << 3) | DPL_KERN)
#define USEL(desc) (((desc) << 3) | DPL_USER)

struct GateDescriptor {
	uint32_t offset_15_0      : 16;
	uint32_t segment          : 16;
	uint32_t pad0             : 8;
	uint32_t type             : 4;
	uint32_t system           : 1;
	uint32_t privilege_level  : 2;
	uint32_t present          : 1;
	uint32_t offset_31_16     : 16;
};
typedef struct GateDescriptor GateDescriptor;

struct TrapFrame {
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pushad
	uint32_t gs, fs, es, ds;                         // Pushed manually
	int32_t irq;                                     // Interrupt number
	int32_t err_code;                                // Error code
	uint32_t eip, cs, eflags, esp_user, ss_user;     // Pushed by CPU
};

struct SegDesc {
	uint32_t lim_15_0 : 16;
	uint32_t base_15_0 : 16;
	uint32_t base_23_16 : 8;
	uint32_t type : 4;
	uint32_t s : 1;
	uint32_t dpl : 2;
	uint32_t p : 1;
	uint32_t lim_19_16 : 4;
	uint32_t avl : 1;
	uint32_t l : 1;          // 64-bit mode (rsv1)
	uint32_t db : 1;
	uint32_t g : 1;
	uint32_t base_31_24 : 8;
} __attribute__((packed));
typedef struct SegDesc SegDesc;

// Correct macro for setting GDT entries
#define SEG_BYTE(_type, _base, _lim, _dpl) (SegDesc) { \
    .lim_15_0 = ((_lim) & 0xffff), \
    .base_15_0 = ((uint32_t)(_base) & 0xffff), \
    .base_23_16 = (((uint32_t)(_base) >> 16) & 0xff), \
    .type = (_type), .s = 1, .dpl = (_dpl), .p = 1, \
    .lim_19_16 = (((uint32_t)(_lim) >> 16) & 0xf), \
    .avl = 0, .l = 0, .db = 1, .g = 0, \
    .base_31_24 = (((uint32_t)(_base) >> 24) & 0xff) \
}

#define SEGTSS(_type, _base, _lim, _dpl) (SegDesc) { \
    ((_lim) & 0xffff), \
    ((uint32_t)(_base) & 0xffff), \
    ((uint32_t)(_base) >> 16) & 0xff, \
    (_type), 0, (_dpl), 1, \
    ((_lim) >> 16) & 0xf, \
    0, 0, 1, 0, \
    ((uint32_t)(_base) >> 24) & 0xff \
}

// Task state segment format
struct TSS {
	uint32_t link;         // old ts selector
	uint32_t esp0;         // Ring 0 Stack pointer and segment selector
	uint32_t ss0;          // after an increase in privilege level
	union{
		struct{
			char dontcare[88];
		};
		struct{
			uint32_t esp1,ss1,esp2,ss2;
			uint32_t cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
			uint32_t es, cs, ss, ds, fs, gs, ldt;
		};
        };
};
typedef struct TSS TSS;

static inline void setGdt(SegDesc *gdt, uint32_t size) {
	volatile static uint16_t data[3];
	data[0] = size - 1;
	data[1] = (uint32_t)gdt;
	data[2] = (uint32_t)gdt >> 16;
	asm volatile("lgdt (%0)" : : "r"(data));
}

static inline void lLdt(uint16_t sel)
{
	asm volatile("lldt %0" :: "r"(sel));
}

#endif
