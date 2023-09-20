/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GR716 specific functions
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "gr712rc.h"
#include "../cpu.h"


#define CGU_BASE0   ((void *)0x80000D00)
#define MCTRL0_BASE ((void *)0x80000000)


/* Clock gating unit */

enum {
	cgu_unlock = 0, /* Unlock register        : 0x00 */
	cgu_clk_en,     /* Clock enable register  : 0x04 */
	cgu_core_reset  /* Core reset register    : 0x08 */
};

/* External PROM & SRAM */

enum {
	mctrl_cfg1 = 0, /* ROM timing and IO    : 0x00 */
	mctrl_cfg2,     /* SRAM timing and IO   : 0x04 */
	mctrl_cfg3,     /* EDAC control         : 0x08 */
	mctrl_cfg5 = 4, /* ROM lead out cycles  : 0x10 */
	mctrl_cfg6,     /* SRAM lead out cycles : 0x14 */
};


static struct {
	vu32 *cgu_base;
	vu32 *mctrl0_base;
} gr712rc_common;


int gaisler_iomuxCfg(iomux_cfg_t *ioCfg)
{
	(void)ioCfg;

	return 0;
}

/* CGU setup - section 28.2 GR716 manual */

void _gr712rc_cguClkEnable(u32 device)
{
	u32 msk = 1 << device;

	*(gr712rc_common.cgu_base + cgu_unlock) |= msk;
	*(gr712rc_common.cgu_base + cgu_core_reset) |= msk;
	*(gr712rc_common.cgu_base + cgu_clk_en) |= msk;
	*(gr712rc_common.cgu_base + cgu_clk_en) &= ~msk;
	*(gr712rc_common.cgu_base + cgu_core_reset) &= ~msk;
	*(gr712rc_common.cgu_base + cgu_clk_en) |= msk;
	*(gr712rc_common.cgu_base + cgu_unlock) &= ~msk;
}


void _gr712rc_cguClkDisable(u32 device)
{
	u32 msk = 1 << device;

	*(gr712rc_common.cgu_base + cgu_unlock) |= msk;
	*(gr712rc_common.cgu_base + cgu_clk_en) &= ~msk;
	*(gr712rc_common.cgu_base + cgu_unlock) &= ~msk;
}


int _gr712rc_cguClkStatus(u32 device)
{
	u32 msk = 1 << device;

	return (*(gr712rc_common.cgu_base + cgu_clk_en) & msk) ? 1 : 0;
}


void _gr712rc_init(void)
{
	gr712rc_common.cgu_base = CGU_BASE0;
	gr712rc_common.mctrl0_base = MCTRL0_BASE;
}
