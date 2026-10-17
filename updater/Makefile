.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error DEVKITARM is not set)
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET := Luma3DS-13.4Y-Updater
BUILD := build
CORE := $(abspath $(TOPDIR)/../Luma3DS_v13_4/sysmodules/rosalina)
SOURCES := source $(CORE)/source $(CORE)/third_party/mbedtls/library
INCLUDES := include $(CORE)/include $(CORE)/third_party/mbedtls/include
LIBDIRS := $(CTRULIB)

APP_TITLE := Luma3DS 13.4Y Updater
APP_DESCRIPTION := Luma3DS 13.4Y Updater v1.0.1
APP_AUTHOR := @yu_

ARCH := -march=armv6k -mtune=mpcore -mfpu=vfp -mfloat-abi=hard -mtp=soft
CFLAGS := -g -std=gnu11 -Os -Wall -Wextra -Werror -mword-relocations -ffunction-sections -fdata-sections -fstack-usage $(ARCH) -D__3DS__ -DMBEDTLS_CONFIG_FILE=\"updater_tls_config.h\"
CFLAGS += $(INCLUDE)
LDFLAGS = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map),--gc-sections
LIBS := -lctru -lm

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(CURDIR)/source $(CORE)/source $(CORE)/third_party/mbedtls/library
export DEPSDIR := $(CURDIR)/$(BUILD)
CFILES := $(notdir $(wildcard source/*.c)) updater_format.c updater_http.c updater_net.c updater_tls.c updater_ca.c updater_clock.c sha256.c $(notdir $(wildcard $(CORE)/third_party/mbedtls/library/*.c))
export OFILES := $(CFILES:.c=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(if $(filter /%,$(dir)),$(dir),$(CURDIR)/$(dir))) $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export LD := $(CC)

.PHONY: all clean cia

all: cia

cia:
	@mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@rm -rf $(BUILD) $(TARGET).cia $(TARGET).elf $(TARGET).smdh $(TARGET).bnr $(TARGET).3dsx

else

.PHONY: all

all: $(OUTPUT).cia

$(OUTPUT).elf: $(OFILES)
	@$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@

$(OUTPUT).smdh: $(TOPDIR)/assets/icon48x48.png
	@bannertool makesmdh -s "$(APP_TITLE)" -l "$(APP_DESCRIPTION)" -p "$(APP_AUTHOR)" -i $< -o $@

$(OUTPUT).bnr: $(TOPDIR)/assets/banner256x128.png $(TOPDIR)/assets/silent.wav
	@bannertool makebanner -i $(word 1,$^) -a $(word 2,$^) -o $@

$(OUTPUT).cia: $(OUTPUT).elf $(OUTPUT).smdh $(OUTPUT).bnr $(TOPDIR)/metadata.rsf
	@makerom -f cia -o $@ -elf $(OUTPUT).elf -rsf $(TOPDIR)/metadata.rsf -icon $(OUTPUT).smdh -banner $(OUTPUT).bnr -target t -exefslogo -ver 1025

-include $(OFILES:.o=.d)

endif
