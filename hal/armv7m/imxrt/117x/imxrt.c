/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * iMXRT 117x basic peripherals control functions
 *
 * Copyright 2017, 2019-2023 Phoenix Systems
 * Author: Aleksander Kaminski, Jan Sikorski, Hubert Buczynski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "imxrt.h"
#include "../../cpu.h"

/* clang-format off */

enum { stk_ctrl = 0, stk_load, stk_val, stk_calib };


enum { aipstz_mpr = 0, aipstz_opacr = 16, aipstz_opacr1, aipstz_opacr2, aipstz_opacr3, aipstz_opacr4 };


enum { gpio_dr = 0, gpio_gdir, gpio_psr, gpio_icr1, gpio_icr2, gpio_imr, gpio_isr, gpio_edge_sel, gpio_dr_set,
	gpio_dr_clear, gpio_dr_toggle };


enum { src_scr = 0, src_srmr, src_sbmr1, src_sbmr2, src_srsr,
	src_gpr1, src_gpr2, src_gpr3, src_gpr4, src_gpr5,
	src_gpr6, src_gpr7, src_gpr8, src_gpr9, src_gpr10,
	src_gpr11, src_gpr12, src_gpr13, src_gpr14, src_gpr15,
	src_gpr16, src_gpr17, src_gpr18, src_gpr19, src_gpr20 };


enum { wdog_wcr = 0, wdog_wsr, wdog_wrsr, wdog_wicr, wdog_wmcr };


enum { rtwdog_cs = 0, rtwdog_cnt, rtwdog_total, rtwdog_win };


/* ANADIG & ANATOP complex */
enum { arm_pll_ctrl = 0x80, sys_pll3_ctrl = 0x84, sys_pll3_update= 0x88, sys_pll3_pfd = 0x8c,
	sys_pll2_ctrl = 0x90, sys_pll2_update = 0x94, sys_pll2_ss = 0x98, sys_pll2_pfd = 0x9c,
	sys_pll2_mfd = 0xa8, sys_pll1_ss = 0xac, sys_pll1_ctrl = 0xb0, sys_pll1_denominator = 0xb4,
	sys_pll1_numerator = 0xb8, sys_pll1_div_select = 0xbc, pll_audio_ctrl = 0xc0, pll_audio_ss = 0xc4,
	pll_audio_denominator = 0xc8, pll_audio_numerator = 0xcc, pll_audio_div_select = 0xd0, pll_video_ctrl = 0xd4,
	pll_video_ss = 0xd8, pll_video_denominator = 0xdc, pll_video_numerator = 0xe0, pll_video_div_select = 0xe4,

	/* PMU */
	pmu_ldo_pll = 0x140, pmu_ref_ctrl = 0x15c,

	/* ANATOP AI */
	vddsoc_ai_ctrl = 0x208, vddsoc_ai_wdata = 0x20c, vddsoc_ai_rdata = 0x210
};


struct {
	volatile u32 *aips[4];
	volatile u32 *stk;
	volatile u32 *src;
	volatile u16 *wdog1;
	volatile u16 *wdog2;
	volatile u32 *wdog3;
	volatile u32 *iomuxc_snvs;
	volatile u32 *iomuxc_lpsr;
	volatile u32 *iomuxc_lpsr_gpr;
	volatile u32 *iomuxc_gpr;
	volatile u32 *iomuxc;
	volatile u32 *gpio[13];
	volatile u32 *ccm;
	volatile u32 *anadig_pll;

	u32 cpuclk;
	u32 cm4state;
} imxrt_common;

/* clang-format on */


/* IOMUX */


__attribute__((section(".noxip"))) static volatile u32 *_imxrt_IOmuxGetReg(int mux)
{
	if (mux < pctl_mux_gpio_emc_b1_00 || mux > pctl_mux_gpio_lpsr_15)
		return NULL;

	if (mux < pctl_mux_wakeup)
		return imxrt_common.iomuxc + 4 + mux - pctl_mux_gpio_emc_b1_00;

	if (mux < pctl_mux_gpio_lpsr_00)
		return imxrt_common.iomuxc_snvs + mux - pctl_mux_wakeup;

	return imxrt_common.iomuxc_lpsr + mux - pctl_mux_gpio_lpsr_00;
}


