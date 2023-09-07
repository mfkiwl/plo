/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Interrupt handling
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


#define STR(x)  #x
#define XSTR(x) STR(x)

/* Hardware interrupts */
#define SIZE_INTERRUPTS 64

#define EXT_IRQ 9

typedef struct {
	int (*isr)(unsigned int, void *);
	void *data;
} irq_handler_t;


static struct {
	vu32 *int_ctrl;
	irq_handler_t handlers[SIZE_INTERRUPTS];
} interrupts_common;


void hal_interruptsEnable(unsigned int irqn)
{
	csr_set(sie, 1 << irqn);
}


void hal_interruptsDisable(unsigned int irqn)
{
	csr_clear(sie, 1 << irqn);
}


void hal_interruptsEnableAll(void)
{
	csr_set(sstatus, SSTATUS_SIE);
}


void hal_interruptsDisableAll(void)
{
	csr_clear(sstatus, SSTATUS_SIE);
}


int interrupts_dispatch(unsigned int irq)
{
	unsigned int cn = irq;

	if (irq == EXT_IRQ) {
		cn = plic_claim(1);
		if (cn == 0) {
			return 0;
		}
	}

	if (interrupts_common.handlers[cn].isr == NULL) {
		return -1;
	}
	interrupts_common.handlers[cn].isr(cn, interrupts_common.handlers[cn].data);

	if (irq == EXT_IRQ) {
		plic_complete(1, cn);
	}

	return 0;
}


int hal_interruptsSet(unsigned int n, int (*isr)(unsigned int, void *), void *data)
{
	if (n >= SIZE_INTERRUPTS || n == 0) {
		return -1;
	}

	hal_interruptsDisableAll();
	interrupts_common.handlers[n].data = data;
	interrupts_common.handlers[n].isr = isr;

	if (isr == NULL) {
		hal_interruptsDisable(n);
	}
	else {
		hal_interruptsEnable(n);
		plic_priority(n, 2);
		plic_enableInterrupt(1, n, 1);
	}

	hal_interruptsEnableAll();

	return 0;
}


extern void _interrupts_dispatch(void);


void interrupts_init(void)
{
	for (int i = 0; i < SIZE_INTERRUPTS; ++i) {
		interrupts_common.handlers[i].isr = NULL;
		interrupts_common.handlers[i].data = NULL;
	}

	csr_write(sscratch, 0);
	csr_write(sie, -1);
	csr_write(stvec, _interrupts_dispatch);

	_plic_init();
}
