#include <stdio.h>
#include "boot_test.h"
#include "system_common.h"
#include "timer.h"
#include "testcase_watchdog.h"
#include "mmio.h"
#include "command.h"
#include "cli.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define TEST_WITH_TNTERRUPT  	1

#ifdef CONFIG_CHIP_SG2042
#define WATCHDOG_INTR	 		99
#elif CONFIG_CHIP_SG2260
#define WATCHDOG_INTR	 		28
#endif

#define REG_WDT				WDT_BASE
#define WDT_CR				0x00
#define WDT_TORR			0x04
#define WDT_CCVR			0x08
#define WDT_CRR				0x0C
#define WDT_STAT			0x10
#define WDT_EOI				0x14
#define WDT_COMP_PARAM_5	0xE4

#define WDT_COMP_PARAM_4	0xE8
#define WDT_COMP_PARAM_3	0xEC
#define WDT_COMP_PARAM_2	0xF0
#define WDT_COMP_PARAM_1	0xF4
#define WDT_COMP_VERSION	0xF8
#define WDT_COMP_TYPE		0xFC

#define GPIO0DATA						0x50027000
#define GPIO0DIRECTION					0x50027004
#define PINMUXGPIO0						0x70300110f8

// Top Misc Control Registers in sg2042
#define REG_TOP_CONTROL					TOP_BASE + 0x008

#define WDT_RST_BIT		14

static u32 us1,us2,us3;

#if TEST_WITH_TNTERRUPT
int watchdog_irq_handler(int irqn, void *priv)
{
    u32 int_status;
	int_status = readl(REG_WDT + WDT_STAT);

	/** Interrupt Clear Register:
	 * Clears the watchdog interrupt.
	 * This can be used to clear the interrupt without restarting the watchdog counter.
	 */
	/*clear int stop reboot; or dont clear let it reboot*/
	// mmio_read_32(REG_WDT + WDT_EOI);
	us3 = timer_meter_get_us();

	uartlog("irq us1=%d us2=%d us3=%d\n", us1,us2,us3);
	uartlog("%s  irqn=%d int_status=0x%x  \n",__func__, irqn, int_status);
	return 0;
}
#endif


static int response_test(int argc, char **argv)
{
	uartlog("%s\n", __func__);
	u32 rpl;

	int response_mode = 0;
	if (argc > 1)
		response_mode = strtol(argv[1], NULL, 10);
	
	if (response_mode == 1) {
		uartlog("In interrupt mode\n");
	} else {
		response_mode = 0;
		uartlog("In reset mode by default\n");
	}

	//reg_sw_root_reset_en
	mmio_write_32(REG_TOP_CONTROL,mmio_read_32(REG_TOP_CONTROL) | 0x04);

	//set pluse lenth 0b'100 �C 32 pclk cycles; at least 4pclk cycles
	rpl = 4;

	// Control Bit 10010: 8 pclk cycles
	//mode :0 reset system,1 first generate interrupt second timeout reset system
	//disable :0,  enable 1
	mmio_write_32(REG_WDT+WDT_CR, (rpl<<2) | (response_mode)<<1 | 0 );
	//mmio_write_32(REG_WDT+WDT_CR, (rpl<<2)  );
#ifdef	PLATFORM_PALLADIUM
	mmio_write_32(REG_WDT+WDT_TORR, 0x2);
#else
	// Timeout Range Register
	//2^(16 + i)
	mmio_write_32(REG_WDT+WDT_TORR, 0x9 );
#endif

	uartlog("CR: %x    TORR:%x  WDT_COMP_PARAM_1: %x\n", mmio_read_32(REG_WDT+WDT_CR),
													mmio_read_32(REG_WDT+WDT_TORR), 
													mmio_read_32(REG_WDT + WDT_COMP_PARAM_1));

#if TEST_WITH_TNTERRUPT
	request_irq(WATCHDOG_INTR, watchdog_irq_handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_MASK, "timer int", NULL);
#endif

	//Pin Mux Selector for GPIO0
	cpu_write32(PINMUXGPIO0,cpu_read32(PINMUXGPIO0) & ~(0x3<<20));
	writel(GPIO0DIRECTION, 0xffffffff);
	writel(GPIO0DATA,0xffffffff);

	// enable watch dog
	timer_meter_start();
	// uartlog("%s 0\n", __func__);

	writel(CLK_EN_REG0, readl(CLK_EN_REG0)&~(1<<27));
	writel(CLK_EN_REG0, readl(CLK_EN_REG0)|(1<<27));

	uartlog("Init CCVR: %x\n", mmio_read_32(REG_WDT+WDT_CCVR));

	// WDT enable
	mmio_write_32(REG_WDT+WDT_CR, (mmio_read_32(REG_WDT+WDT_CR)) | 0x1 );
	us1 = timer_meter_get_us();
	uartlog("us1: %u\n", us1);
	

#ifdef	PLATFORM_PALLADIUM
	while((timer_meter_get_us() -us1) < 10000)
	// while(0)
#else
	while((timer_meter_get_us() -us1) < 5000)
#endif
	{
		// mmio_write_32(REG_WDT+WDT_CRR,0x76);
		uartlog("CCVR: %x\n", mmio_read_32(REG_WDT+WDT_CCVR));
	}
	us2 = timer_meter_get_us();
	uartlog("us2: %u\n", us2);

#if !TEST_WITH_TNTERRUPT
	writel(GPIO0DATA,0);
	//writel(CLK_EN_REG1, readl(CLK_EN_REG1)&~(1<<4));
	//mdelay(1);
	//writel(CLK_EN_REG1, readl(CLK_EN_REG1)|(1<<4));

	while (mmio_read_32(REG_WDT+WDT_STAT) == 0);

	us3 = timer_meter_get_us();
	writel(GPIO0DATA,0xffffffff);
	uartlog(" us1=%d us2=%d us3=%d\n", us1,us2,us3);
#endif

  uartlog("need check whether cpu is reset!!!!\n");

  return 0;
}

static struct cmd_entry test_cmd_list[] __attribute__((unused)) = {
	{ "response_mode", response_test, 0, "set response mode (0: direct reset mode (default), 1: interrupt mode)" },
	//{ "loop", do_loop, 3, "loop port master/slave baudrate" },
	{ NULL, NULL, 0, NULL },
};

int testcase_watchdog(void)
{
	int i, ret = 0;

	printf("enter watchdog test\n");

	for (i = 0; i < ARRAY_SIZE(test_cmd_list) - 1; i++) {
		command_add(&test_cmd_list[i]);
	}
	cli_simple_loop();

	return ret;
}

#ifndef BUILD_TEST_CASE_ALL
int testcase_main()
{
  return testcase_watchdog();
}
#endif