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

#include "gr712rc.h"
#include "peripherals.h"
#include "../gaisler.h"
#include "../types.h"
#include "../../cpu.h"

#include <phoenix/arch/syspage-sparcv8leon3.h>
#include <phoenix/syspage.h>

#define PATH_KERNEL "phoenix-sparcv8leon3-gr712rc.elf"

#endif /* __ASSEMBLY__ */


#define NWINDOWS    8
#define SYSCLK_FREQ 50000000 /* Hz */

/* Import platform specific definitions */
#include "ld/sparcv8leon3-gr716.ldt"


#endif
