/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Leon3 Serial driver
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/lib.h>
#include <devices/devs.h>


#define BUFFER_SIZE 0x200

/* UART control bits */
#define RX_FIFO_INT (1 << 10)
#define TX_FIFO_INT (1 << 9)
#define PARITY_EN   (1 << 5)
#define TX_INT      (1 << 3)
#define RX_INT      (1 << 2)
#define TX_EN       (1 << 1)
#define RX_EN       (1 << 0)

/* UART status bits */
#define RX_FIFO_FULL (1 << 10)
#define TX_FIFO_FULL (1 << 9)
#define DATA_READY   (1 << 0)
#define TX_SR_EMPTY  (1 << 1)


/* UART */
enum {
	uart_data,   /* Data register           : 0x00 */
	uart_status, /* Status register         : 0x04 */
	uart_ctrl,   /* Control register        : 0x08 */
	uart_scaler, /* Scaler reload register  : 0x0C */
	uart_dbg     /* FIFO debug register     : 0x10 */
};


typedef struct {
	volatile u32 *base;
	unsigned int irq;
	u16 clk;

	u8 dataRx[BUFFER_SIZE];
	cbuffer_t cbuffRx;
} uart_t;


struct {
	uart_t uarts[UART_MAX_CNT];
} uart_common;


const struct {
	vu32 *base;
	unsigned int irq;
} info[UART_MAX_CNT] = {
	{ UART0_BASE, UART0_IRQ }
};


static inline void uart_rxData(uart_t *uart)
{
	char c;
	/* Keep getting data until rx fifo is not empty */
	while ((*(uart->base + uart_status) & DATA_READY) != 0) {
		c = *(uart->base + uart_data) & 0xff;
		lib_cbufWrite(&uart->cbuffRx, &c, 1);
	}
}


static inline void uart_txData(uart_t *uart, const void *buff, size_t len)
{
	size_t i;
	const char *c = buff;

	for (i = 0; i < len; i++) {
		/* Fill until tx fifo is not full */
		while ((*(uart->base + uart_status) & TX_FIFO_FULL) != 0)
			;
		*(uart->base + uart_data) = c[i];
	}
}


static int uart_irqHandler(unsigned int n, void *data)
{
	uart_t *uart = (uart_t *)data;
	u32 status = *(uart->base + uart_status);

	if ((status & DATA_READY) != 0) {
		uart_rxData(uart);
	}

	return 0;
}

/* From datasheet:
 * appropriate formula to calculate the scaler for desired baudrate,
 * using integer division where the remainder is discarded:
 * scaler = (sysclk_freq)/(baudrate * 8 + 7)
 */
static u16 uart_calcScaler(u32 baud)
{
	u32 scaler = 0;
	u32 sys_clk_freq = SYSCLK_FREQ;

	scaler = (sys_clk_freq / (baud * 8 + 7));

	return (u16)scaler;
}

/* Device interface */

static ssize_t uart_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	ssize_t res;
	uart_t *uart;
	time_t start;

	if (minor >= UART_MAX_CNT) {
		return -EINVAL;
	}

	uart = &uart_common.uarts[minor];
	start = hal_timerGet();
	while (lib_cbufEmpty(&uart->cbuffRx) != 0) {
		if (hal_timerGet() - start > timeout) {
			return -ETIME;
		}
	}
	hal_interruptsDisableAll();
	res = lib_cbufRead(&uart->cbuffRx, buff, len);
	hal_interruptsEnableAll();

	return res;
}


static ssize_t uart_write(unsigned int minor, const void *buff, size_t len)
{
	size_t res;
	uart_t *uart;

	if (minor >= UART_MAX_CNT) {
		return -EINVAL;
	}

	uart = &uart_common.uarts[minor];

	uart_txData(uart, buff, len);

	return res;
}


static ssize_t uart_safeWrite(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	ssize_t res;
	size_t l;

	for (l = 0; l < len; l += res) {
		res = uart_write(minor, buff + l, len - l);
		if (res < 0) {
			return -ENXIO;
		}
	}

	return len;
}


static int uart_sync(unsigned int minor)
{
	uart_t *uart;

	if (minor >= UART_MAX_CNT) {
		return -EINVAL;
	}

	uart = &uart_common.uarts[minor];

	/* Wait until TxFIFO is empty */
	while ((*(uart->base + uart_status) & TX_FIFO_FULL) != 0)
		;

	return EOK;
}


static int uart_done(unsigned int minor)
{
	int res;
	uart_t *uart;

	res = uart_sync(minor);
	if (res < 0) {
		return res;
	}

	uart = &uart_common.uarts[minor];

	/* Disable interrupts, TX & RX */
	*(uart->base + uart_ctrl) &= ~(TX_FIFO_INT | RX_FIFO_INT | TX_INT | RX_INT | TX_EN | RX_EN);

	hal_interruptsSet(uart->irq, NULL, NULL);

	return EOK;
}


static int uart_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	if (minor >= UART_MAX_CNT) {
		return -EINVAL;
	}

	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode) {
		return -EINVAL;
	}

	/* UART is not mappable to any region */
	return dev_isNotMappable;
}


static int uart_init(unsigned int minor)
{
	uart_t *uart;

	if (minor >= UART_MAX_CNT) {
		return -EINVAL;
	}

	uart = &uart_common.uarts[minor];

	uart->base = info[minor].base;
	uart->irq = info[minor].irq;

	lib_cbufInit(&uart->cbuffRx, uart->dataRx, BUFFER_SIZE);

	*(uart->base + uart_scaler) = uart_calcScaler(115200);

	/* UART control - clear everything and: enable FIFO, 1 stop bit,
	 * disable parity, enable TX & RX interrupts, enable TX & RX */
	*(uart->base + uart_ctrl) = TX_FIFO_INT | RX_FIFO_INT | TX_INT | RX_INT | TX_EN | RX_EN;

	hal_interruptsSet(uart->irq, uart_irqHandler, (void *)uart);

	return EOK;
}


__attribute__((constructor)) static void uart_reg(void)
{
	static const dev_handler_t h = {
		.init = uart_init,
		.done = uart_done,
		.read = uart_read,
		.write = uart_safeWrite,
		.erase = NULL,
		.sync = uart_sync,
		.map = uart_map,
	};

	devs_register(DEV_UART, UART_MAX_CNT, &h);
}
