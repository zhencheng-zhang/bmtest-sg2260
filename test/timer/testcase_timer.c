#include <stdio.h>
#include "boot_test.h"
#include "system_common.h"
#include "timer.h"
#include "mmio.h"
#include "command.h"
#include "cli.h"

#define TEST_WITH_TNTERRUPT  	1

#define REG_TIMER_BASE			TIMER_BASE

#ifdef CONFIG_CHIP_SG2042
#define TIMER_INTR	 			100
#elif CONFIG_CHIP_SG2260
#define TIMER_INTR	 			30
#endif

#define REG_TIMER1_BASE				(REG_TIMER_BASE+0x00)
#define REG_TIMER2_BASE				(REG_TIMER_BASE+0x14)
#define REG_TIMER3_BASE				(REG_TIMER_BASE+0x28)
#define REG_TIMER4_BASE				(REG_TIMER_BASE+0x3C)
#define REG_TIMER5_BASE				(REG_TIMER_BASE+0x50)
#define REG_TIMER6_BASE				(REG_TIMER_BASE+0x64)
#define REG_TIMER7_BASE				(REG_TIMER_BASE+0x78)
#define REG_TIMER8_BASE				(REG_TIMER_BASE+0x8C)
#define REG_TIMERS_INTSTATUS		(REG_TIMER_BASE+0xA0)
#define REG_TIMERS_EOI				(REG_TIMER_BASE+0xA4)
#define REG_TIMERS_RAW_INTSTATUS	(REG_TIMER_BASE+0xA8)
#define REG_TIMERS_COMP_VERSION		(REG_TIMER_BASE+0xAC)
#define REG_TIMERN_LOADCOUNT2_BASE	(REG_TIMER_BASE+0xB0)

#define REG_TIMER1_LOADCONNT		REG_TIMER1_BASE
#define REG_TIMER2_LOADCONNT		REG_TIMER2_BASE
#define REG_TIMER3_LOADCONNT		REG_TIMER3_BASE
#define REG_TIMER4_LOADCONNT		REG_TIMER4_BASE
#define REG_TIMER5_LOADCONNT		REG_TIMER5_BASE
#define REG_TIMER6_LOADCONNT		REG_TIMER6_BASE
#define REG_TIMER7_LOADCONNT		REG_TIMER7_BASE
#define REG_TIMER8_LOADCONNT		REG_TIMER8_BASE

#define REG_TIMER1_LOADCONNT2		(REG_TIMERN_LOADCOUNT2_BASE+0x00)
#define REG_TIMER2_LOADCONNT2		(REG_TIMERN_LOADCOUNT2_BASE+0x04)
#define REG_TIMER3_LOADCONNT2		(REG_TIMERN_LOADCOUNT2_BASE+0x08)
#define REG_TIMER4_LOADCONNT2		(REG_TIMERN_LOADCOUNT2_BASE+0x0C)
#define REG_TIMER5_LOADCONNT2		(REG_TIMERN_LOADCOUNT2_BASE+0x10)
#define REG_TIMER6_LOADCONNT2		(REG_TIMERN_LOADCOUNT2_BASE+0x14)
#define REG_TIMER7_LOADCONNT2		(REG_TIMERN_LOADCOUNT2_BASE+0x18)
#define REG_TIMER8_LOADCONNT2		(REG_TIMERN_LOADCOUNT2_BASE+0x1C)

#define REG_TIMER1_CURRENT_VALUE		(REG_TIMER1_BASE+0x04)
#define REG_TIMER2_CURRENT_VALUE		(REG_TIMER2_BASE+0x04)
#define REG_TIMER3_CURRENT_VALUE		(REG_TIMER3_BASE+0x04)
#define REG_TIMER4_CURRENT_VALUE		(REG_TIMER4_BASE+0x04)
#define REG_TIMER5_CURRENT_VALUE		(REG_TIMER5_BASE+0x04)
#define REG_TIMER6_CURRENT_VALUE		(REG_TIMER6_BASE+0x04)
#define REG_TIMER7_CURRENT_VALUE		(REG_TIMER7_BASE+0x04)
#define REG_TIMER8_CURRENT_VALUE		(REG_TIMER8_BASE+0x04)


