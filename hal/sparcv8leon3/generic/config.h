/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Platform configuration
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_


#ifndef __ASSEMBLY__

#include <phoenix/arch/syspage-sparcv8leon3.h>

#include "types.h"
#include "peripherals.h"

#include <phoenix/syspage.h>

#include "../cpu.h"

#endif

#define NWINDOWS    8
#define SYSCLK_FREQ 40000000 /* Hz */

/* Import platform specific definitions */
#include "ld/sparcv8leon3-generic.ldt"


#endif
