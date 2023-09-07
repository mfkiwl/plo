/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * RV64 dummy flash driver
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/errno.h>
#include <devices/devs.h>


#define FLASH_NO    1
#define FLASH_START ((void *)0x20000000)


static int flashdrv_isValidMinor(unsigned int minor)
{
	return (minor < FLASH_NO) ? 1 : 0;
}


/* Device interface */
static ssize_t flashdrv_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	char *memptr;
	ssize_t ret = -EINVAL;

	(void)timeout;

	if (flashdrv_isValidMinor(minor) != 0) {
		memptr = FLASH_START;

		hal_memcpy(buff, memptr + offs, len);
		ret = (ssize_t)len;
	}

	return ret;
}


static ssize_t flashdrv_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	return -ENOSYS;
}


static int flashdrv_done(unsigned int minor)
{
	if (flashdrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	/* Nothing to do */

	return EOK;
}


static int flashdrv_sync(unsigned int minor)
{
	if (flashdrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	/* Nothing to do */

	return EOK;
}


static int flashdrv_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	if (flashdrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	/* Data can be copied from device to map */
	return dev_isNotMappable;
}


static int flashdrv_init(unsigned int minor)
{
	if (flashdrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	return EOK;
}


__attribute__((constructor)) static void flashdrv_reg(void)
{
	static const dev_handler_t h = {
		.init = flashdrv_init,
		.done = flashdrv_done,
		.read = flashdrv_read,
		.write = flashdrv_write,
		.erase = NULL,
		.sync = flashdrv_sync,
		.map = flashdrv_map
	};

	devs_register(DEV_STORAGE, FLASH_NO, &h);
}
