/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Console
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>

/* UART control bits */
#define RX_FIFO_INT (1 << 10)
#define TX_FIFO_INT (1 << 9)
#define PARITY_EN   (1 << 5)
#define TX_INT      (1 << 3)
#define RX_INT      (1 << 2)
#define TX_EN       (1 << 1)
#define RX_EN       (1 << 0)


enum {
	uart_data,   /* Data register           : 0x00 */
	uart_status, /* Status register         : 0x04 */
	uart_ctrl,   /* Control register        : 0x08 */
	uart_scaler, /* Scaler reload register  : 0x0C */
	uart_dbg     /* FIFO debug register     : 0x10 */
};


struct {
	vu32 *uart;
} halconsole_common;


void hal_consolePrint(const char *s)
{
	for (; *s; s++) {
		/* Wait until TX fifo is empty */
		while ((*(halconsole_common.uart + uart_status) & (1 << 9)))
			;
		*(halconsole_common.uart + uart_data) = *s;
	}
}


void console_init(void)
{
	halconsole_common.uart = UART0_BASE;
	*(halconsole_common.uart + uart_ctrl) |= TX_EN;
}