__attribute__((section(".noxip"))) int _imxrt_setIOmux(int mux, char sion, char mode)
{
	volatile u32 *reg;

	if ((reg = _imxrt_IOmuxGetReg(mux)) == NULL)
		return -1;

	(*reg) = (!!sion << 4) | (mode & 0xf);
	hal_cpuDataMemoryBarrier();

	return 0;
}


__attribute__((section(".noxip"))) static volatile u32 *_imxrt_IOpadGetReg(int pad)
{
	if (pad < pctl_pad_gpio_emc_b1_00 || pad > pctl_pad_gpio_lpsr_15)
		return NULL;

	if (pad < pctl_pad_test_mode)
		return imxrt_common.iomuxc + pad + 149 - pctl_pad_gpio_emc_b1_00;

	if (pad < pctl_pad_gpio_lpsr_00)
		return imxrt_common.iomuxc_snvs + pad + 14 - pctl_pad_test_mode;

	return imxrt_common.iomuxc_lpsr + pad + 16 - pctl_pad_gpio_lpsr_00;
}


__attribute__((section(".noxip"))) int _imxrt_setIOpad(int pad, char sre, char dse, char pue, char pus, char ode, char apc)
{
	u32 t;
	volatile u32 *reg;
	char pull;

	if ((reg = _imxrt_IOpadGetReg(pad)) == NULL)
		return -1;

	if (pad >= pctl_pad_gpio_emc_b1_00 && pad <= pctl_pad_gpio_disp_b2_15) {
		/* Fields have slightly diffrent meaning... */
		if (!pue)
			pull = 3;
		else if (pus)
			pull = 1;
		else
			pull = 2;

		t = *reg & ~0x1e;
		t |= (!!dse << 1) | (pull << 2) | (!!ode << 4);
	}
	else {
		t = *reg & ~0x1f;
		t |= (!!sre) | (!!dse << 1) | (!!pue << 2) | (!!pus << 3);

		if (pad >= pctl_pad_test_mode && pad <= pctl_pad_gpio_snvs_09) {
			t &= ~(1 << 6);
			t |= !!ode << 6;
		}
		else {
			t &= ~(1 << 5);
			t |= !!ode << 5;
		}
	}

	/* APC field is not documented. Leave it alone for now. */
	//t &= ~(0xf << 28);
	//t |= (apc & 0xf) << 28;

	(*reg) = t;
	hal_cpuDataMemoryBarrier();

	return 0;
}


__attribute__((section(".noxip"))) static volatile u32 *_imxrt_IOiselGetReg(int isel, u32 *mask)
{
	if (isel < pctl_isel_flexcan1_rx || isel > pctl_isel_sai4_txsync)
		return NULL;

	switch (isel) {
		case pctl_isel_flexcan1_rx:
		case pctl_isel_ccm_enet_qos_ref_clk:
		case pctl_isel_enet_ipg_clk_rmii:
		case pctl_isel_enet_1g_ipg_clk_rmii:
		case pctl_isel_enet_1g_mac0_mdio:
		case pctl_isel_enet_1g_mac0_rxclk:
		case pctl_isel_enet_1g_mac0_rxdata_0:
		case pctl_isel_enet_1g_mac0_rxdata_1:
		case pctl_isel_enet_1g_mac0_rxdata_2:
		case pctl_isel_enet_1g_mac0_rxdata_3:
		case pctl_isel_enet_1g_mac0_rxen:
		case enet_qos_phy_rxer:
		case pctl_isel_flexspi1_dqs_fa:
		case pctl_isel_lpuart1_rxd:
		case pctl_isel_lpuart1_txd:
		case pctl_isel_qtimer1_tmr0:
		case pctl_isel_qtimer1_tmr1:
		case pctl_isel_qtimer2_tmr0:
		case pctl_isel_qtimer2_tmr1:
		case pctl_isel_qtimer3_tmr0:
		case pctl_isel_qtimer3_tmr1:
		case pctl_isel_qtimer4_tmr0:
		case pctl_isel_qtimer4_tmr1:
		case pctl_isel_sdio_slv_clk_sd:
		case pctl_isel_sdio_slv_cmd_di:
		case pctl_isel_sdio_slv_dat0_do:
		case pctl_isel_slv_dat1_irq:
		case pctl_isel_sdio_slv_dat2_rw:
		case pctl_isel_sdio_slv_dat3_cs:
		case pctl_isel_spdif_in1:
		case pctl_isel_can3_canrx:
		case pctl_isel_lpuart12_rxd:
		case pctl_isel_lpuart12_txd:
			(*mask) = 0x3;
			break;

		default:
			(*mask) = 0x1;
			break;
	}

	if (isel >= pctl_isel_can3_canrx)
		return imxrt_common.iomuxc_lpsr + 32 + isel - pctl_isel_can3_canrx;

	return imxrt_common.iomuxc + 294 + isel - pctl_isel_flexcan1_rx;
}