// bit[0]: Timer Enable
// bit[1]: Timer Mode
// bit[2]: Timer Interrupt Mask
// bit[3]: PWM
#define REG_TIMER1_CONTROL		(REG_TIMER1_BASE+0x08)
#define REG_TIMER2_CONTROL		(REG_TIMER2_BASE+0x08)
#define REG_TIMER3_CONTROL		(REG_TIMER3_BASE+0x08)
#define REG_TIMER4_CONTROL		(REG_TIMER4_BASE+0x08)
#define REG_TIMER5_CONTROL		(REG_TIMER5_BASE+0x08)
#define REG_TIMER6_CONTROL		(REG_TIMER6_BASE+0x08)
#define REG_TIMER7_CONTROL		(REG_TIMER7_BASE+0x08)
#define REG_TIMER8_CONTROL		(REG_TIMER8_BASE+0x08)


#define REG_TIMER1_EOI		(REG_TIMER1_BASE+0x0C)
#define REG_TIMER2_EOI		(REG_TIMER2_BASE+0x0C)
#define REG_TIMER3_EOI		(REG_TIMER3_BASE+0x0C)
#define REG_TIMER4_EOI		(REG_TIMER4_BASE+0x0C)
#define REG_TIMER5_EOI		(REG_TIMER5_BASE+0x0C)
#define REG_TIMER6_EOI		(REG_TIMER6_BASE+0x0C)
#define REG_TIMER7_EOI		(REG_TIMER7_BASE+0x0C)
#define REG_TIMER8_EOI		(REG_TIMER8_BASE+0x0C)

#define REG_TIMER1_INTSTATUS		(REG_TIMER1_BASE+0x10)
#define REG_TIMER2_INTSTATUS		(REG_TIMER2_BASE+0x10)
#define REG_TIMER3_INTSTATUS		(REG_TIMER3_BASE+0x10)
#define REG_TIMER4_INTSTATUS		(REG_TIMER4_BASE+0x10)
#define REG_TIMER5_INTSTATUS		(REG_TIMER5_BASE+0x10)
#define REG_TIMER6_INTSTATUS		(REG_TIMER6_BASE+0x10)
#define REG_TIMER7_INTSTATUS		(REG_TIMER7_BASE+0x10)
#define REG_TIMER8_INTSTATUS		(REG_TIMER8_BASE+0x10)

#define TIMER_CLK	(50 * 1000 * 1000)


#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))

#define PINMUXGPIO0						0x70300110f8
#define GPIO0DATA						0x50027000
#define GPIO0DIRECTION					0x50027004

static int us1, us2;
//static int need_loop;

 __attribute__ ((noinline))  void sleep(int count)
{
	int i = 0;

	for (i=0; i < count; i++) {
		;/*loop*/
	}
}

void BM1684_timer_delay(int delay, int num, int running_mode)
{
	// disable & user-defined count mode  ~0x01)|0x2
	// disable & free-running count mode  ~0x03)|0
	mmio_write_32((REG_TIMER1_CONTROL + 0x14*num),(mmio_read_32(REG_TIMER1_CONTROL + 0x14*num) & ~0x03)|0);//0x01)|0x2);	 //0x03)|0);
	// set count
	mmio_write_32((REG_TIMER1_LOADCONNT + 0x14*num), delay);

	

	us1 = timer_meter_get_us();
	//need_loop = 1;
	// enable
	mmio_write_32((REG_TIMER1_CONTROL + 0x14*num),mmio_read_32(REG_TIMER1_CONTROL + 0x14*num)  | 0x01 | running_mode << 1);

	uartlog("Init count: %0x, delay: %x, CR: %0x\n", mmio_read_32(REG_TIMER1_CURRENT_VALUE + 0x14*num), delay, mmio_read_32(REG_TIMER1_CONTROL + 0x14*num));

	//writel(CLK_EN_REG0, readl(CLK_EN_REG0)&~(1<<20));
	//mdelay(1);
	//writel(CLK_EN_REG0, readl(CLK_EN_REG0)|(1<<20));

	while (timer_meter_get_us() - us1 < 1500) {
		uartlog("current count: %0x\n", mmio_read_32(REG_TIMER1_CURRENT_VALUE + 0x14*num));
	}
#if !TEST_WITH_TNTERRUPT
	u32 curr_cnt1, curr_cnt2=0;
	while (1) {
		while ((mmio_read_32(REG_TIMER1_INTSTATUS + 0x14*num) == 0) &&
			(readl(REG_TIMERS_INTSTATUS) == 0) &&
			(readl(REG_TIMERS_RAW_INTSTATUS) == 0) );

	    us2 = timer_meter_get_us();
		uartlog("loop get int,us1=%d us2=%d,intn=0x%x, 0x%x 0x%x\n",us1, us2, readl(REG_TIMER1_INTSTATUS),
				readl(REG_TIMERS_INTSTATUS),readl(REG_TIMERS_RAW_INTSTATUS));
		mmio_read_32(REG_TIMER1_EOI + 0x14*num);
	}
#endif

}

