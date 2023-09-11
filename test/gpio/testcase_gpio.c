#include "boot_test.h"
#include "cli.h"
#include "command.h"
#include "system_common.h"

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))

#define REG_GPIO0_BASE		0x07030009000
#define REG_GPIO1_BASE		0x0703000A000
#define REG_GPIO2_BASE		0x0703000B000
#define SOFT_RESET_REG0 	0x7030013000

#define GPIO_SWPORTA_DR 	0x00 // 数据寄存器（Data Register）
#define GPIO_SWPORTA_DDR 	0x04 // 数据方向寄存器（Data Direction Register）
#define GPIO_SWPORTA_CTL	0x08 // 控制寄存器（Control Register）
#define GPIO_INTEN 			0x30 // 中断使能寄存器（Interrupt Enable Register）
#define GPIO_INTMASK	 	0x34 // 中断屏蔽寄存器（Interrupt Mask Register）
// GPIO_INTTYPE_LEVEL和PIO_INT_POLITY两者配合工作最佳
#define GPIO_INTTYPE_LEVEL 	0x38 // 中断类型级别寄存器（Interrupt Type Level Register）
#define GPIO_INT_POLITY 	0x3c // 中断极性寄存器（Interrupt Polarity Register）
#define GPIO_INTSTATUS		0x40 // 中断状态寄存器（Interrupt Status Register）
#define GPIO_RAW_INTSTATUS	0x44 // 原始中断状态寄存器（Raw Interrupt Status Register）
#define GPIO_DEBOUNCE		0x48 // 去抖动寄存器（Debounce Register）
#define GPIO_PORTA_EOI		0x4c // 中断清除寄存器（End of Interrupt Register）
#define GPIO_EXT_PORTA		0x50 // 外部端口寄存器（External Port Register）
#define GPIO_LS_SYNC		0x60 // 低功耗同步寄存器（Low Power Synchronous Register）
#define GPIO_ID_CODE		0x64 // ID代码寄存器（ID Code Register）
#define GPIO_VER_ID_CODE	0x6C // 版本ID代码寄存器（Version ID Code Register）
#define GPIO_CONFIG_REG1	0x74 // 配置寄存器1（Configuration Register 1）
#define GPIO_CONFIG_REG2	0x70 // 配置寄存器2（Configuration Register 2）

#define GPIO0_INTR_FLAG 96
#define GPIO1_INTR_FLAG 97
#define GPIO2_INTR_FLAG 98

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

static int do_interrupt(int argc, char **argv)
{

	// testcase_gpio_cmd();
	printf("request gpio irq number:%d\n", GPIO0_INTR_FLAG);
	request_irq(GPIO0_INTR_FLAG, gpio0_irq_handler, 0, "gpio_0 interrupt", NULL);

	printf("request gpio irq number:%d\n", GPIO1_INTR_FLAG);
	request_irq(GPIO1_INTR_FLAG, gpio1_irq_handler, 0, "gpio_1 interrupt", NULL);

	printf("request gpio irq number:%d\n", GPIO2_INTR_FLAG);
	request_irq(GPIO2_INTR_FLAG, gpio2_irq_handler, 0, "gpio_2 interrupt", NULL);


	// set gpio input
	writel(REG_GPIO0_BASE + GPIO_SWPORTA_DDR, 0x0);
	writel(REG_GPIO1_BASE + GPIO_SWPORTA_DDR, 0x0);
	writel(REG_GPIO2_BASE + GPIO_SWPORTA_DDR, 0x0);

	// clr gpio intr
	writel(REG_GPIO0_BASE + GPIO_PORTA_EOI, 0xffffffff);
	writel(REG_GPIO1_BASE + GPIO_PORTA_EOI, 0xffffffff);
	writel(REG_GPIO2_BASE + GPIO_PORTA_EOI, 0xffffffff);

	// enable gpio intr
	writel(REG_GPIO0_BASE + GPIO_INTEN, 0xffffffff);
	writel(REG_GPIO1_BASE + GPIO_INTEN, 0xffffffff);
	writel(REG_GPIO2_BASE + GPIO_INTEN, 0xffffffff);

	// activity low
	writel(REG_GPIO0_BASE + GPIO_INT_POLITY, 0x0);
	writel(REG_GPIO1_BASE + GPIO_INT_POLITY, 0x0);
	writel(REG_GPIO2_BASE + GPIO_INT_POLITY, 0x0);

	mdelay(100);

	// activity high
	writel(REG_GPIO0_BASE + GPIO_INT_POLITY, 0xffffffff);
	writel(REG_GPIO1_BASE + GPIO_INT_POLITY, 0xffffffff);
	writel(REG_GPIO2_BASE + GPIO_INT_POLITY, 0xffffffff);

	mdelay(100);

	// disable gpio intr
	writel(REG_GPIO0_BASE + GPIO_INTEN, 0x0);
	writel(REG_GPIO1_BASE + GPIO_INTEN, 0x0);
	writel(REG_GPIO2_BASE + GPIO_INTEN, 0x0);

	// do {
	// 	ret = cli_simple_loop();
	// } while (ret == 0);
	uartlog("-----------test gpio_interrupt ending------------\n");

	return 0;
}

