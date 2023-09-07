#
# Makefile for Phoenix-RTOS loader (RISC-V 64 HAL)
#
# Copyright 2023 Phoenix Systems
#
# %LICENSE%
#

include hal/riscv64/$(TARGET_SUBFAMILY)/Makefile

CFLAGS += -Ihal/riscv64/$(TARGET_SUBFAMILY)

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/, _init.o _interrupts.o _string.o exceptions.o string.o)
ifeq ($(TARGET_PROJECT), spike)
  OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/, interrupts.o)
else
  OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/, interrupts-plic.o plic.o)
endif
