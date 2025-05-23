/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Leon trap handlers
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__


#include <config.h>
#include "cpu.h"
#include "gaisler/grlib-tn-0018.h"


#define BAD_TRAP \
	mov %psr, %l0; \
	sethi %hi(_exceptions_dispatch), %l3; \
	jmp %l3 + %lo(_exceptions_dispatch); \
	nop;


#define TRAP(handler) \
	mov %psr, %l0; \
	sethi %hi(handler),%l3; \
	jmp %l3 + %lo(handler); \
	nop;


#define INTERRUPT(irqn) \
	mov %psr, %l0; \
	sethi %hi(_interrupts_dispatch),%l3; \
	jmp %l3 + %lo(_interrupts_dispatch); \
	mov (irqn), %l3;


#define RESET \
	mov	%g0, %g4; \
	call _init; \
	nop; \
	nop;


.section ".init", "ax"
.global _start
.type _start, #function
_start:
	call _init
	nop
.size _start, . - _start


/* When trap occurs, traps are disabled: ET <- 0
 * existing supervisor mode is saved in PS <- S
 * supervisor mode is enabled: S <- 1
 * register window is advanced to the next window: CWP <- (CWP - 1) % NWINDOWS
 * without test for window overflow
 * PSR -> %l0, PC -> %l1, nPC -> %l2
 * tt field is written to the particular value that identifies the trap
 *
 * Trap table consists of 256 4-instruction entries.
 *
 * NOTE: the trap table is placed in the .noxip section to avoid XIP during flash programming.
 * This means that we can't trap until it's copied to RAM (which happens in _startc).
 * This is fine as we don't expect to trap before _startc is executed
 * (only one register window change is expected to happen until the copy).
 */

.section ".noxip", "ax"
.global _trap_table
.global _start
_trap_table:
	/* 0x00 - Hardware traps */
.align 0x1000
	RESET                     /* 0x00 - reset */
	BAD_TRAP                  /* 0x01 - instruction access exception */
	BAD_TRAP                  /* 0x02 - illegal instruction */
	BAD_TRAP                  /* 0x03 - privileged instruction */
	BAD_TRAP                  /* 0x04 - fp disabled */
	TRAP(_traps_ovfHandler)   /* 0x05 - window overflow */
	TRAP(_traps_unfHandler)   /* 0x06 - window underflow */
	BAD_TRAP                  /* 0x07 - mem address not aligned */
	BAD_TRAP                  /* 0x08 - fp exception */
	BAD_TRAP                  /* 0x09 - data access exception */
	BAD_TRAP                  /* 0x0a - tag overflow */
	BAD_TRAP                  /* 0x0b - watchpoint detected */
	BAD_TRAP                  /* 0x0c - reserved */
	BAD_TRAP                  /* 0x0d - reserved */
	BAD_TRAP                  /* 0x0e - reserved */
	BAD_TRAP                  /* 0x0f - reserved */
	BAD_TRAP                  /* 0x10 - reserved */

	/* 0x11 - Interrupts */

	INTERRUPT(1)              /* 0x11 - interrupt level 1 */
	INTERRUPT(2)              /* 0x12 - interrupt level 2 */
	INTERRUPT(3)              /* 0x13 - interrupt level 3 */
	INTERRUPT(4)              /* 0x14 - interrupt level 4 */
	INTERRUPT(5)              /* 0x15 - interrupt level 5 */
	INTERRUPT(6)              /* 0x16 - interrupt level 6 */
	INTERRUPT(7)              /* 0x17 - interrupt level 7 */
	INTERRUPT(8)              /* 0x18 - interrupt level 8 */
	INTERRUPT(9)              /* 0x19 - interrupt level 9 */
	INTERRUPT(10)             /* 0x1a - interrupt level 10 */
	INTERRUPT(11)             /* 0x1b - interrupt level 11 */
	INTERRUPT(12)             /* 0x1c - interrupt level 12 */
	INTERRUPT(13)             /* 0x1d - interrupt level 13 */
	INTERRUPT(14)             /* 0x1e - interrupt level 14 */
	INTERRUPT(15)             /* 0x1f - interrupt level 15 */

	/* 0x20 - GR716 - defined traps */

	BAD_TRAP                  /* 0x20 - r register access error */
	BAD_TRAP
	BAD_TRAP
	BAD_TRAP
	BAD_TRAP
	BAD_TRAP
	BAD_TRAP
	BAD_TRAP
	BAD_TRAP
	BAD_TRAP
	BAD_TRAP                  /* 0x2a - division by zero */
#ifdef __CPU_GR716
	BAD_TRAP
#else
	TRAP(_traps_stErrHandler) /* 0x2b - data store error */
#endif
	BAD_TRAP
	BAD_TRAP
	BAD_TRAP
	BAD_TRAP

	/* 0x30 - 0x7F - reserved */

	.rept 80
	BAD_TRAP
	.endr

	/* 0x80 - ABI defined traps */

	BAD_TRAP                  /* 0x80 - syscall */
	BAD_TRAP                  /* 0x81 - breakpoint */
	BAD_TRAP                  /* 0x82 - division by zero */
	BAD_TRAP                  /* 0x83 - flush windows */
	BAD_TRAP                  /* 0x84 - clean windows */
	BAD_TRAP                  /* 0x85 - range check */
	BAD_TRAP                  /* 0x86 - fix alignment */
	BAD_TRAP                  /* 0x87 - integer overflow */
	BAD_TRAP                  /* 0x88 - syscall */
	/* Reserved for OS */
	TRAP(_traps_setPil)       /* 0x89 - set processor int level (reserved for OS) */
	BAD_TRAP                  /* 0x8a - reserved for OS */
	BAD_TRAP                  /* 0x8b - reserved for OS */

	/* 0x8c - 0xFF - reserved for OS */

	.rept 116
	BAD_TRAP
	.endr


