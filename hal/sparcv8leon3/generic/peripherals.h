/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Peripherals definitions for Leon3 QEMU
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_

/* UART */
#define UART_MAX_CNT 1
#define UART0_BASE   ((void *)0x80000100)
#define UART0_IRQ    3

/* Interrupts */
#define INT_CTRL_BASE ((void *)0x80000200)

/* Timers */
#define GPTIMER0_BASE ((void *)0x80000300)
#define TIMER_IRQ     6

#endif /* _PERIPHERALS_H_ */
