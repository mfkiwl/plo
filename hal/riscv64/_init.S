/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Low level initialization
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__

#include "cpu.h"
#define SR_SUM          0x00040000
#define SR_MER          0x00080000

.align 8

.section .init, "ax"

.globl dtbAddr
dtbAddr:
	.dword

.globl _start
.type _start, @function
_start:
	la t0, dtbAddr
	sd a1, (t0)
	/* Mask all interrupts */
	csrw sie, zero

.option push
.option norelax
	la gp, __global_pointer$
.option pop

	/* Disable FPU */
	li t0, SSTATUS_FS
	csrc sstatus, t0

	/* Allow supervisor access to user space and reading from execute pages (for the time being) */
	li t0, SR_SUM | SR_MER
	csrs sstatus, t0

	la sp, _stack
	j _startc
.size _start, . - _start