__attribute__((section(".noxip"))) int _imxrt_setIOisel(int isel, char daisy)
{
	volatile u32 *reg;
	u32 mask;

	if ((reg = _imxrt_IOiselGetReg(isel, &mask)) == NULL)
		return -1;

	(*reg) = daisy & mask;
	hal_cpuDataMemoryBarrier();

	return 0;
}


/* GPIO */


static volatile u32 *_imxrt_gpioGetReg(unsigned int d)
{
	return (d < sizeof(imxrt_common.gpio) / sizeof(imxrt_common.gpio[0])) ? imxrt_common.gpio[d] : NULL;
}


int _imxrt_gpioConfig(unsigned int d, u8 pin, u8 dir)
{
	volatile u32 *reg = _imxrt_gpioGetReg(d);
	u32 register clr;

	if ((reg == NULL) || (pin > 31u)) {
		return -1;
	}

	clr = *(reg + gpio_gdir) & ~(1u << pin);
	dir = (dir != 0u) ? 1u : 0u;
	*(reg + gpio_gdir) = clr | (dir << pin);

	return 0;
}


int _imxrt_gpioSet(unsigned int d, u8 pin, u8 val)
{
	volatile u32 *reg = _imxrt_gpioGetReg(d);
	u32 register clr;

	if ((reg == NULL) || (pin > 31u)) {
		return -1;
	}

	clr = *(reg + gpio_dr) & ~(1u << pin);
	val = (val != 0u) ? 1u : 0u;
	*(reg + gpio_dr) = clr | (val << pin);

	return 0;
}


int _imxrt_gpioSetPort(unsigned int d, u32 val)
{
	volatile u32 *reg = _imxrt_gpioGetReg(d);

	if (reg == NULL) {
		return -1;
	}

	*(reg + gpio_dr) = val;

	return 0;
}


int _imxrt_gpioGet(unsigned int d, u8 pin, u8 *val)
{
	volatile u32 *reg = _imxrt_gpioGetReg(d);

	if ((reg == NULL) || (pin > 31)) {
		return -1;
	}

	*val = ((*(reg + gpio_psr) & (1u << pin)) != 0u) ? 1u : 0u;

	return 0;
}


int _imxrt_gpioGetPort(unsigned int d, u32 *val)
{
	volatile u32 *reg = _imxrt_gpioGetReg(d);

	if (reg == NULL) {
		return -1;
	}

	*val = *(reg + gpio_psr);

	return 0;
}


/* CCM */

__attribute__((section(".noxip"))) int _imxrt_getDevClock(int clock, int *div, int *mux, int *mfd, int *mfn, int *state)
{
	unsigned int t;
	volatile u32 *reg = imxrt_common.ccm + (clock * 0x20);

	if (clock < pctl_clk_cm7 || clock > pctl_clk_ccm_clko2)
		return -1;

	t = *reg;

	*div = t & 0xff;
	*mux = (t >> 8) & 0x7;
	*mfd = (t >> 16) & 0xf;
	*mfn = (t >> 20) & 0xf;
	*state = !(t & (1 << 24));

	return 0;
}

