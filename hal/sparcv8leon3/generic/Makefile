#
# Makefile for Phoenix-RTOS loader (SPARCV8 LEON3 GENERIC HAL)
#
# Copyright 2022 Phoenix Systems
#
# %LICENSE%
#

LDFLAGS:=$(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS:=$(filter-out -Tdata% , $(LDFLAGS))

CFLAGS:=$(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS:=$(filter-out -mfpu% , $(CFLAGS))

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)

GCCLIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

PLO_COMMANDS = alias app call console copy dump echo go help map phfs reboot script wait

include devices/uart-leon3-qemu/Makefile

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, hal.o timer.o console.o)
