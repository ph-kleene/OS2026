#ifndef __X86_H__
#define __X86_H__

#include "x86/cpu.h"
#include "x86/memory.h"
#include "x86/irq.h"

void initSeg(void);
void loadUMain(void);
uint32_t getSegBase(uint32_t selector);

#endif
