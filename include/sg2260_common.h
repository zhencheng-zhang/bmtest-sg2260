#ifndef __SG2260_COMMON_H_
#define __SG2260_COMMON_H_

#ifdef CONFIG_CHIP_SG2042
#define SYS_CTRL_BASE		0x7030010000
#define TOP_BASE		SYS_CTRL_BASE
#define SOFT_RESET_REG0 	0x7030013000
#define SOFT_RESET_REG1 	0x7030013004

#define EFUSE0_BASE		0x7030000000
#define EFUSE1_BASE		0x7030001000
#define RTC_BASE		0x7030002000
#define TIMER_BASE		0x7030003000
#define WDT_BASE		0x7030004000
#define I2C0_BASE		0x7030005000
#define I2C1_BASE		0x7030006000
#define I2C2_BASE		0x7030007000
#define I2C3_BASE		0x7030008000
#define GPIO0_BASE		0x7030009000
#define GPIO1_BASE		0x703000A000
#define GPIO2_BASE		0x703000B000
#define	PWM_BASE		0x703000C000
#define UART0_BASE		0x7040000000
#define UART1_BASE		0x7040001000
#define UART2_BASE		0x7040002000
#define UART3_BASE		0x7040003000
#define SPI0_BASE		0x07000180000
#define SPI1_BASE		0x07002180000

#define CLINT_BASE		0x7094000000
#define PLIC_BASE		0x7090000000

#define CLK_EN_REG0 		0x7030012000
#define CLK_EN_REG1 		0x7030012004

#elif CONFIG_CHIP_SG2260
#define SYS_CTRL_BASE		0x7050000000
#define TOP_BASE		SYS_CTRL_BASE
#define SOFT_RESET_REG0 	0x7050003000
#define SOFT_RESET_REG1 	0x7050003004

#define UART0_BASE		0x7030000000
#define UART1_BASE		0x7030001000
#define UART2_BASE		0x7030002000
#define UART3_BASE		0x7030003000
#define SPI0_BASE		0x7001000000
#define SPI1_BASE		0x7005000000
#define ETH_BASE		0x7030006000
#define EMMC_BASE		0x703000A000
#define SD_BASE			0x703000B000
#define EFUSE0_BASE		0x7040000000
#define EFUSE1_BASE		0x7040001000
#define RTC_BASE		0x7040002000
#define TIMER_BASE		0x7040003000
#define WDT_BASE		0x7040004000
#define I2C0_BASE		0x7040005000
#define I2C1_BASE		0x7040006000
#define I2C2_BASE		0x7040007000
#define I2C3_BASE		0x7040008000
#define GPIO0_BASE		0x7040009000
#define GPIO1_BASE		0x704000A000
#define GPIO2_BASE		0x704000B000
#define	PWM_BASE		0x704000C000
#define DW_SPI0_BASE      0x7030004000
#define DW_SPI1_BASE      0x7030005000

#define CLINT_BASE		0x7094000000
#define PLIC_BASE		0x6E00000000

#define CLK_EN_REG0 		0x7050002000	
#define CLK_EN_REG1 		0x7050002004
#endif



/* CLINT */
#define CLINT_TIMECMPL0		(CLINT_BASE + 0x4000)
#define CLINT_TIMECMPH0		(CLINT_BASE + 0x4004)

#define CLINT_MTIME(cnt)		asm volatile("csrr %0, time\n" : "=r"(cnt) :: "memory");

/* PLIC */
#define PLIC_PRIORITY0		(PLIC_BASE + 0x0)
#define PLIC_PRIORITY1		(PLIC_BASE + 0x4)
#define PLIC_PRIORITY2		(PLIC_BASE + 0x8)
#define PLIC_PRIORITY3		(PLIC_BASE + 0xc)
#define PLIC_PRIORITY4		(PLIC_BASE + 0x10)

#define PLIC_PENDING1		(PLIC_BASE + 0x1000)
#define PLIC_PENDING2		(PLIC_BASE + 0x1004)
#define PLIC_PENDING3		(PLIC_BASE + 0x1008)
#define PLIC_PENDING4		(PLIC_BASE + 0x100C)

#define PLIC_ENABLE1		(PLIC_BASE + 0x2000)
#define PLIC_ENABLE2		(PLIC_BASE + 0x2004)
#define PLIC_ENABLE3		(PLIC_BASE + 0x2008)
#define PLIC_ENABLE4		(PLIC_BASE + 0x200C)

#define PLIC_THRESHOLD		(PLIC_BASE + 0x200000)
#define PLIC_CLAIM		(PLIC_BASE + 0x200004)

typedef struct {
  volatile uint32_t* timecmpl0;
  volatile uint32_t* timecmph0;
  volatile uint64_t* mtime;
  uint64_t smtime;
  uint64_t interval;
  void (*intr_handler)(void);
} timer_hls_t;

#define TIMER_HZ 1000

#endif /*__SG2260_COMMOM_H */
