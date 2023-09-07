/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Timer controller
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


#define TIMER_IRQ 5


static struct {
	volatile time_t jiffies;
	u32 interval;
} timer_common;


static int timer_irqHandler(unsigned int n, void *arg)
{
	(void)n;
	(void)arg;

	timer_common.jiffies += timer_common.interval;
	sbi_ecall(SBI_SETTIMER, 0, csr_read(time) + timer_common.interval, 0, 0, 0, 0, 0);
	return 0;
}


time_t hal_timerGet(void)
{
	time_t val;

	hal_interruptsDisable(TIMER_IRQ);
	val = timer_common.jiffies;
	hal_interruptsEnable(TIMER_IRQ);

	return val;
}


void timer_init(void)
{
	timer_common.interval = 1000 * 1000; /* 1ms */
	timer_common.jiffies = 0;

	hal_interruptsSet(TIMER_IRQ, timer_irqHandler, NULL);
	sbi_ecall(SBI_SETTIMER, 0, csr_read(time) + timer_common.interval, 0, 0, 0, 0, 0);
}
