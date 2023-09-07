/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Platform configuration
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_


#ifndef __ASSEMBLY__

#include "peripherals.h"
#include "types.h"
#include "../cpu.h"
#include "../plic.h"
#include "../sbi.h"

#include <phoenix/arch/syspage-riscv64.h>
#include <phoenix/syspage.h>

#define PATH_KERNEL "phoenix-riscv64-generic.elf"

#endif /* __ASSEMBLY__ */


/* Import platform specific definitions */
#include "ld/riscv64-generic.ldt"


#endif
