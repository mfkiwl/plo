/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Console
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


void hal_consolePrint(const char *s)
{
	for (; *s != '\0'; s++) {
		sbi_ecall(SBI_PUTCHAR, 0, *s, 0, 0, 0, 0, 0);
	}
}


void console_init(void)
{
}
