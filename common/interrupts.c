/*
 * Copyright (c) 2016-17 Microsemi Corporation.
 * Padmarao Begari, Microsemi Corporation <padmarao.begari@microsemi.com>
 *
 * Copyright (C) 2017 Andes Technology Corporation
 * Rick Chen, Andes Technology Corporation <rick@andestech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <ptrace.h>
#include <asm/encoding.h>
#include "system_common.h"
#include "sg2260_common.h"

static void _exit_trap(int code, uint64_t epc, struct pt_regs *regs);
extern void do_irq(void);
void riscv_start_timer(uint64_t interval);
void riscv_stop_timer(void);

timer_hls_t thls;
uint64_t riscv_get_timer_count()
{
    return thls.interval;
}

void riscv_start_timer(uint64_t interval)
{
    /* T-HEAD C906, CLINT */
    thls.timecmpl0 = (volatile u32 *)CLINT_TIMECMPL0;
    thls.timecmph0 = (volatile u32 *)CLINT_TIMECMPH0;

    thls.interval=0;

	CLINT_MTIME(thls.smtime);

    /* Set compare timer */
    *thls.timecmpl0 = (interval + thls.smtime) & 0xFFFFFFFF;
    *thls.timecmph0 = (interval + thls.smtime) >> 32;

    /* Enable timer interrupt */
    set_csr(mstatus, MSTATUS_MIE);
    set_csr(mie, MIP_MTIP);

	/* C906 mxstatus bit 17 for enable clint*/
//    set_csr(mxstatus, 17);
//	ptr= (unsigned int *) 0x74000000;
//	*ptr = 1;
//	printf("mxstatus=%lx\n",read_csr(mxstatus));
}

void riscv_stop_timer(void)
{
    /* Stop timer and disable timer interrupt */
    clear_csr(mie, MIP_MTIP);
    clear_csr(mip, MIP_MTIP);
}

void timer_interrupt(struct pt_regs *regs)
{
    uint64_t tval;
    if(thls.intr_handler)
        thls.intr_handler();
    /* Clear timer interrupt pending */
    clear_csr(mip, MIP_MTIP);
    thls.interval++;
    /* Update timer for real interval, and not consider about overflow (64 bit) in this case*/
    tval = (thls.interval + 1) * CPU_TIMER_CLOCK / TIMER_HZ + thls.smtime;
    *thls.timecmpl0 = tval & 0xFFFFFFFF;
    *thls.timecmph0 = tval >> 32;
}


uint64_t handle_trap(uint64_t scause, uint64_t epc, struct pt_regs *regs)
{
	uint64_t is_int;

	// printf("handle_trap: scause=0x%08lx, epc=0x%08lx\n", scause, epc);
	is_int = (scause & MCAUSE_INT);
	if ((is_int) && ((scause & MCAUSE_CAUSE)  == IRQ_M_EXT))
		do_irq();
	else if ((is_int) && ((scause & MCAUSE_CAUSE)  == IRQ_M_TIMER))
		timer_interrupt(0);     /* handle_m_timer_interrupt */
	else
		_exit_trap(scause, epc, regs);

	return epc;
}

static void _exit_trap(int code, uint64_t epc, struct pt_regs *regs)
{
	static const char *exception_code[] = {
		"Instruction address misaligned", /*  0 */
		"Instruction access fault",       /*  1 */
		"Illegal instruction",            /*  2 */
		"Breakpoint",                     /*  3 */
		"Load address misaligned",        /*  4 */
		"Load access fault",              /*  5 */
		"Store/AMO address misaligned",   /*  6 */
		"Store/AMO access fault",         /*  7 */
		"Environment call from U-mode",   /*  8 */
		"Environment call from S-mode",   /*  9 */
		"Reserved",                       /* 10 */
		"Environment call from M-mode",   /* 11 */
		"Instruction page fault",         /* 12 */
		"Load page fault",                /* 13 */
		"Reserved",                       /* 14 */
		"Store/AMO page fault",           /* 15 */
		"Reserved",                       /* 16 */
	};

	printf("exception code: %d , %s , epc %08lx , ra %08lx, badaddr %08lx\n",
		code, exception_code[code], epc, regs->ra, regs->sbadaddr);

	if (code == 7)	/* Store/AMO access fault */
		while(1);
}
