#
# Makefile for Phoenix-RTOS loader (RISC-V 64 Generic HAL)
#
# Copyright 2023 Phoenix Systems
#
# %LICENSE%
#

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)

GCCLIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

PLO_COMMANDS = alias app call console copy dump echo go help kernel map mem phfs reboot script wait

include devices/flash-riscv64/Makefile

ifeq ($(TARGET_PROJECT), qemu)
  include devices/uart-16550/Makefile
else ifeq ($(TARGET_PROJECT), spike)
  include devices/tty-spike/Makefile
endif

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, console.o hal.o timer.o)