__attribute__((section(".noxip"))) int _imxrt_setDevClock(int clock, int div, int mux, int mfd, int mfn, int state)
{
	unsigned int t;
	volatile u32 *reg = imxrt_common.ccm + (clock * 0x20);

	if (clock < pctl_clk_cm7 || clock > pctl_clk_ccm_clko2)
		return -1;

	t = *reg & ~0x01ff07ffu;
	*reg = t | (!state << 24) | ((mfn & 0xf) << 20) | ((mfd & 0xf) << 16) | ((mux & 0x7) << 8) | (div & 0xff);

	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();

	return 0;
}


__attribute__((section(".noxip"))) int _imxrt_setDirectLPCG(int clock, int state)
{
	u32 t;
	volatile u32 *reg;

	if (clock < pctl_lpcg_m7 || clock > pctl_lpcg_uniq_edt_i)
		return -1;

	reg = imxrt_common.ccm + 0x1800 + clock * 0x8;

	t = *reg & ~1u;
	*reg = t | (state & 1);

	hal_cpuDataMemoryBarrier();
	hal_cpuInstrBarrier();

	return 0;
}


__attribute__((section(".noxip"))) int _imxrt_setLevelLPCG(int clock, int level)
{
	volatile u32 *reg;

	if (clock < pctl_lpcg_m7 || clock > pctl_lpcg_uniq_edt_i)
		return -1;

	if (level < 0 || level > 4)
		return -1;

	reg = imxrt_common.ccm + 0x1801 + clock * 0x8;
	*reg = (level << 28) | (level << 24) | (level << 20) | (level << 16) | level;

	hal_cpuDataMemoryBarrier();
	hal_cpuInstrBarrier();

	return 0;
}


__attribute__((section(".noxip"))) void _imxrt_setRootClock(u8 root, u8 mux, u8 div, u8 clockoff)
{
	*(imxrt_common.ccm + 0x20u * (u32)root) = ((clockoff != 0) ? (1uL << 24u) : 0u) |
		(((u32)mux & 7u) << 8u) | (((u32)div - 1u) & 0xffu);
	hal_cpuDataMemoryBarrier();
	hal_cpuInstrBarrier();
}


static void _imxrt_pmuEnablePllLdo(void)
{
	u32 val;

	/* Set address of PHY_LDO_CTRL0 */
	val = *(imxrt_common.anadig_pll + vddsoc_ai_ctrl);
	*(imxrt_common.anadig_pll + vddsoc_ai_ctrl) = val | (1uL << 16u);

	val = *(imxrt_common.anadig_pll + vddsoc_ai_ctrl) & ~(0xffu);
	*(imxrt_common.anadig_pll + vddsoc_ai_ctrl) = val | (0); /* PHY_LDO_CTRL0 = 0 */

	/* Toggle ldo PLL AI */
	*(imxrt_common.anadig_pll + pmu_ldo_pll) ^= 1uL << 16u;
	/* Read data */
	val = *(imxrt_common.anadig_pll + vddsoc_ai_rdata);

	if (val == ((0x10uL << 4u) | (1u << 2u) | 1u)) {
		/* Already set PHY_LDO_CTRL0 LDO */
		return;
	}

	val = *(imxrt_common.anadig_pll + vddsoc_ai_ctrl);
	*(imxrt_common.anadig_pll + vddsoc_ai_ctrl) = val & ~(1uL << 16u);

	val = *(imxrt_common.anadig_pll + vddsoc_ai_ctrl) & ~(0xffu);
	*(imxrt_common.anadig_pll + vddsoc_ai_ctrl) = val | (0); /* PHY_LDO_CTRL0 = 0 */

	/* Write data */
	*(imxrt_common.anadig_pll + vddsoc_ai_wdata) = (0x10u << 4u) | (1u << 2u) | 1u;
	/* Toggle ldo PLL AI */
	*(imxrt_common.anadig_pll + pmu_ldo_pll) ^= 1uL << 16u;

	/* Busy wait ~3us */
	for (unsigned i = 0; i < 0x10000u; i++) {
		asm volatile("nop");
	}

	/* Enable Voltage Reference for PLLs before those PLLs were enabled */
	*(imxrt_common.anadig_pll + pmu_ref_ctrl) = 1u << 4u;
}


