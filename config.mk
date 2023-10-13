OPTFLAGS := \
	-O0 \
	-Wall \
	-Werror

COMMONFLAGS := \
	-march=rv64imafdc -mcmodel=medany -mabi=lp64d

ifeq ($(BOARD), ZEBU)
DEFS += -DBOARD_ZEBU
DEFS += -DBOARD_PALLADIUM
endif

ifeq ($(BOARD), ASIC)
DEFS += -DBOARD_ASIC
DEFS += -DSUBTYPE_ASIC
endif

ifeq ($(BOARD), FPGA)
DEFS += -DBOARD_FPGA
DEFS += -DSUBTYPE_FPGA
endif

ifeq ($(BOARD), PALLADIUM)
DEFS += -DPLATFORM_PALLADIUM
DEFS += -DSUBTYPE_PALLADIUM
endif

ifeq ($(BOARD), QEMU)
DEFS += -DBOARD_QEMU
endif

ifeq ($(DEBUG), 1)
DEFS += -g -DENABLE_DEBUG
endif

DEFS += -DUSE_BMTAP

ifeq ($(CHIP), SG2042)
DEFS += -DCONFIG_CHIP_SG2042
CFLAGS += -DCONFIG_CHIP_SG2042
else
DEFS += -DCONFIG_CHIP_SG2260
CFLAGS += -DCONFIG_CHIP_SG2260
endif

ASFLAGS := \
	$(OPTFLAGS) \
	$(COMMONFLAGS) \
	-std=gnu99

CFLAGS := \
	$(COMMONFLAGS) \
	$(OPTFLAGS) \
	-ffunction-sections \
	-fdata-sections \
	-nostdlib \
	-std=gnu99

CXXFLAGS := \
	$(COMMONFLAGS) \
	$(OPTFLAGS) \
	-ffunction-sections \
	-fdata-sections \
	-nostdlib \
	-fno-threadsafe-statics \
	-fno-rtti \
	-fno-exceptions \
	-std=c++14

LDFLAGS := \
	$(COMMONFLAGS) \
	-Wl,--build-id=none \
	-Wl,--cref \
	-Wl,--check-sections \
	-Wl,--gc-sections \
	-Wl,--unresolved-symbols=report-all \
	-nostartfiles

INC += \
	-I$(ROOT)/include \
	-I$(ROOT)/drivers/include \
	-I$(ROOT)/test

C_SRC += \
	$(ROOT)/common/main.c                   \
	$(ROOT)/common/version.c                \
	$(ROOT)/common/axi_mon.c                \
	$(ROOT)/common/cli.c                    \
	$(ROOT)/common/crc16.c                  \
	$(ROOT)/common/phy_pll_init.c           \
	$(ROOT)/common/command.c                \
	$(ROOT)/common/system.c                 \
	$(ROOT)/common/syscalls.c               \
	$(ROOT)/common/pll.c                    \
	$(ROOT)/common/timer.c                  \
	$(ROOT)/common/md5.c                    \
	$(ROOT)/common/dma.c                    \
	$(ROOT)/common/common.c                 \
	$(ROOT)/common/thread_safe_printf.c     \
	$(ROOT)/common/riscv_lock.c             \
	$(ROOT)/common/smp.c

ifeq ($(ARCH), RISCV)
C_SRC  += $(ROOT)/common/interrupts.c
CFLAGS += -DCONFIG_64BIT
endif

include $(ROOT)/drivers/config.mk

ifeq ($(INIT_DDR), YES)
DEFS += -DINIT_DDR
endif


ifeq ($(RUN_ENV), SRAM)
DEFS += -DRUN_IN_SRAM
else
DEFS += -DRUN_IN_DDR
endif

ifeq ($(BOARD), QEMU)
C_SRC += $(ROOT)/common/uart_pl01x.c
else
C_SRC += $(ROOT)/common/uart_dw.c
endif

ifeq ($(CHIP_NUM), MULTI)
DEFS += -DCONFIG_SUPPORT_SMP
endif

A_SRC +=

include $(ROOT)/boot/config.mk
include $(ROOT)/common/stdlib/stdlib.mk