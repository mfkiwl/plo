
# Makefile for Phoenix SBI
#
# Copyright 2024 Phoenix Systems
#
# %LICENSE%
#

SIL ?= @
MAKEFLAGS += --no-print-directory

KERNEL=1

include ../../phoenix-rtos-build/Makefile.common

# check necessary variables
ifeq ($(filter clean,$(MAKECMDGOALS)),)
  ifeq ($(PAYLOAD_PATH),)
    $(error PAYLOAD_PATH is not set)
  endif
  ifeq ($(PAYLOAD_ADDR),)
    $(error PAYLOAD_ADDR is not set)
  endif
endif

LDGEN ?= $(CC)

CFLAGS += -I. -Iinclude -ffreestanding
CPPFLAGS += -DPAYLOAD_PATH=\"$(PAYLOAD_PATH)\"

ifneq ($(FDT_PATH),)
  CPPFLAGS += -DFDT_PATH=\"$(FDT_PATH)\"
  $(PREFIX_O)core/_start.o: $(FDT_PATH)
endif

LDSFLAGS += -DPAYLOAD_ADDR=$(PAYLOAD_ADDR)

PAYLOAD_NAME := $(notdir $(PAYLOAD_PATH))
$(PREFIX_O)core/_start.o: $(PREFIX_O)/$(PAYLOAD_NAME).stamp

$(PREFIX_O)/$(PAYLOAD_NAME).stamp: $(PAYLOAD_PATH)
	@touch $@

OBJS :=

include core/Makefile
include devices/Makefile

OBJS += $(addprefix $(PREFIX_O), entry.o)

# incremental build quick-fix, WARN: assuming the sources are in c
DEPS := $(patsubst %.o, %.c.d, $(OBJS))
-include $(DEPS)

.PHONY: all base ram clean

.PRECIOUS: $(BUILD_DIR)%/.

all: base ram

base: $(PREFIX_PROG_STRIPPED)sbi-$(TARGET_SUBFAMILY).elf $(PREFIX_PROG_STRIPPED)sbi-$(TARGET_SUBFAMILY).img

ram: $(PREFIX_PROG_STRIPPED)sbi-ram-$(TARGET_SUBFAMILY).elf $(PREFIX_PROG_STRIPPED)sbi-ram-$(TARGET_SUBFAMILY).img


$(PREFIX_PROG)sbi-$(TARGET_SUBFAMILY).elf: $(PREFIX_O)/sbi-$(TARGET_SUBFAMILY).ld $(OBJS) | $(PREFIX_PROG)/.
	@echo "LD  $(@F)"
	$(SIL)$(LD) $(CFLAGS) $(LDFLAGS) -Wl,-Map=$<.map -o $@ -Wl,-T,$^ -nostdlib -lgcc


$(PREFIX_PROG)sbi-ram-$(TARGET_SUBFAMILY).elf: $(PREFIX_O)/sbi-$(TARGET_SUBFAMILY).ld $(OBJS) | $(PREFIX_PROG)/.
	@echo "LD  $(@F)"
	$(SIL)$(LD) $(CFLAGS) $(LDFLAGS) -Wl,-Map=$<.map -o $@ -Wl,-T,$^ -nostdlib -lgcc


$(PREFIX_PROG_STRIPPED)%.hex: $(PREFIX_PROG_STRIPPED)%.elf
	@echo "HEX $(@F)"
	$(SIL)$(OBJCOPY) -O ihex $< $@


$(PREFIX_PROG_STRIPPED)%.img: $(PREFIX_PROG_STRIPPED)%.elf
	@echo "BIN $(@F)"
	$(SIL)$(OBJCOPY) -O binary $< $@


-include $(PREFIX_O)/sbi-$(TARGET_SUBFAMILY)*ld.d
$(PREFIX_O)/sbi-$(TARGET_SUBFAMILY).ld: | $(PREFIX_O)/.
	@echo "GEN $(@F)"
	$(SIL)$(LDGEN) $(LDSFLAGS) -MP -MF $@.d -MMD -D__LINKER__ -undef -xc -E -P ld/$(TARGET_SUBFAMILY).ldt > $@
	$(SIL)$(SED) -i.tmp -e 's`.*\.o[ \t]*:`$@:`' $@.d && rm $@.d.tmp


$(PREFIX_O)/sbi-$(TARGET_SUBFAMILY)-%.ld: | $(PREFIX_O)/.
	@echo "GEN $(@F)"
	$(SIL)$(LDGEN) $(LDSFLAGS) -MP -MF $@.d -MMD -D__LINKER__ -D$* -undef -xc -E -P ld/$(TARGET_SUBFAMILY).ldt > $@
	$(SIL)$(SED) -i.tmp -e 's`.*\.o[ \t]*:`$@:`' $@.d && rm $@.d.tmp


%/.:
	@echo "MKDIR $(@D)"
	$(SIL)mkdir -p "$(@D)"


clean:
	@echo "rm -rf $(BUILD_DIR)"


ifneq ($(filter clean,$(MAKECMDGOALS)),)
	$(shell rm -rf $(BUILD_DIR))
endif
