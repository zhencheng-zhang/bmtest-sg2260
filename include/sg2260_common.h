#ifndef __ATHENA2RV_COMMON_H_
#define __ATHENA2RV_COMMON_H_

#define SEC_BASE            0x02000000
#define SYS_CTRL_BASE		0x7030010000
#define TOP_BASE		SYS_CTRL_BASE

#define SPACC_BASE          (SEC_BASE + 0x00060000)
#define TRNG_BASE           (SEC_BASE + 0x00070000)
#define SEC_DBG_I2C_BASE    (SEC_BASE + 0x00080000)
#define FAB_FIREWALL_BASE   (SEC_BASE + 0x00090000)
#define DDR_FIREWALL_BASE   (SEC_BASE + 0x000A0000)

#define PINMUX_BASE SYS_CTRL_BASE
#define TEMPSEN_BASE        (TOP_BASE + 0xE0000)
#define CLKGEN_BASE         (TOP_BASE + 0x00002000)
#define WDT0_BASE            (0x27000000)
#define WDT1_BASE            (0x27001000)
#define WDT2_BASE            (0x27002000)

#define UART0_BASE          0x7040000000
#define UART1_BASE          0x7040001000
#define UART2_BASE          0x7040002000
#define UART3_BASE          0x7040003000

#define GPIO0_BASE          0x7030009000
#define GPIO1_BASE          0x703000A000
#define GPIO2_BASE          0x703000B000


#define KEYSCAN_BASE        0x27030000
#define SRAM_BASE           0x0E000000
#define IRRX_BASE           0x0502E000

#define REG_CLK_ENABLE_REG0       (CLKGEN_BASE)
#define REG_CLK_ENABLE_REG1       (CLKGEN_BASE + 0x4)
#define REG_CLK_BYPASS_SEL_REG      (CLKGEN_BASE + 0x30)
#define REG_CLK_BYPASS_SEL_REG2      (CLKGEN_BASE + 0x34)
#define REG_CLK_DIV0_CTL_CA53_REG   (CLKGEN_BASE + 0x40)
#define REG_CLK_DIV0_CTL_CPU_AXI0_REG (CLKGEN_BASE + 0x48)
#define REG_CLK_DIV0_CTL_TPU_AXI_REG  (CLKGEN_BASE + 0x54)
#define REG_CLK_DIV0_CTL_TPU_FAB_REG  (CLKGEN_BASE + 0x5C)

/* RISC-V */
#define CLINT_BASE              0x7094000000
#define PLIC_BASE               0x7090000000

/* CLINT */
#define CLINT_TIMECMPL0         (CLINT_BASE + 0x4000)
#define CLINT_TIMECMPH0         (CLINT_BASE + 0x4004)

#define CLINT_MTIME(cnt)             asm volatile("csrr %0, time\n" : "=r"(cnt) :: "memory");

/* PLIC */
#define PLIC_PRIORITY0          (PLIC_BASE + 0x0)
#define PLIC_PRIORITY1          (PLIC_BASE + 0x4)
#define PLIC_PRIORITY2          (PLIC_BASE + 0x8)
#define PLIC_PRIORITY3          (PLIC_BASE + 0xc)
#define PLIC_PRIORITY4          (PLIC_BASE + 0x10)

#define PLIC_PENDING1           (PLIC_BASE + 0x1000)
#define PLIC_PENDING2           (PLIC_BASE + 0x1004)
#define PLIC_PENDING3           (PLIC_BASE + 0x1008)
#define PLIC_PENDING4           (PLIC_BASE + 0x100C)

#define PLIC_ENABLE1            (PLIC_BASE + 0x2000)
#define PLIC_ENABLE2            (PLIC_BASE + 0x2004)
#define PLIC_ENABLE3            (PLIC_BASE + 0x2008)
#define PLIC_ENABLE4            (PLIC_BASE + 0x200C)

#define PLIC_THRESHOLD          (PLIC_BASE + 0x200000)
#define PLIC_CLAIM              (PLIC_BASE + 0x200004)

typedef struct {
  volatile uint32_t* timecmpl0;
  volatile uint32_t* timecmph0;
  volatile uint64_t* mtime;
  uint64_t smtime;
  uint64_t interval;
  void (*intr_handler)(void);
} timer_hls_t;

#define TIMER_HZ 1000

#endif /*__ATHENA2RV_COMMOM_H */
