#include "boot_test.h"
#include "cli.h"
#include "command.h"
#include "system_common.h"

static int gpio0_irq_handler(int irqn, void *priv)
{
	disable_irq(irqn);
	printf("enter gpio intr:%d\n", irqn);
	return 0;
}

static int gpio1_irq_handler(int irqn, void *priv)
{
	disable_irq(irqn);
	printf("enter gpio intr:%d\n", irqn);
	return 0;
}

static int gpio2_irq_handler(int irqn, void *priv)
{
	disable_irq(irqn);
	printf("enter gpio intr:%d\n", irqn);
	return 0;
}

int testcase_gpio(void)
{
	int ret;

	// testcase_gpio_cmd();
	printf("request gpio irq number:%d\n", GPIO0_INTR_FLAG);
	request_irq(GPIO0_INTR_FLAG, gpio0_irq_handler, 0, "gpio interrupt", NULL);

	printf("request gpio irq number:%d\n", GPIO1_INTR_FLAG);
	request_irq(GPIO1_INTR_FLAG, gpio1_irq_handler, 0, "gpio interrupt", NULL);

	printf("request gpio irq number:%d\n", GPIO2_INTR_FLAG);
	request_irq(GPIO2_INTR_FLAG, gpio2_irq_handler, 0, "gpio interrupt", NULL);


	// set gpio input
	writel(GPIO0_BASE + 0x4, 0x0);
	writel(GPIO1_BASE + 0x4, 0x0);
	writel(GPIO2_BASE + 0x4, 0x0);

	// clr gpio intr
	writel(GPIO0_BASE + 0x4c, 0xffffffff);
	writel(GPIO1_BASE + 0x4c, 0xffffffff);
	writel(GPIO2_BASE + 0x4c, 0xffffffff);

	// enable gpio intr
	writel(GPIO0_BASE + 0x30, 0xffffffff);
	writel(GPIO1_BASE + 0x30, 0xffffffff);
	writel(GPIO2_BASE + 0x30, 0xffffffff);

	// activity low
	writel(GPIO0_BASE + 0x3c, 0x0);
	writel(GPIO1_BASE + 0x3c, 0x0);
	writel(GPIO2_BASE + 0x3c, 0x0);

	mdelay(100);

	// activity high
	writel(GPIO0_BASE + 0x3c, 0xffffffff);
	writel(GPIO1_BASE + 0x3c, 0xffffffff);
	writel(GPIO2_BASE + 0x3c, 0xffffffff);

	mdelay(100);

	// disable gpio intr
	writel(GPIO0_BASE + 0x30, 0x0);
	writel(GPIO1_BASE + 0x30, 0x0);
	writel(GPIO2_BASE + 0x30, 0x0);

	do {
		ret = cli_simple_loop();
	} while (ret == 0);

	return 0;
}

#ifndef BUILD_TEST_CASE_ALL
int testcase_main()
{
	return testcase_gpio();
}
#endif
