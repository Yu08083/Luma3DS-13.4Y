rwildcard = $(foreach d, $(wildcard $1*), $(filter $(subst *, %, $2), $d) $(call rwildcard, $d/, $2))

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/3ds_rules

name := pxi

dir_source := source
dir_build := build
dir_out := ../../$(dir_build)

LIBS := -lctru
LIBDIRS	:= $(CTRULIB)
LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

INCLUDE	:= $(foreach dir,$(LIBDIRS),-I$(dir)/include)

ARCH := -mcpu=mpcore -mfloat-abi=hard -mtp=soft
# --- 追加: -fpermissive の代わりの、的を絞った診断設定 -----------------------
# GCC 14 以降、以下はすべて既定でエラー(permerror)になった。
# -fpermissive はこれを全部まとめて警告に格下げしてしまうため使わない。
#
#   実行時に本当に壊れるもの → エラーのまま維持する
#     implicit-function-declaration : 戻り値が int と仮定され u64/float/構造体で破綻する
#     implicit-int / return-type    : 未定義の値が返る
#
#   ARM32 では実害が出にくいもの → 警告に落として目視で確認する
#     int-conversion / incompatible-pointer-types
#     （ポインタも整数も 32bit で同じレジスタに乗るため生成コードは変わらない。
#       2018 年の v9.1 ツリーには元から大量にあり、止めるとビルドが進まない）
HARDERRORS := -Werror=implicit-function-declaration -Werror=implicit-int \
              -Werror=return-type \
              -Wno-error=int-conversion -Wno-error=incompatible-pointer-types
ifeq ($(strip $(STRICT)),1)
    WERROR := -Werror
else
    WERROR :=
endif
# ---------------------------------------------------------------------------

CFLAGS := -Wall -Wextra $(WERROR) $(HARDERRORS) -MMD -MP -marm $(ARCH) -fno-builtin -std=c11 -O2 -g -ffast-math -mword-relocations \
	  -ffunction-sections -fdata-sections $(INCLUDE) -DARM11 -D_3DS
LDFLAGS := -specs=3dsx.specs -Wl,--gc-sections $(ARCH)

objects = $(patsubst $(dir_source)/%.c, $(dir_build)/%.o, \
          $(call rwildcard, $(dir_source), *.c))

.PHONY: all
all: $(dir_out)/$(name).cxi

.PHONY: clean
clean:
	@rm -rf $(dir_build)

$(dir_out)/$(name).cxi: $(dir_build)/$(name).elf
	@makerom -f ncch -rsf $(name).rsf -nocodepadding -o $@ -elf $<

$(dir_build)/$(name).elf: $(objects)
	$(LINK.o) $(OUTPUT_OPTION) $^ $(LIBPATHS) $(LIBS)

$(dir_build)/memory.o : CFLAGS += -O3

$(dir_build)/%.o: $(dir_source)/%.c
	@mkdir -p "$(@D)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<
include $(call rwildcard, $(dir_build), *.d)
