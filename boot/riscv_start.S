#include "asm/csr.h"
#include "smp.h"

#define PLAT_SG_HOLD_BASE 0X0
#define PLAT_SG_TRUSTED_MAILBOX_BASE 0x0

	.option norvc
	.section .text.init,"ax",@progbits
	.globl entry_vector

entry_vector:
#if 0
/* this setting is for RISCV C920 */
	li	t0, 0x70013
	csrw	CSR_MCOR, t0
#endif
	/* disable l0btb to workround auipc bug */
	li	t0, 0x1ff
	csrw	CSR_MHCR, t0

	li	t0, 0xe0410009
	csrw	CSR_MCCR2, t0

	li	t0, 0xe6e30c
	csrw	CSR_MHINT, t0

	csrr	t0, CSR_MHINT2
	li	t1, 3
	slli	t1, t1, 7
	or	t0, t0, t1
	li	t1, 1
	slli	t1, t1, 22
	or	t0, t0, t1
	csrw	CSR_MHINT2, t0

	li	t0, 0x638000
	csrw	CSR_MXSTATUS, t0

	/* disable dvm */
	li	t0, 0x20
	csrw	CSR_MHINT4, t0

	li	t0, 1
	csrw	CSR_MSMPR, t0

#if 0
	/* delegate 0-7 exceptions to S-mode */
	csrr	t0, CSR_MEDELEG
	ori	t0, t0, 0xff
	csrw	CSR_MEDELEG, t0
#endif

	fence.i
	fence iorw, iorw

reg_loop:
	li	x1, 0
	li	x2, 0
	li	x3, 0
	li	x4, 0
	li	x5, 0
	li	x6, 0
	li	x7, 0
	li	x8, 0
	li	x9, 0
	li	x10, 0
	li	x11, 0
	li	x12, 0
	li	x13, 0
	li	x14, 0
	li	x15, 0
	li	x16, 0
	li	x17, 0
	li	x18, 0
	li	x19, 0
	li	x20, 0
	li	x21, 0
	li	x22, 0
	li	x23, 0
	li	x24, 0
	li	x25, 0
	li	x26, 0
	li	x27, 0
	li	x28, 0
	li	x29, 0
	li	x30, 0
	li	x31, 0
loop:

	csrw	CSR_MSCRATCH, x0
	csrw	CSR_MIE, x0
	/* write mtvec and make sure it sticks */
	la	t0, trap_vector
	csrw	mtvec, t0

#if 0
	li	t0, PLAT_SG_HOLD_BASE
	sd	zero, (t0)
	li	t1, PLAT_SG_TRUSTED_MAILBOX_BASE
	sd	zero, (t1)
#endif
	csrr	t2, mhartid
	beq	t2, zero, 1f

#ifdef CONFIG_SUPPORT_SMP
	j	multi_core
#endif
#if 0
poll_mailbox:
	ld	t2, (t0)
	beq	t2, zero, poll_mailbox
	ld	t2, (t1)
	jalr	t2
#endif
1:
	la sp, STACK_TOP0-8
	/* double-word aligned in rom.lds */
	la a2, __DATA_LMA_START__
	la a3, __DATA_START__
	la a4, __DATA_SIZE__

data_cpy:
	ld a5, 0(a2)
	sd a5, 0(a3)
	addi a2, a2, 8
	addi a3, a3, 8
	addi a4, a4, -8
	bnez a4, data_cpy

	la a3, __BSS_START__
	la a4, __BSS_SIZE__

data_clear:
	sd x0, 0(a3)
	addi a3, a3, 8
	addi a4, a4, -8
	bnez a4, data_clear

primary_core:
	j core_init_done

multi_core:
#ifdef CONFIG_SUPPORT_SMP
	la	t0, smp_context
	csrr	t1, mhartid
	slli	t1, t1, SMP_CONTEXT_SIZE_SHIFT
	add	t0, t0, t1
	li	t1, SMP_CONTEXT_SIZE
	li	t2, 0
clean_smp_context:
	slt	t3, t2, t1
	beqz	t3, clean_smp_context_done
	add	t3, t0, t2
	sd	zero, (t3)
	addi	t2, t2, 8

	j	clean_smp_context

clean_smp_context_done:
secondary_core_poll:

	la	t0, smp_context
	csrr	t1, mhartid
	slli	t1, t1, SMP_CONTEXT_SIZE_SHIFT
	add	t1, t0, t1
	add	t2, t1, SMP_CONTEXT_FN_OFFSET
	add	t3, t1, SMP_CONTEXT_SP_OFFSET
	add	t4, t1, SMP_CONTEXT_PRIV_OFFSET
	ld	a0, (t4)
	ld	t5, (t3)
	add	t3, t1, SMP_CONTEXT_STATCKSIZE_OFFSET
	ld	t6, (t3)
	add	sp, t6, t5
	ld	t5, (t2)
	bnez	t5, mul_core_wake_up
	j	secondary_core_poll
