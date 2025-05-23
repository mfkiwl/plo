/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Serial console
 *
 * Copyright 2017, 2021 Phoenix Systems
 * Author: Michał Mirosław, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include "console.h"


/* UART registers */
enum {
	rbr = 0, /* Receiver Buffer Register */
	thr = 0, /* Transmitter Holding Register */
	dll = 0, /* Divisor Latch LSB */
	ier = 1, /* Interrupt Enable Register */
	dlm = 1, /* Divisor Latch MSB */
	iir = 2, /* Interrupt Identification Register */
	fcr = 2, /* FIFO Control Register */
	lcr = 3, /* Line Control Register */
	mcr = 4, /* Modem Control Register */
	lsr = 5, /* Line Status Register */
	msr = 6, /* Modem Status Register */
	spr = 7, /* Scratch Pad Register */
};


static struct {
	volatile u8 *base;
	ssize_t (*writeHook)(int, const void *, size_t);
} halconsole_common;


static unsigned char console_read(unsigned int reg)
{
	unsigned int addr = (unsigned int)halconsole_common.base;

	/* Read from IO-port */
	if (addr & 0x1)
		return hal_inb((u16)((addr & ~0x3) + reg));

	/* Read from memory */
	return *(halconsole_common.base + reg);
}


static void console_write(unsigned int reg, unsigned char val)
{
	unsigned int addr = (unsigned int)halconsole_common.base;

	/* Write to IO-port */
	if (addr & 0x1) {
		hal_outb((u16)((addr & ~0x3) + reg), val);
		return;
	}

	/* Write to memory */
	*(halconsole_common.base + reg) = val;
}


static void hal_consoleSerialSetHooks(ssize_t (*writeHook)(int, const void *, size_t))
{
	halconsole_common.writeHook = writeHook;
}


static void hal_consoleSerialPrint(const char *s)
{
	const char *ptr;

	for (ptr = s; *ptr != '\0'; ++ptr) {
		/* Wait until TX fifo is empty */
		while ((console_read(lsr) & 0x20) == 0) {
		}
		console_write(thr, *ptr);
	}

	if (halconsole_common.writeHook != NULL) {
		(void)halconsole_common.writeHook(0, s, ptr - s);
	}
}


static void hal_consoleSerialInit(void)
{
	unsigned int divisor = 115200 / UART_BAUDRATE;

	halconsole_common.base = (void *)(UART_COM1 | 0x1);

	/* Set speed */
	console_write(lcr, 0x80);
	console_write(dll, divisor);
	console_write(dlm, divisor >> 8);

	/* Set data format */
	console_write(lcr, 0x03);

	/* Set DTR and RTS */
	console_write(mcr, 0x03);

	/* Enable and configure FIFOs */
	console_write(fcr, 0xa7);

	/* Disable interrupts */
	console_write(ier, 0x00);
}


static struct halconsole serial_console;


__attribute__((constructor)) static void hal_consoleVgaRegister(void)
{
	serial_console.setHooks = hal_consoleSerialSetHooks;
	serial_console.print = hal_consoleSerialPrint;
	serial_console.init = hal_consoleSerialInit;

	hal_consoleRegister(&serial_console);
}