static int reset_test(int argc, char **argv)
{
	uartlog("%s\n", __func__);
	u32 num;
	num = strtoul(argv[1], NULL, 10);

	if(num > 7) {
		uartlog("test timer, invalid args, num: 0~7\n");
		return 0;
	}

	writel(SOFT_RESET_REG0, readl(SOFT_RESET_REG0)&~(1<<13));
	sleep(1000);
	writel(SOFT_RESET_REG0, readl(SOFT_RESET_REG0)|(1<<13));

	u32 count;
	count =1 * TIMER_CLK/1;  //fpga 50Mclk  5* 1��
#ifdef	PLATFORM_PALLADIUM
	count = 1000 * 50;       //pld 50M clk�� 1000΢��
#endif

	mmio_write_32((REG_TIMER1_CONTROL + 0x14*num),(mmio_read_32(REG_TIMER1_CONTROL + 0x14*num) & ~0x03)|0);//0x01)|0x2);	 //0x03)|0);
	// set count
	mmio_write_32((REG_TIMER1_LOADCONNT + 0x14*num), count);
	// enable
	mmio_write_32((REG_TIMER1_CONTROL + 0x14*num),mmio_read_32(REG_TIMER1_CONTROL + 0x14*num)  | 0x01);

	uartlog("Init count: %0x\n", mmio_read_32(REG_TIMER1_CURRENT_VALUE + 0x14*num));

	writel(SOFT_RESET_REG0, readl(SOFT_RESET_REG0)&~(1<<13));
	sleep(1000);
	writel(SOFT_RESET_REG0, readl(SOFT_RESET_REG0)|(1<<13));

	uartlog("After reset count: %0x (check whether it's default value)\n", mmio_read_32(REG_TIMER1_CURRENT_VALUE + 0x14*num));

	uartlog("Continue running, check whether it's work well\n");
	mmio_write_32((REG_TIMER1_CONTROL + 0x14*num),(mmio_read_32(REG_TIMER1_CONTROL + 0x14*num) & ~0x03)|0);//0x01)|0x2);	 //0x03)|0);
	// set count
	mmio_write_32((REG_TIMER1_LOADCONNT + 0x14*num), count);
	mmio_write_32((REG_TIMER1_CONTROL + 0x14*num),mmio_read_32(REG_TIMER1_CONTROL + 0x14*num)  | 0x01);
	uartlog("Init count: %0x, delay: %x, CR: %0x\n", mmio_read_32(REG_TIMER1_CURRENT_VALUE + 0x14*num), count, mmio_read_32(REG_TIMER1_CONTROL + 0x14*num));
	us1 = timer_meter_get_us();
	while (timer_meter_get_us() - us1 < 1000) {
		uartlog("current count: %0x\n", mmio_read_32(REG_TIMER1_CURRENT_VALUE + 0x14*num));
	}

  return 0;
}