void _imxrt_initArmPll(u8 loopDivider, u8 postDivider)
{
	u32 m7clkroot, reg;

	if ((loopDivider <= 104u) || (208u <= loopDivider)) {
		return;
	}

	/* Save current M7 clock root */
	m7clkroot = *(imxrt_common.ccm + 0x20u * clkroot_m7);

	/* Temporarily switch M7 clock trusted clock source if not already */
	if (((m7clkroot >> 8u) & 0xffu) != mux_clkroot_m7_oscrc400m) {
		_imxrt_setRootClock(clkroot_m7, mux_clkroot_m7_oscrc400m, 1, 0);
	}

	_imxrt_pmuEnablePllLdo();

	reg = *(imxrt_common.anadig_pll + arm_pll_ctrl) & ~(1uL << 29u);

	/* Disable and gate clock if not already */
	if ((reg & ((1uL << 13u) | (1uL << 14u))) != 0u) {
		/* Power down the PLL, disable clock */
		reg &= ~((1uL << 13u) | (1uL << 14u));
		/* Gate the clock */
		reg |= 1uL << 30u;
		*(imxrt_common.anadig_pll + arm_pll_ctrl) = reg;
	}

	/* Set the configuration. */
	reg &= ~((3uL << 15u) | 0xffu);
	reg |= ((loopDivider & 0xffu) | ((postDivider & 3uL) << 15u)) | (1uL << 30u) | (1uL << 13u);
	*(imxrt_common.anadig_pll + arm_pll_ctrl) = reg;

	hal_cpuDataMemoryBarrier();
	hal_cpuInstrBarrier();

	/* Wait for stable PLL */
	while ((*(imxrt_common.anadig_pll + arm_pll_ctrl) & (1uL << 29u)) == 0u) {
	}

	/* Enable the clock. */
	reg |= 1uL << 14u;

	/* Ungate the clock */
	reg &= ~(1uL << 30u);

	*(imxrt_common.anadig_pll + arm_pll_ctrl) = reg;

	/* Restore previous M7 clock mux */
	*(imxrt_common.ccm + 0x20u * clkroot_m7) = m7clkroot;
}


