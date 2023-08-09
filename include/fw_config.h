#ifndef __CONFIG_H__
#define __CONFIG_H__

#define ENABLE_PRINT

#if defined(BUILD_TEST_CASE_audio) && defined(RUN_IN_DDR) || defined(TEST_H264_DEC) || defined(TEST_H265_DEC)
#define CONFIG_HEAP_SIZE                (32*1024*1024)
#else
#if defined(RUN_IN_SRAM)
#define CONFIG_HEAP_SIZE                (8*1024)
#else
#define CONFIG_HEAP_SIZE                (4*1024*1024)
#endif
#endif

#ifdef __riscv
#ifdef BOARD_FPGA
#define CONFIG_TIMER_FREQ               (5000000)   // 5MHz
#else
#define CONFIG_TIMER_FREQ               (50000000)  // 50MHz
#endif
#else
#ifdef BOARD_FPGA
#define CONFIG_TIMER_FREQ               (5000000)   // 5MHz
#else
#define CONFIG_TIMER_FREQ               (25000000)  // 25MHz
#endif
#endif

#define CPU_TIMER_CLOCK           CONFIG_TIMER_FREQ

#define CONFIG_NPU_CTRL_REG0            (0x00)
#define CONFIG_NPU_CTRL_REG1            (0x04)
#define CONFIG_NPU_RED_REG0             (0x08)
#define CONFIG_NPU_RED_REG1             (0x0c)
#define CONFIG_NPU_RED_REG2             (0x10)
#define CONFIG_NPU_RED_REG3             (0x14)
#define CONFIG_NPU_MON_BUF              (0x80) //0x80~0x100
#define CONFIG_LOCAL_MEM_CTRL_SIZE      (0x400)

#define CONFIG_NPU_CTRL_REG_BASE        (0x43ff0000)
#define CONFIG_LOCALMEM_CLOUMN          (8)
#define CONFIG_LOCALMEM_ROW_WIDTH       (0x80)

#define CONFIG_DRAM_SIZE                0x80000000
#define CONFIG_DRAM_TOTAL_BANK          2
#define REG_BM1680_TOP_DDR_ADDR_MODE    0x1C0

#endif
