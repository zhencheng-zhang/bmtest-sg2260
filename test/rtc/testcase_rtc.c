#include "testcase_rtc.h"
#include "mmio.h"
#include "system_common.h"
#include "timer.h"
#include "command.h"
#include "cli.h"

#define RTC_INTR 35

#define   RTC_CCVR_OFFSET       0x00
#define   RTC_CMR_OFFSET        0x04
#define   RTC_CLR_OFFSET        0x08
#define   RTC_CCR_OFFSET        0x0C
#define   RTC_STAT_OFFSET       0x10
#define   RTC_RSTAT_OFFSET      0x14
#define   RTC_EOI_OFFSET        0x18
#define   RTC_CPSR_OFFSET       0x20
#define   RTC_CPCVR_OFFSET      0x24

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

int rtc_irq_handler(int irqn, void *priv)
{
	uartlog("%s  irqn=0x%x end: %d\n",__func__, irqn, timer_meter_get_ms());
	return 0;
}

static int second_test(int argc, char **argv)
{
  mmio_write_32(RTC_BASE+RTC_CCR_OFFSET, 0);

  // 0.01s
  mmio_write_32(RTC_BASE+RTC_CPSR_OFFSET, 327);
  mmio_write_32(RTC_BASE+RTC_CMR_OFFSET, 1);

  timer_meter_start();
  u32 start = timer_meter_get_ms();
  uartlog("start: %d\n", start);
  mmio_write_32(RTC_BASE+RTC_CCR_OFFSET, 0xd);

  while(timer_meter_get_ms() - start < 15)
    uartlog("CPCVR: %d CCVR: %d\n", mmio_read_32(RTC_BASE+RTC_CPCVR_OFFSET), 
                                    mmio_read_32(RTC_BASE+RTC_CCVR_OFFSET));


  return 0;
}

static struct cmd_entry test_cmd_list[] __attribute__((unused)) = {
	{ "second", second_test, 0, NULL },
	{ NULL, NULL, 0, NULL },
};

int testcase_uart(void)
{
	int i, ret = 0;

	printf("enter rtc test\n");

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