static int do_reset(int argc, char **argv)
{
	// test_reset
	writel(REG_GPIO0_BASE+GPIO_SWPORTA_DDR,0xffffffff);				//set direction output
	writel(REG_GPIO0_BASE+GPIO_SWPORTA_DR,0xffffffff);

	writel(REG_GPIO2_BASE+GPIO_SWPORTA_DDR,0);				        //set direction input
	writel(REG_GPIO2_BASE+GPIO_INTTYPE_LEVEL, 0xffffffff);

	// 执行软件复位
	writel(SOFT_RESET_REG0,readl(SOFT_RESET_REG0)&~(0x5<<17));
	writel(SOFT_RESET_REG0,readl(SOFT_RESET_REG0)|(0x5<<17));

	if((0xffffffff != readl(REG_GPIO0_BASE+GPIO_SWPORTA_DDR)) &&
		(0xffffffff != readl(REG_GPIO0_BASE+GPIO_SWPORTA_DR)) &&
		(0xffffffff != readl(REG_GPIO2_BASE+GPIO_INTTYPE_LEVEL)))
		uartlog("gpio_reset test pass,val=0x%x  0x%x 0x%x\n", readl(REG_GPIO0_BASE+GPIO_SWPORTA_DDR),
			readl(REG_GPIO0_BASE+GPIO_SWPORTA_DR),
			readl(REG_GPIO2_BASE+GPIO_INTTYPE_LEVEL));
	else
		uartlog("gpio_reset test failed \n ");

	return 0;
}

static int do_clk_gate(int argc, char **argv)
{
	int clk_enable = strtoul(argv[1], NULL, 10);
	writel(REG_GPIO0_BASE+GPIO_SWPORTA_DDR,0xffffffff);			//set direction output
	writel(REG_GPIO0_BASE+GPIO_SWPORTA_DR,0xffffffff);

	uartlog("gpio_clk_gate test entry, clk_enable=%d \n ",clk_enable);
	if(clk_enable == 1) {
		if(1) {//while(1){
			writel(SOFT_RESET_REG0,readl(SOFT_RESET_REG0)&~(0x1<<17));
			writel(SOFT_RESET_REG0,readl(SOFT_RESET_REG0)|(0x1<<17));
			mdelay(500);
			writel(REG_GPIO0_BASE+GPIO_SWPORTA_DR,0x0);
			mdelay(500);
			writel(REG_GPIO0_BASE+GPIO_SWPORTA_DR,0xffffffff);
		}
		uartlog("gpio_clk_gate test pass, GPIO_SWPORTA_DDR=0x%x, GPIO_SWPORTA_DR=0x%x\n",readl(REG_GPIO0_BASE+GPIO_SWPORTA_DDR),readl(REG_GPIO0_BASE+GPIO_SWPORTA_DR));
	}else if(clk_enable == 0) {
		writel(SOFT_RESET_REG0,readl(SOFT_RESET_REG0)&~(0x1<<17));
		writel(REG_GPIO0_BASE+GPIO_SWPORTA_DR,0xffff0000);
		uartlog("gpio_clk_gate test failed, GPIO_SWPORTA_DDR=0x%x, GPIO_SWPORTA_DR=0x%x\n",readl(REG_GPIO0_BASE+GPIO_SWPORTA_DDR),readl(REG_GPIO0_BASE+GPIO_SWPORTA_DR));
	}

	return 0;
}

