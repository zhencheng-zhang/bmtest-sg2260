#sophgo-test default configs here, you can change it.
NOSTD		?= 0 #if 0, use newlib; else use std lib of ourself
DEBUG		?= 1 #if 0, disable printf
SHELL_EN	?= 1 #if 0, disable shell

BOARD		?= PALLADIUM
CHIP		?= SG2260
CHIP_NUM	?= SINGLE #single or multi
ARCH		?= RISCV

V	?= 1

ifeq ($(strip $(V)), 1)
	Q =
	E =
else
	Q = @
	E = 1>/dev/null 2>&1
endif

ROOT := $(shell pwd)
OUTPUT_NAME := $(CHIP)
OUTPUT_PATH := out
RELEASE_PATH := release_out
TOOL_PATH := /home/zzc/code/gcc-riscv/gcc-riscv64-unknown-elf/bin/

include config.mk

#gloable variable here
comma   := ,
empty   :=
space   := $(empty) $(empty)

OUTPUT_NAME := $(OUTPUT_NAME)_$(BOARD)_$(strip $(subst $(space),_,$(TEST_CASE)))${SUB_TEST_CASE}
C_SRC += $(wildcard $(TEST_CASE:%=$(ROOT)/test/%/*.c))
C_SRC += $(wildcard $(TEST_CASE:%=$(ROOT)/test/%/$(CHIP)/*.c))
configs := $(foreach testcase,$(TEST_CASE),$(ROOT)/test/$(testcase)/config.mk)
ifneq ($(configs), )
sinclude $(configs)
endif

CROSS_COMPILE := $(TOOL_PATH)riscv64-unknown-elf-

AS := $(CROSS_COMPILE)as
CC := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

MKDIR := mkdir -p
RM := rm -rf
CP := cp -r
SSEC := *.slow*

EXT_C_OBJ := $(patsubst %,   %.o, $(EXT_C_SRC))
A_OBJ   := $(addprefix $(OUTPUT_PATH)/, $(patsubst $(ROOT)/%,   %.o, $(A_SRC)))
C_OBJ   := $(addprefix $(OUTPUT_PATH)/, $(patsubst $(ROOT)/%,   %.o, $(C_SRC)))
CXX_OBJ := $(addprefix $(OUTPUT_PATH)/, $(patsubst $(ROOT)/%,   %.o, $(CXX_SRC)))

MAP	:= $(OUTPUT_PATH)/$(OUTPUT_NAME).map
ELF	:= $(OUTPUT_PATH)/$(OUTPUT_NAME).elf
BIN	:= $(OUTPUT_PATH)/$(OUTPUT_NAME).bin
ASM	:= $(OUTPUT_PATH)/$(OUTPUT_NAME).asm
TEXT	:= $(OUTPUT_PATH)/$(OUTPUT_NAME).bin.text

CFLAGS += -g

all: $(OUTPUT_PATH) $(BIN) $(ASM) $(TEXT)

.PHONY: $(USER_PHONY) $(OUTPUT_PATH) clean release all_src all_def all_inc all_flag

$(OUTPUT_PATH):
	$(MKDIR) $(OUTPUT_PATH) $(dir $(A_OBJ)) $(dir $(C_OBJ)) $(dir $(CXX_OBJ)) $(dir $(MAP)) $(dir $(ELF)) $(dir $(BIN)) $(dir $(ASM)) $(dir $(BIN_OBJ)) $(dir $(TEXT))

clean: $(USER_CLEAN)
	$(RM) $(OUTPUT_PATH)

release: all
	$(MKDIR) $(RELEASE_PATH)
	$(CP) $(MAP) $(RELEASE_PATH)/$(OUTPUT_NAME)_$(shell date "+%F-%H-%M-%S").map
	$(CP) $(ELF) $(RELEASE_PATH)/$(OUTPUT_NAME)_$(shell date "+%F-%H-%M-%S").elf
	$(CP) $(BIN) $(RELEASE_PATH)/$(OUTPUT_NAME)_$(shell date "+%F-%H-%M-%S").bin
	$(CP) $(ASM) $(RELEASE_PATH)/$(OUTPUT_NAME)_$(shell date "+%F-%H-%M-%S").asm

$(EXT_C_OBJ): %.o: %
	"$(CC)" -c $(DEFS) $(CFLAGS) $< -o $@ $(INC)

$(A_OBJ): $(OUTPUT_PATH)/%.o: $(ROOT)/%
	"$(CC)" -c $(DEFS) $(ASFLAGS) $< -o $@ $(INC)

$(C_OBJ): $(OUTPUT_PATH)/%.o: $(ROOT)/%
	"$(CC)" -c $(DEFS) $(CFLAGS) $< -o $@ $(INC)

$(CXX_OBJ): $(OUTPUT_PATH)/%.o: $(ROOT)/%
	"$(CXX)" -c $(DEFS) $(CXXFLAGS) $< -o $@ $(INC)

$(BIN_OBJ): $(OUTPUT_PATH)/%.o: $(ROOT)/%
	"$(OBJCOPY)" -I binary -O elf64-littleaarch64 --binary-architecture aarch64 $< $@
	"$(OBJCOPY)" --redefine-sym _binary_`echo $< | sed -z 's/[/.]/_/g'`_start=_binary_`echo $* | sed -z 's/[/.]/_/g'`_start $@

$(ELF): $(A_OBJ) $(EXT_C_OBJ) $(C_OBJ) $(CXX_OBJ) $(LIB_OBJ) $(BIN_OBJ)
	#touch $(ROOT)/test/$(TEST_CASE)/testcase_$(TEST_CASE).c
	echo "const char fw_version_info[] = \"`git rev-parse HEAD`\";" > common/version.c
	$(CC) -T$(LINK) -o $@ -Wl,-Map,$(MAP) $(LDFLAGS) \
	-Wl,--start-group \
	$^ \
	-lm ${_LDFLAGS} \
	-Wl,--end-group

$(BIN): $(ELF)
	$(OBJCOPY) -O binary -R $(SSEC) $^ $@

$(ASM): $(ELF)
	$(OBJDUMP) -DS $^ > $@

$(TEXT): $(BIN)
	hexdump -v -e '1/4 "%08x\n"' $^ > $@

all_obj:
	@echo "ASM OBJ:<"$(A_OBJ)">"
	@echo "EXT C OBJ:<"$(EXT_C_OBJ)">"
	@echo "C OBJ:<"$(C_OBJ)">"
	@echo "CXX OBJ:<"$(CXX_OBJ)">"

all_src:
	@echo "ASM SRC:<"$(A_SRC)">"
	@echo "C SRC:<"$(C_SRC)">"
	@echo "CXX SRC:<"$(CXX_SRC)">"

all_def:
	@echo $(DEFS)

all_inc:
	@echo $(INC)

all_flag:
	@echo "LDFLAGS:<"$(LDFLAGS)">"
	@echo "ASFLAGS:<"$(ASFLAGS)">"
	@echo "CFLAGS:<"$(CFLAGS)">"