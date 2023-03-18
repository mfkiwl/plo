/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Timer controller
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>

/* Timer control bitfields */
#define TIMER_ENABLE      (1)
#define TIMER_ONESHOT     (0 << 1)
#define TIMER_PERIODIC    (1 << 1)
#define TIMER_LOAD        (1 << 2)
#define TIMER_INT_ENABLE  (1 << 3)
#define TIMER_INT_PENDING (1 << 4)
#define TIMER_CHAIN       (1 << 5)

#define TIMER_DEFAULT_FREQ 1000


enum {
	gpt_scaler = 0, /* Scaler value register        : 0x00 */
	gpt_sreload,    /* Scaler reload value register : 0x04 */
	gpt_config,     /* Configuration register       : 0x08 */
	gpt_latchcfg,   /* Latch configuration register : 0x0C */
	gpt_tcntval1,   /* Timer 1 counter value reg    : 0x10 */
	gpt_trldval1,   /* Timer 1 reload value reg     : 0x14 */
	gpt_tctrl1,     /* Timer 1 control register     : 0x18 */
	gpt_tlatch1,    /* Timer 1 latch register       : 0x1C */
	gpt_tcntval2,   /* Timer 2 counter value reg    : 0x20 */
	gpt_trldval2,   /* Timer 2 reload value reg     : 0x24 */
	gpt_tctrl2,     /* Timer 2 control register     : 0x28 */
	gpt_tlatch2,    /* Timer 2 latch register       : 0x2C */
};


enum {
	timer1 = 0,
	timer2
};


struct {
	vu32 *gptimer0_base;
	volatile time_t time;
	u32 ticksPerFreq;
} timer_common;


static int timer_isr(unsigned int irq, void *data)
{
	vu32 st = *(timer_common.gptimer0_base + gpt_tctrl1) & TIMER_INT_PENDING;

	if (st != 0) {
		++timer_common.time;
		/* Clear irq status */
		*(timer_common.gptimer0_base + gpt_tctrl1) |= TIMER_INT_PENDING;
	}

	return 0;
}


static void timer_setPrescaler(int timer, u32 freq)
{
	u32 prescaler = SYSCLK_FREQ / 1000000; /* 1 MHz */
	u32 ticks = (SYSCLK_FREQ / prescaler) / freq;

	*(timer_common.gptimer0_base + gpt_trldval1 + timer * 4) = ticks - 1;
	*(timer_common.gptimer0_base + gpt_sreload) = prescaler - 1;

	timer_common.ticksPerFreq = ticks;
}


time_t hal_timerGet(void)
{
	time_t val;

	hal_interruptsDisableAll();
	val = timer_common.time;
	hal_interruptsEnableAll();

	return val;
}


void timer_done(void)
{
	/* Disable timer interrupts - bits cleared when written 1 */
	vu32 st = *(timer_common.gptimer0_base + gpt_tctrl1) & (TIMER_INT_ENABLE | TIMER_INT_PENDING);
	*(timer_common.gptimer0_base + gpt_tctrl1) = st;

	/* Disable timers */
	*(timer_common.gptimer0_base + gpt_tctrl1) = 0;
	*(timer_common.gptimer0_base + gpt_tctrl2) = 0;
	/* Reset counter and reload value */
	*(timer_common.gptimer0_base + gpt_tcntval1) = 0;
	*(timer_common.gptimer0_base + gpt_trldval1) = 0;
	*(timer_common.gptimer0_base + gpt_tcntval2) = 0;
	*(timer_common.gptimer0_base + gpt_trldval2) = 0;

	hal_interruptsSet(TIMER_IRQ, NULL, NULL);
}


void timer_init(void)
{
	timer_common.time = 0;
	timer_common.gptimer0_base = (u32 *)GPTIMER0_BASE;

	/* Reset timer */
	timer_done();

	timer_setPrescaler(timer1, TIMER_DEFAULT_FREQ);

	hal_interruptsSet(TIMER_IRQ, timer_isr, NULL);

	/* Enable timer and interrupts */
	/* Load reload value into counter register */
	*(timer_common.gptimer0_base + gpt_tctrl1) |= TIMER_ENABLE | TIMER_PERIODIC | TIMER_LOAD | TIMER_INT_ENABLE;
}