static int do_input(int argc, char **argv)
{
	u32 gpio2_val;
	int cyc_num = 100;
	//dbg i2c should connect with i2c0  , set pinmux as gpio
	// 如何让两者GPIO0和GPIO1联系起来????
	writel(REG_GPIO0_BASE+GPIO_SWPORTA_DDR,0xffffffff);		//set  output
	writel(REG_GPIO2_BASE+GPIO_SWPORTA_DDR, 0);		//set direction input

	while(cyc_num > 0){

		// GPIO0负责写，GPIO2负责读
		writel(REG_GPIO0_BASE+GPIO_SWPORTA_DR,0);
		udelay(500);
		gpio2_val = readl(REG_GPIO2_BASE+GPIO_EXT_PORTA);
		uartlog("first gpio2_val=0x%x \n",gpio2_val);

		writel(REG_GPIO0_BASE+GPIO_SWPORTA_DR,0xffffffff);
		udelay(500);
		gpio2_val = readl(REG_GPIO2_BASE+GPIO_EXT_PORTA);
		uartlog("second gpio2_val=0x%x \n",gpio2_val);
		cyc_num--;
	}
	uartlog("-----------test input ending------------\n");
	return 0;
}

static int do_output(int argc, char **argv)
{
	u32 val = strtoul(argv[1], NULL, 16);
	int cyc_num = 100;

	writel(REG_GPIO0_BASE+GPIO_SWPORTA_DDR,0xffffffff);		//set direction output
	writel(REG_GPIO1_BASE+GPIO_SWPORTA_DDR,0xffffffff);		//set direction output
	writel(REG_GPIO2_BASE+GPIO_SWPORTA_DDR,0xffffffff);		//set direction output

#if 0
#ifndef PLATFORM_FPGA
	// 如何设置对应的pinmux???
	// set pinmux for uart and so on, test all gpios on palladium
	mmio_clrsetbits_32(PINMUXGPIO84_GPIO85,0x3<<20|0x3<<4,0x1<<20);
	mmio_clrsetbits_32(PINMUXGPIO86_GPIO87,0x3<<20|0x3<<4,0x1<<20|0x1<<4);
	mmio_clrsetbits_32(PINMUXGPIO88_GPIO89,0x3<<20|0x3<<4,0x1<<4);
	mmio_clrsetbits_32(PINMUXGPIO90,0x3<<4,0x1<<4);
#endif
#endif

	while(cyc_num > 0){
		writel(REG_GPIO0_BASE+GPIO_SWPORTA_DR,0);
		writel(REG_GPIO1_BASE+GPIO_SWPORTA_DR,0);
		writel(REG_GPIO2_BASE+GPIO_SWPORTA_DR,0);
		uartlog("do_output value=0x%x\n",val);
		udelay(500);
		writel(REG_GPIO0_BASE+GPIO_SWPORTA_DR,0xffffffff);
		writel(REG_GPIO1_BASE+GPIO_SWPORTA_DR,0xffffffff);
		writel(REG_GPIO2_BASE+GPIO_SWPORTA_DR,0xffffffff);
		uartlog("do_output value=0x%x\n",val);
		udelay(500);
		cyc_num--;
	}

	uartlog("do_output value=0x%x\n",val);
	uartlog("-----------test output ending------------\n");

	return 0;
}

static struct cmd_entry test_cmd_list[] __attribute__ ((unused)) = {
	{"do_interrupt", do_interrupt, 0, "do_interrupt"},
	{"do_reset", do_reset, 0, "do_reset"},
	{"do_clk_gate", do_clk_gate, 1, "do_clk_gate[clk_enable(0 or 1)]"},
	{"do_input", do_input, 0, "do_input"},
	{"do_output", do_output, 1, "do_output[any num]"},
	{NULL, NULL, 0, NULL}
};

int testcase_gpio(void)
{
	int i = 0;
	uartlog("-----------testcase_gpio entry------------\n");
	// gpio_pinmux_config();
	for(i = 0;i < ARRAY_SIZE(test_cmd_list) - 1;i++) {
		command_add(&test_cmd_list[i]);
	}

	cli_simple_loop();

	return 0;
}

#ifndef BUILD_TEST_CASE_ALL
int testcase_main()
{
	return testcase_gpio();
}
#endif
