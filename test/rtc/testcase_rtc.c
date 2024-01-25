#include "testcase_rtc.h"
#include "mmio.h"
#include "system_common.h"
#include "timer.h"
#include "command.h"
#include "cli.h"
#include "boot_test.h"

#define   RTC_INTR 35

#define	RTC_CCVR_OFFSET       0x00
#define	RTC_CMR_OFFSET        0x04
#define	RTC_CLR_OFFSET        0x08
#define	RTC_CCR_OFFSET        0x0C
#define	RTC_STAT_OFFSET       0x10
#define	RTC_RSTAT_OFFSET      0x14
#define	RTC_EOI_OFFSET        0x18
#define	RTC_CPSR_OFFSET       0x20
#define	RTC_CPCVR_OFFSET      0x24

#define	RTC_RST_BIT	12
#define RTC_CLOCK_BIT	26

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

int rtc_irq_handler(int irqn, void *priv)
{
	uartlog("%s  irqn=0x%x end: %d\n",__func__, irqn, timer_meter_get_ms());
	return 0;
}

static void reset_rtc(void)
{
	writel(SOFT_RESET_REG0, readl(SOFT_RESET_REG0)&~(1<<RTC_RST_BIT));
	writel(SOFT_RESET_REG0, readl(SOFT_RESET_REG0)|(1<<RTC_RST_BIT));


	writel(CLK_EN_REG1, readl(CLK_EN_REG1)&~(1<<RTC_CLOCK_BIT));
	writel(CLK_EN_REG1, readl(CLK_EN_REG1)|(1<<RTC_CLOCK_BIT));
}

static int ms_test(int argc, char **argv)
{
	int ms_count = strtoul(argv[1], NULL, 10);

	uartlog("init CCR: %x\n", mmio_read_32(RTC_BASE+RTC_CCR_OFFSET));
	mmio_write_32(RTC_BASE+RTC_CCR_OFFSET, 0);

	// 1ms
	mmio_write_32(RTC_BASE+RTC_CMR_OFFSET, ms_count);
	mmio_write_32(RTC_BASE+RTC_CPSR_OFFSET, 32);
	mmio_write_32(RTC_BASE+RTC_CLR_OFFSET, 0);

	timer_meter_start();
	u32 start = timer_meter_get_ms();
	uartlog("start: %d\n", start);
	mmio_write_32(RTC_BASE+RTC_CCR_OFFSET, 0xd);
	uartlog("enable rtc CCR:%x\n", mmio_read_32(RTC_BASE+RTC_CCR_OFFSET));

	while(timer_meter_get_ms() - start < 15) {
		uartlog("CPCVR: %d CCVR: %d\n", mmio_read_32(RTC_BASE+RTC_CPCVR_OFFSET), 
						mmio_read_32(RTC_BASE+RTC_CCVR_OFFSET));
		mdelay(1);
	}


	return 0;
}

static struct cmd_entry test_cmd_list[] __attribute__((unused)) = {
	{ "ms", ms_test, 0, NULL },
	{ NULL, NULL, 0, NULL },
};

int testcase_uart(void)
{
	int i, ret = 0;

	printf("enter rtc test\n");

	reset_rtc();

	request_irq(RTC_INTR, rtc_irq_handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_MASK, "timer int", NULL);

	for (i = 0; i < ARRAY_SIZE(test_cmd_list) - 1; i++) {
		command_add(&test_cmd_list[i]);
	}
	cli_simple_loop();

	return ret;
}

#ifndef BUILD_TEST_CASE_all
int testcase_main()
{
	int ret = testcase_uart();
	uartlog("testcase rtc %s\n", ret?"failed":"passed");
	return ret;
}
#endif