.section ".noxip", "ax"
.align 4

#ifndef __CPU_GR716

/* Data store error handler */

.type _traps_stErrHandler, #function
_traps_stErrHandler:
	sta %g0, [%g0] ASI_CCTRL

	TN_0018_WAIT_ICACHE(%l3, %l4)

	/* restore %psr */
	wr %l0, %psr
	nop
	nop
	nop

	TN_0018_FIX(%l3, %l4)

	jmp %l1
	rett %l2
.size _traps_stErrHandler, . - _traps_stErrHandler

#endif

/* Underflow/overflow traps never return directly to 'jmpl' instruction - no TN-0018 fix needed */

/* Window underflow handler
* on entry:
* %psr in %l0, PC in %l1, nPC in %l2
*/

.type _traps_unfHandler, #function
_traps_unfHandler:
	/* calculate new wim
	 * wim = (wim << 1) ^ (wim >> (NWINDOWS - 1))
	 */
	mov %wim, %l3
	sll %l3, 1, %l4
	srl %l3, (NWINDOWS - 1), %l3
	wr %l3, %l4, %wim
	nop
	nop
	nop
	/* go back to window that caused the trap */
	restore
	restore
	ldd [%sp + 0x00], %l0
	ldd [%sp + 0x08], %l2
	ldd [%sp + 0x10], %l4
	ldd [%sp + 0x18], %l6
	ldd [%sp + 0x20], %i0
	ldd [%sp + 0x28], %i2
	ldd [%sp + 0x30], %i4
	ldd [%sp + 0x38], %i6
	save
	save
	jmp %l1
	rett %l2

.size _traps_unfHandler, . - _traps_unfHandler

/* Window overflow handler
 * on entry:
 * %psr in %l0, PC in %l1, nPC in %l2
 */

.type _traps_ovfHandler, #function
_traps_ovfHandler:
	/* move to next window and save it on stack (using d-word store) */
	save
	std %l0, [%sp + 0x00]
	std %l2, [%sp + 0x08]
	std %l4, [%sp + 0x10]
	std %l6, [%sp + 0x18]
	std %i0, [%sp + 0x20]
	std %i2, [%sp + 0x28]
	std %i4, [%sp + 0x30]
	std %i6, [%sp + 0x38]
	/* calculate new wim */
	mov %wim, %l3
	sll	%l3, (NWINDOWS - 1), %l4 /* %l4 = wim << (NWINDOWS - 1) */
	srl %l3, 1, %l3              /* %l3 = wim >> 1 */
	wr  %l3, %l4, %wim           /* %wim = %l3 ^ %l4 */
	nop
	nop
	nop
	restore /* go back to window that caused trap */
	jmp %l1 /* re-execute save that caused trap */
	rett %l2

.size _traps_ovfHandler, . - _traps_ovfHandler

/* Set processor interrupt level
 * on entry:
 * %psr in %l0, PC in %l1, nPC in %l2
 * new PIL in %i0
 */

.type _traps_setPil, #function
_traps_setPil:
	andn %l0, PSR_PIL, %l0
	or %l0, %i0, %l0
	wr %l0, %psr
	nop
	nop
	nop
	/* TN-0018 fix done after 'ta' instruction */
	jmp %l2
	rett %l2 + 4
.size _traps_setPil, . - _traps_setPil

/* BAD_TRAP handler
 * on entry:
 * %psr in %l0, PC in %l1, nPC in %l2
 * must not touch global or out registers until saved on stack
 */

.type _exceptions_dispatch, #function
_exceptions_dispatch:
	/* go back to window that caused the trap */
	restore

	/* 152 bytes needed for exception context */
	sub %sp, 0x98, %sp

	/* save context on stack */
	std %g0, [%sp + 0x00]
	std %g2, [%sp + 0x08]
	std %g4, [%sp + 0x10]
	std %g6, [%sp + 0x18]

	std %o0, [%sp + 0x20]
	std %o2, [%sp + 0x28]
	std %o4, [%sp + 0x30]
	/* sp stored later */
	st  %o7, [%sp + 0x3c]

	std %l0, [%sp + 0x40]
	std %l2, [%sp + 0x48]
	std %l4, [%sp + 0x50]
	std %l6, [%sp + 0x58]

	std %i0, [%sp + 0x60]
	std %i2, [%sp + 0x68]
	std %i4, [%sp + 0x70]
	std %i6, [%sp + 0x78]

	/* save sp */
	add %sp, 0x98, %l0
	st %l0, [%sp + 0x38]

	rd  %y, %g1
	st  %g1, [%sp + 0x80]
	rd  %psr, %g1
	xor %g1, PSR_ET, %g1
	st  %g1, [%sp + 0x84]
	rd  %wim, %g1
	st  %g1, [%sp + 0x88]
	rd  %tbr, %g1
	st  %g1, [%sp + 0x8c]

	save

	/* store pc, npc */
	st %l1, [%i6 + 0x90]
	st %l2, [%i6 + 0x94]

	/* allocate new stack frame for exception handler */
	sub %i6, 0x60, %sp

	/* Enable traps, disable interrupts */
	or %l0, PSR_PIL, %l4
	wr %l4, PSR_ET, %psr
	nop
	nop
	nop

	/* trap type */
	set TBR_TT_MSK, %g2
	and %g1, %g2, %o0
	srl %o0, TBR_TT_SHIFT, %o0

	/* void exceptions_dispatch(unsigned int n, exc_context_t *ctx) */
	call exceptions_dispatch
	mov %i6, %o1
.size _exceptions_dispatch, . - _exceptions_dispatch