void _imxrt_initClockTree(void)
{
	static const struct {
		u8 root;
		u8 mux;
		u8 div;
		u8 clkoff;
	} clktree[] = {
		{ clkroot_m7, mux_clkroot_m7_armpllout, 1, 0 },
		/* { clkroot_m4, 1, mux_clkroot_m4_syspll3pfd3, 0 }, */
		{ clkroot_bus, mux_clkroot_bus_syspll3out, 2, 0 },
		{ clkroot_bus_lpsr, mux_clkroot_bus_lpsr_syspll3out, 3, 0 },
		/* { clkroot_semc, mux_clkroot_semc_syspll2pfd1, 3, 0 }, */
		{ clkroot_cssys, mux_clkroot_cssys_oscrc48mdiv2, 1, 0 },
		{ clkroot_cstrace, mux_clkroot_cstrace_syspll2out, 4, 0 },
		{ clkroot_m4_systick, mux_clkroot_m4_systick_oscrc48mdiv2, 1, 0 },
		{ clkroot_m7_systick, mux_clkroot_m7_systick_oscrc48mdiv2, 2, 0 },
		{ clkroot_adc1, mux_clkroot_adc1_oscrc48mdiv2, 1, 0 },
		{ clkroot_adc2, mux_clkroot_adc2_oscrc48mdiv2, 1, 0 },
		{ clkroot_acmp, mux_clkroot_acmp_oscrc48mdiv2, 1, 0 },
		{ clkroot_flexio1, mux_clkroot_flexio1_oscrc48mdiv2, 1, 0 },
		{ clkroot_flexio2, mux_clkroot_flexio2_oscrc48mdiv2, 1, 0 },
		{ clkroot_gpt1, mux_clkroot_gpt1_oscrc48mdiv2, 1, 0 },
		{ clkroot_gpt2, mux_clkroot_gpt2_oscrc48mdiv2, 1, 0 },
		{ clkroot_gpt3, mux_clkroot_gpt3_oscrc48mdiv2, 1, 0 },
		{ clkroot_gpt4, mux_clkroot_gpt4_oscrc48mdiv2, 1, 0 },
		{ clkroot_gpt5, mux_clkroot_gpt5_oscrc48mdiv2, 1, 0 },
		{ clkroot_gpt6, mux_clkroot_gpt6_oscrc48mdiv2, 1, 0 },
		/* { clkroot_flexspi1, mux_clkroot_flexspi1_oscrc48mdiv2, 1, 0 }, */
		/* { clkroot_flexspi2, mux_clkroot_flexspi2_oscrc48mdiv2, 1, 0 }, */
		{ clkroot_can1, mux_clkroot_can1_oscrc48mdiv2, 1, 0 },
		{ clkroot_can2, mux_clkroot_can2_oscrc48mdiv2, 1, 0 },
		{ clkroot_can3, mux_clkroot_can3_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpuart1, mux_clkroot_lpuart1_syspll2out, 2, 0 },
		{ clkroot_lpuart2, mux_clkroot_lpuart2_syspll2out, 2, 0 },
		{ clkroot_lpuart3, mux_clkroot_lpuart3_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpuart4, mux_clkroot_lpuart4_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpuart5, mux_clkroot_lpuart5_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpuart6, mux_clkroot_lpuart6_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpuart7, mux_clkroot_lpuart7_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpuart8, mux_clkroot_lpuart8_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpuart9, mux_clkroot_lpuart9_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpuart10, mux_clkroot_lpuart10_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpuart11, mux_clkroot_lpuart11_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpuart12, mux_clkroot_lpuart12_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpi2c1, mux_clkroot_lpi2c1_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpi2c2, mux_clkroot_lpi2c2_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpi2c3, mux_clkroot_lpi2c3_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpi2c4, mux_clkroot_lpi2c4_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpi2c5, mux_clkroot_lpi2c5_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpi2c6, mux_clkroot_lpi2c6_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpspi1, mux_clkroot_lpspi1_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpspi2, mux_clkroot_lpspi2_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpspi3, mux_clkroot_lpspi3_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpspi4, mux_clkroot_lpspi4_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpspi5, mux_clkroot_lpspi5_oscrc48mdiv2, 1, 0 },
		{ clkroot_lpspi6, mux_clkroot_lpspi6_oscrc48mdiv2, 1, 0 },
		{ clkroot_emv1, mux_clkroot_emv1_oscrc48mdiv2, 1, 0 },
		{ clkroot_emv2, mux_clkroot_emv2_oscrc48mdiv2, 1, 0 },
		{ clkroot_enet1, mux_clkroot_enet1_oscrc48mdiv2, 1, 0 },
		{ clkroot_enet2, mux_clkroot_enet2_oscrc48mdiv2, 1, 0 },
		{ clkroot_enet_qos, mux_clkroot_enet_qos_oscrc48mdiv2, 1, 0 },
		{ clkroot_enet_25m, mux_clkroot_enet_25m_oscrc48mdiv2, 1, 0 },
		{ clkroot_enet_timer1, mux_clkroot_enet_timer1_oscrc48mdiv2, 1, 0 },
		{ clkroot_enet_timer2, mux_clkroot_enet_timer2_oscrc48mdiv2, 1, 0 },
		{ clkroot_enet_timer3, mux_clkroot_enet_timer3_oscrc48mdiv2, 1, 0 },
		{ clkroot_usdhc1, mux_clkroot_usdhc1_oscrc48mdiv2, 1, 0 },
		{ clkroot_usdhc2, mux_clkroot_usdhc2_oscrc48mdiv2, 1, 0 },
		{ clkroot_asrc, mux_clkroot_asrc_oscrc48mdiv2, 1, 0 },
		{ clkroot_mqs, mux_clkroot_mqs_oscrc48mdiv2, 1, 0 },
		{ clkroot_mic, mux_clkroot_mic_oscrc48mdiv2, 1, 0 },
		{ clkroot_spdif, mux_clkroot_spdif_oscrc48mdiv2, 1, 0 },
		{ clkroot_sai1, mux_clkroot_sai1_oscrc48mdiv2, 1, 0 },
		{ clkroot_sai2, mux_clkroot_sai2_oscrc48mdiv2, 1, 0 },
		{ clkroot_sai3, mux_clkroot_sai3_oscrc48mdiv2, 1, 0 },
		{ clkroot_sai4, mux_clkroot_sai4_oscrc48mdiv2, 1, 0 },
		{ clkroot_gc355, mux_clkroot_gc355_videopllout, 2, 0 },
		{ clkroot_lcdif, mux_clkroot_lcdif_oscrc48mdiv2, 1, 0 },
		{ clkroot_lcdifv2, mux_clkroot_lcdifv2_oscrc48mdiv2, 1, 0 },
		{ clkroot_mipi_ref, mux_clkroot_mipi_ref_oscrc48mdiv2, 1, 0 },
		{ clkroot_mipi_esc, mux_clkroot_mipi_esc_oscrc48mdiv2, 1, 0 },
		{ clkroot_csi2, mux_clkroot_csi2_oscrc48mdiv2, 1, 0 },
		{ clkroot_csi2_esc, mux_clkroot_csi2_esc_oscrc48mdiv2, 1, 0 },
		{ clkroot_csi2_ui, mux_clkroot_csi2_ui_oscrc48mdiv2, 1, 0 },
		{ clkroot_csi, mux_clkroot_csi_oscrc48mdiv2, 1, 0 },
		{ clkroot_cko1, mux_clkroot_cko1_oscrc48mdiv2, 1, 0 },
		{ clkroot_cko2, mux_clkroot_cko2_oscrc48mdiv2, 1, 0 },
	};

	for (unsigned n = 0; n < sizeof(clktree) / sizeof(clktree[0]); n++) {
		_imxrt_setRootClock(clktree[n].root, clktree[n].mux, clktree[n].div, clktree[n].clkoff);
	}
}