mul_core_wake_up:
	la      t0, smp_context
	csrr    t1, mhartid
	slli    t1, t1, SMP_CONTEXT_SIZE_SHIFT
	add     t1, t0, t1
	add     t2, t1, SMP_CONTEXT_FN_OFFSET
	add     t3, t1, SMP_CONTEXT_SP_OFFSET
	add     t4, t1, SMP_CONTEXT_PRIV_OFFSET
	ld      a0, (t4)
	ld      t5, (t3)
	add     t3, t1, SMP_CONTEXT_STATCKSIZE_OFFSET
	ld      t6, (t3)
	add     sp, t6, t5
	ld      t5, (t2)

	jalr    t5

	la	t0, smp_context
	csrr	t1, mhartid
	slli	t1, t1, SMP_CONTEXT_SIZE_SHIFT
	add	t1, t0, t1
	add	t1, t1, SMP_CONTEXT_FN_OFFSET
	sd	zero, (t1)
	j	secondary_core_poll
#endif

core_init_done:
	jal system_init
	call main
	j die

	.align 4
trap_vector:
	/* j trap_vector */
	j trap_entry
	/* never reach here */
	j die


#define LREG			ld
#define SREG			sd
#define REGBYTES			8
#define RELOC_TYPE			R_RISCV_64
#define SYM_INDEX			0x20
#define SYM_SIZE			0x18
/*
 * trap entry
 */
trap_entry:
	addi	sp, sp, -35*REGBYTES
	SREG	x1, 1*REGBYTES(sp)
	SREG	x2, 2*REGBYTES(sp)
	SREG	x3, 3*REGBYTES(sp)
	SREG	x4, 4*REGBYTES(sp)
	SREG	x5, 5*REGBYTES(sp)
	SREG	x6, 6*REGBYTES(sp)
	SREG	x7, 7*REGBYTES(sp)
	SREG	x8, 8*REGBYTES(sp)
	SREG	x9, 9*REGBYTES(sp)
	SREG	x10, 10*REGBYTES(sp)
	SREG	x11, 11*REGBYTES(sp)
	SREG	x12, 12*REGBYTES(sp)
	SREG	x13, 13*REGBYTES(sp)
	SREG	x14, 14*REGBYTES(sp)
	SREG	x15, 15*REGBYTES(sp)
	SREG	x16, 16*REGBYTES(sp)
	SREG	x17, 17*REGBYTES(sp)
	SREG	x18, 18*REGBYTES(sp)
	SREG	x19, 19*REGBYTES(sp)
	SREG	x20, 20*REGBYTES(sp)
	SREG	x21, 21*REGBYTES(sp)
	SREG	x22, 22*REGBYTES(sp)
	SREG	x23, 23*REGBYTES(sp)
	SREG	x24, 24*REGBYTES(sp)
	SREG	x25, 25*REGBYTES(sp)
	SREG	x26, 26*REGBYTES(sp)
	SREG	x27, 27*REGBYTES(sp)
	SREG	x28, 28*REGBYTES(sp)
	SREG	x29, 29*REGBYTES(sp)
	SREG	x30, 30*REGBYTES(sp)
	SREG	x31, 31*REGBYTES(sp)
	csrr	a1, mepc
	SREG	a1, 0*REGBYTES(sp)
	csrr	t0, mstatus
	SREG	t0, 32*REGBYTES(sp)
	csrr	t1, mbadaddr
	SREG	t1, 33*REGBYTES(sp)
	csrr	a0, mcause
	SREG	a0, 34*REGBYTES(sp)
	mv	a2, sp
	jal	handle_trap

	LREG	t0, 32*REGBYTES(sp)
	LREG	a1, 0*REGBYTES(sp)
	csrw	mstatus, t0
	csrw	mepc, a1

	LREG	x1, 1*REGBYTES(sp)
	LREG	x2, 2*REGBYTES(sp)
	LREG	x3, 3*REGBYTES(sp)
	LREG	x4, 4*REGBYTES(sp)
	LREG	x5, 5*REGBYTES(sp)
	LREG	x6, 6*REGBYTES(sp)
	LREG	x7, 7*REGBYTES(sp)
	LREG	x8, 8*REGBYTES(sp)
	LREG	x9, 9*REGBYTES(sp)
	LREG	x10, 10*REGBYTES(sp)
	LREG	x11, 11*REGBYTES(sp)
	LREG	x12, 12*REGBYTES(sp)
	LREG	x13, 13*REGBYTES(sp)
	LREG	x14, 14*REGBYTES(sp)
	LREG	x15, 15*REGBYTES(sp)
	LREG	x16, 16*REGBYTES(sp)
	LREG	x17, 17*REGBYTES(sp)
	LREG	x18, 18*REGBYTES(sp)
	LREG	x19, 19*REGBYTES(sp)
	LREG	x20, 20*REGBYTES(sp)
	LREG	x21, 21*REGBYTES(sp)
	LREG	x22, 22*REGBYTES(sp)
	LREG	x23, 23*REGBYTES(sp)
	LREG	x24, 24*REGBYTES(sp)
	LREG	x25, 25*REGBYTES(sp)
	LREG	x26, 26*REGBYTES(sp)
	LREG	x27, 27*REGBYTES(sp)
	LREG	x28, 28*REGBYTES(sp)
	LREG	x29, 29*REGBYTES(sp)
	LREG	x30, 30*REGBYTES(sp)
	LREG	x31, 31*REGBYTES(sp)
	addi	sp, sp, 35*REGBYTES
	mret

die:
1:
	j	1b

