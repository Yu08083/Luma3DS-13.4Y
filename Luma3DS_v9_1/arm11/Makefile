rwildcard = $(foreach d, $(wildcard $1*), $(filter $(subst *, %, $2), $d) $(call rwildcard, $d/, $2))

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_tools

name := $(shell basename $(CURDIR))

dir_source := source
dir_build := build
dir_out := ../$(dir_build)

ASFLAGS := -mcpu=mpcore
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

CFLAGS  := -Wall -Wextra $(WERROR) $(HARDERRORS) -MMD -MP -marm $(ASFLAGS) -fno-builtin -std=c11 -Wno-main -O2 -flto -ffast-math
LDFLAGS := -nostartfiles -Wl,--nmagic

objects = $(patsubst $(dir_source)/%.s, $(dir_build)/%.o, \
          $(patsubst $(dir_source)/%.c, $(dir_build)/%.o, \
          $(call rwildcard, $(dir_source), *.s *.c)))

.PHONY: all
all: $(dir_out)/$(name).elf

.PHONY: clean
clean:
	@rm -rf $(dir_build)

$(dir_out)/$(name).elf: $(objects)
	$(LINK.o) -T linker.ld $(OUTPUT_OPTION) $^

$(dir_build)/%.o: $(dir_source)/%.c
	@mkdir -p "$(@D)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(dir_build)/%.o: $(dir_source)/%.s
	@mkdir -p "$(@D)"
	$(COMPILE.s) $(OUTPUT_OPTION) $<