/* CM4 */


int _imxrt_setVtorCM4(int dwpLock, int dwp, addr_t vtor)
{
	u32 tmp;

	/* is CM4 running already ? */
	if ((imxrt_common.cm4state & 1u) != 0u) {
		return -1;
	}

	tmp = *(imxrt_common.iomuxc_lpsr_gpr + 0u);
	tmp |= *(imxrt_common.iomuxc_lpsr_gpr + 1u);

	/* is DWP locked or CM7 forbidden ? */
	if ((tmp & (0xdu << 28)) != 0u) {
		return -1;
	}

	tmp = ((((u32)dwpLock & 3u) << 30) | (((u32)dwp & 3u) << 28));

	*(imxrt_common.iomuxc_lpsr_gpr + 0u) = (tmp | (vtor & 0xfff8u));
	*(imxrt_common.iomuxc_lpsr_gpr + 1u) = (tmp | ((vtor >> 16) & 0xffffu));

	hal_cpuDataMemoryBarrier();

	imxrt_common.cm4state |= 2u;

	return 0;
}


void _imxrt_runCM4(void)
{
	/* CM7 is allowed to reset system, CM4 is disallowed */
	*(imxrt_common.src + src_srmr) |= ((3u << 10) | (3u << 6));
	hal_cpuDataMemoryBarrier();

	/* Release CM4 reset */
	*(imxrt_common.src + src_scr) |= 1u;
	hal_cpuDataSyncBarrier();

	imxrt_common.cm4state |= 1u;
}


u32 _imxrt_getStateCM4(void)
{
	return imxrt_common.cm4state;
}