static int clk_gating_test(int argc, char **argv)
{
	// uartlog("%s\n", __func__);
	u32 num;
	num = strtoul(argv[1], NULL, 10);

	if(num > 7) {
		uartlog("test timer, invalid args, num: 0~7\n");
		return 0;
	}

	writel(SOFT_RESET_REG0, readl(SOFT_RESET_REG0)&~(1<<13));
	// sleep(1000);
	writel(SOFT_RESET_REG0, readl(SOFT_RESET_REG0)|(1<<13));

	int clk_bit = 12 + num;
	writel(CLK_EN_REG0, readl(CLK_EN_REG0)&~(1<<clk_bit));
	writel(CLK_EN_REG0, readl(CLK_EN_REG0)|(1<<clk_bit));

	u32 count;
	count =1 * TIMER_CLK/1;  //fpga 50Mclk  5* 1��
#ifdef	PLATFORM_PALLADIUM
	count = 1000 * 50;       //pld 50M clk�� 1000΢��
#endif
	mmio_write_32((REG_TIMER1_CONTROL + 0x14*num),(mmio_read_32(REG_TIMER1_CONTROL + 0x14*num) & ~0x03)|0);//0x01)|0x2);	 //0x03)|0);
	// set count
	mmio_write_32((REG_TIMER1_LOADCONNT + 0x14*num), count);
	// enable
	mmio_write_32((REG_TIMER1_CONTROL + 0x14*num),mmio_read_32(REG_TIMER1_CONTROL + 0x14*num)  | 0x01);

	// uartlog("Init count: %0x, clk_bit: %d\n", mmio_read_32(REG_TIMER1_CURRENT_VALUE + 0x14*num), clk_bit);

	int gating_time = 50;
	int ungating_time = 100;
	// const u64 gating_addr = 0x7050002000;

	us1 = timer_meter_get_us();
	uint32_t timeline = timer_meter_get_us() - us1;
	int gating = 0;
	while (timeline < 1000) {
		if (timeline > gating_time && timeline < ungating_time && gating == 0) {
			// gating
			uartlog("gating \n");
			gating = 1;
			writel(CLK_EN_REG0, readl(CLK_EN_REG0)&(~(1<<clk_bit)));

		} else if (timeline > ungating_time && gating == 1) {
			// ungating
			uartlog("ungating \n");
			gating = 0;
			writel(CLK_EN_REG0, readl(CLK_EN_REG0)|(1<<clk_bit));
		}
		uartlog("%0x\n", mmio_read_32(REG_TIMER1_CURRENT_VALUE + 0x14*num));
		timeline = timer_meter_get_us() - us1;
	}
	
  return 0;
}


#if TEST_WITH_TNTERRUPT
int timer_irq_handler(int irqn, void *priv)
{
    u32 int_status;
	int_status = readl(REG_TIMERS_RAW_INTSTATUS);

	uartlog("---In IRQ---\n");
	/*clear int*/
	mmio_read_32(REG_TIMERS_EOI);
	us2 = timer_meter_get_us();
	uartlog(" us1=%d us2=%d 0x%x 0x%x \n", us1, us2, readl(REG_TIMER1_INTSTATUS), readl(REG_TIMERS_INTSTATUS));
	uartlog("%s  irqn=0x%x int_status=0x%x  0x%x num=%d \n",__func__, irqn, int_status,readl(REG_TIMERS_RAW_INTSTATUS), *(u32 *)priv);
	return 0;
}
#endif

static int test_timer(int argc, char **argv)
{
	uartlog("%s\n", __func__);
	u32 num;
	num = strtoul(argv[1], NULL, 10);
	u32 running_mode = strtoul(argv[2], NULL, 10);
	if (running_mode != 1)
		running_mode = 0;

	if(num > 7) {
		uartlog("test timer, invalid args, num: 0~7\n");
		return 0;
	}

	writel(SOFT_RESET_REG0, readl(SOFT_RESET_REG0)&~(1<<13));
	sleep(1000);
	writel(SOFT_RESET_REG0, readl(SOFT_RESET_REG0)|(1<<13));

#if TEST_WITH_TNTERRUPT
	request_irq(TIMER_INTR, timer_irq_handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_MASK, "timer int", (void *)&num);
#endif

	u32 count;
	cpu_write32(PINMUXGPIO0,cpu_read32(PINMUXGPIO0) & ~(0x3<<20));
	writel(GPIO0DIRECTION, 0xffffffff);

	uartlog("FPGA check GPIO0 in J7 C4 11\n");
	count =1 * TIMER_CLK/1;  //fpga 50Mclk  5* 1��
#ifdef	PLATFORM_PALLADIUM
	count = 1000 * 50;       //pld 50M clk�� 1000΢��
#endif

	uartlog("count %d\n", count);
	writel(GPIO0DATA,0xffffffff);
	BM1684_timer_delay(count, num, running_mode);
	writel(GPIO0DATA,0x0);

  return 0;
}


static struct cmd_entry test_cmd_list[] __attribute__ ((unused)) = {
	{"timer", test_timer, 1, "timer [num] [running mode]"},
	{"reset_test", reset_test, 1, "reset_test [num]"},
	{"clk_gating_test", clk_gating_test, 1, "clk_gating_test [num]"},
	{NULL, NULL, 0, NULL}
};

int  testcase_timer (void) {
	int i = 0;
	for(i = 0;i < ARRAY_SIZE(test_cmd_list) - 1;i++) {
		command_add(&test_cmd_list[i]);
	}
	cli_simple_loop();

	return 0;
}

#ifndef BUILD_TEST_CASE_ALL
int testcase_main()
{
  return testcase_timer();
}
#endif