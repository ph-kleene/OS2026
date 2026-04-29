#ifndef __IRQ_H__
#define __IRQ_H__

/* Interrupt handling functions */
void initIdt(void);
void initIntr(void);

/* Assembly interrupt entry declarations */
void irqEmpty();
void irqErrorCode();
void irqDoubleFault();
void irqInvalidTSS();
void irqSegNotPresent();
void irqStackSegFault();
void irqGProtectFault();
void irqPageFault();
void irqAlignCheck();
void irqSecException();
void irqSyscall();
void irqTimer();

#endif