void _imxrt_init(void)
{
	int i;
	volatile u32 *reg;

	imxrt_common.aips[0] = (void *)0x40000000;
	imxrt_common.aips[1] = (void *)0x40400000;
	imxrt_common.aips[2] = (void *)0x40800000;
	imxrt_common.aips[3] = (void *)0x40c00000;
	imxrt_common.stk = (void *)0xe000e010;
	imxrt_common.wdog1 = (void *)0x40030000;
	imxrt_common.wdog2 = (void *)0x40034000;
	imxrt_common.wdog3 = (void *)0x40038000;
	imxrt_common.src = (void *)0x40c04000;
	imxrt_common.iomuxc_snvs = (void *)0x40c94000;
	imxrt_common.iomuxc_lpsr = (void *)0x40c08000;
	imxrt_common.iomuxc_lpsr_gpr = (void *)0x40c0c000;
	imxrt_common.iomuxc_gpr = (void *)0x400e4000;
	imxrt_common.iomuxc = (void *)0x400e8000;
	imxrt_common.ccm = (void *)0x40cc0000;
	imxrt_common.anadig_pll = (void *)0x40c84000;

	imxrt_common.gpio[0] = (void *)0x4012c000;
	imxrt_common.gpio[1] = (void *)0x40130000;
	imxrt_common.gpio[2] = (void *)0x40134000;
	imxrt_common.gpio[3] = (void *)0x40138000;
	imxrt_common.gpio[4] = (void *)0x4013c000;
	imxrt_common.gpio[5] = (void *)0x40140000;
	imxrt_common.gpio[6] = (void *)0x40c5c000;
	imxrt_common.gpio[7] = (void *)0x40c60000;
	imxrt_common.gpio[8] = (void *)0x40c64000;
	imxrt_common.gpio[9] = (void *)0x40c68000;
	imxrt_common.gpio[10] = (void *)0x40c6c000;
	imxrt_common.gpio[11] = (void *)0x40c70000;
	imxrt_common.gpio[12] = (void *)0x40ca0000;

	imxrt_common.cpuclk = 640000000;

	imxrt_common.cm4state = (*(imxrt_common.src + src_scr) & 1u);

	/* Initialize ARM PLL to 996 MHz */
	_imxrt_initArmPll(166, 0);

	/* Module clock root configurations */
	_imxrt_initClockTree();

	/* Disable watchdogs */
	if (*(imxrt_common.wdog1 + wdog_wcr) & (1 << 2))
		*(imxrt_common.wdog1 + wdog_wcr) &= ~(1 << 2);
	if (*(imxrt_common.wdog2 + wdog_wcr) & (1 << 2))
		*(imxrt_common.wdog2 + wdog_wcr) &= ~(1 << 2);

	*(imxrt_common.wdog3 + rtwdog_cnt) = 0xd928c520; /* Update key */
	*(imxrt_common.wdog3 + rtwdog_total) = 0xffff;
	*(imxrt_common.wdog3 + rtwdog_cs) |= 1 << 5;
	*(imxrt_common.wdog3 + rtwdog_cs) &= ~(1 << 7);

	/* Disable Systick which might be enabled by bootrom */
	if (*(imxrt_common.stk + stk_ctrl) & 1)
		*(imxrt_common.stk + stk_ctrl) &= ~1;

	/* HACK - temporary fix for crash when booting from FlexSPI */
	_imxrt_setDirectLPCG(pctl_lpcg_semc, 0);

	/* Disable USB cache (set by bootrom) */
	*(imxrt_common.iomuxc_gpr + 28) &= ~(1 << 13);
	hal_cpuDataMemoryBarrier();

	/* Clear SRSR */
	*(imxrt_common.src + src_srsr) = 0xffffffffu;

	/* Reconfigure all IO pads as slow slew-rate and low drive strength */
	for (i = pctl_pad_gpio_emc_b1_00; i <= pctl_pad_gpio_disp_b2_15; ++i) {
		if ((reg = _imxrt_IOpadGetReg(i)) == NULL)
			continue;

		*reg |= 1 << 1;
		hal_cpuDataMemoryBarrier();
	}

	for (; i <= pctl_pad_gpio_lpsr_15; ++i) {
		if ((reg = _imxrt_IOpadGetReg(i)) == NULL)
			continue;

		*reg &= ~0x3;
		hal_cpuDataMemoryBarrier();
	}
}
