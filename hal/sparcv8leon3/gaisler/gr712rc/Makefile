#
# Makefile for Phoenix-RTOS loader (SPARCV8 LEON3 GR712RC HAL)
#
# Copyright 2023 Phoenix Systems
#
# %LICENSE%
#

comma := ,

LDFLAGS:=$(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS:=$(filter-out -Tdata% , $(LDFLAGS))
LDFLAGS:=$(filter-out -Wl$(comma)--section-start% , $(LDFLAGS))

CFLAGS:=$(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS:=$(filter-out -mfpu% , $(CFLAGS))

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)

PLO_COMMANDS := alias app call console copy dump echo go help kernel map mem phfs reboot script wait

include devices/uart-grlib/Makefile

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, _init.o hal.o gr712rc.o)
