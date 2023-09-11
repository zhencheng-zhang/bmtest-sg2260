#include <stdio.h>
#include "boot_test.h"
#include "system_common.h"
#include "command.h"
#include "cli.h"


#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))

#define REG_PWM_BASE		0x0703000C000

#define	HLPERIOD0			0x0000
#define PERIOD0				0x0004
#define HLPERIOD1			0x0008
#define PERIOD1				0x000c

#define FREQ0NUM			0x0020
#define FREQ0DATA			0x0024
#define FREQ1NUM			0x0028
#define FREQ1DATA			0x002c
#define FREQ2DATA			0x0034
#define FREQ3DATA			0x003C

#define PWM_INDEX 8
#define FAN_INDEX 8

// new
#define PINMUX_RGMII0_MDIO_PWM0 0x7030011080
#define PINMUX_PWM3_FAN0 0x7030011088
#define PINMUX_FAN3_IIC0_SDA 0x7030011090


#define SOFT_RESET_REG0 	0x7030013000
#define SOFT_RESET_REG1 	0x7030013004
#define CLK_EN_REG0 		0x7030012000



static void sleep(int count)
{
	int i = 0;

	for (i=0; i < count; i++) {
		;/*loop*/
	}
}


int do_pwm(int argc, char **argv)
{
	u32 num, period, hlperiod;
	u32 a, b, c, d;

	num = strtoul(argv[1], NULL, 10);
	hlperiod = strtoul(argv[2], NULL, 10);
	period = strtoul(argv[3], NULL, 10);

	if(num == 0){
		writel(REG_PWM_BASE + PERIOD0, period);
		writel(REG_PWM_BASE + HLPERIOD0, hlperiod);
	}else if(num == 1){
		writel(REG_PWM_BASE + PERIOD1, period);
		writel(REG_PWM_BASE + HLPERIOD1, hlperiod);
	}
	a = readl(REG_PWM_BASE + PERIOD1);
	b = readl(REG_PWM_BASE + HLPERIOD1);
	c = readl(REG_PWM_BASE + PERIOD0);
	d = readl(REG_PWM_BASE + HLPERIOD0);
	uartlog("cxk test_pwm: a = %d , b = %d , c = %d, d = %d\n", a, b, c, d);
	uartlog("pwm%d Output %d%% duty cycle waveform, clk=pclk/%d\n", num, hlperiod * 100 / period, period);
	return 0;
}

int do_fan(int argc, char **argv)
{
	
	u64  fre0data, fre1data; // 存储从PWM模块读取数据
	int i = 0;
	u32 frenum, high, per;

	//num = strtoul(argv[1], NULL, 10);
	frenum = strtoul(argv[1], NULL, 10);
	high = strtoul(argv[2], NULL, 10);
	per = strtoul(argv[3], NULL, 10);
	writel(REG_PWM_BASE + HLPERIOD0, high);
	writel(REG_PWM_BASE + PERIOD0, per);
	// 10ns*100000 = 1ms(the time window width the smallest required)
	writel(REG_PWM_BASE + FREQ0NUM, frenum);

	// writel(REG_PWM_BASE + PERIOD1, 1000);
	// writel(REG_PWM_BASE + HLPERIOD1, 200);
	// writel(REG_PWM_BASE + FREQ1NUM, 100000);

	for(i=0; i<50; i++) {
	//we should wait for a moment to sample
#ifndef PLATFORM_PALLADIUM
		mdelay(1000);
#else
		mdelay(1);
#endif
	    fre0data = readl(REG_PWM_BASE + FREQ0DATA);
		fre1data = readl(REG_PWM_BASE + FREQ1DATA);
		uartlog("freq0data=%ld, freq1data=%ld\n", fre0data, fre1data);
	}

	mdelay(10);
	uartlog("HLPERIOD0=%d, PERIOD0=%d, FREQ0NUM=%d\n", readl(REG_PWM_BASE + HLPERIOD0), readl(REG_PWM_BASE + PERIOD0), readl(REG_PWM_BASE + FREQ0NUM));
	uartlog("freq0data=%ld, freq1data=%ld\n", fre0data, fre1data);
	return fre0data;
}


int pwm_fan_reset(int argc, char **argv)
{
	uartlog("enter pwm_fan_reset.........\n");
	writel(REG_PWM_BASE + HLPERIOD0, 500);
	writel(REG_PWM_BASE + PERIOD0, 1000);
	writel(REG_PWM_BASE + FREQ0NUM, 25000000);
	uartlog("SOFT_RESET_REG0 value_first = %x\n", readl(SOFT_RESET_REG0));
	writel(SOFT_RESET_REG0, readl(SOFT_RESET_REG0)&~(1<<20));
	uartlog("SOFT_RESET_REG0 value_second = %x\n", readl(SOFT_RESET_REG0));
#ifndef PLATFORM_PALLADIUM
	mdelay(2000);
#endif
	writel(SOFT_RESET_REG0, readl(SOFT_RESET_REG0)|(1<<20));
	uartlog("SOFT_RESET_REG0 value_third = %x\n", readl(SOFT_RESET_REG0));
	sleep(5000);
	if((500 != readl(REG_PWM_BASE + HLPERIOD0)) &&
		(1000 != readl(REG_PWM_BASE + PERIOD0)) &&
		(25000000 != readl(REG_PWM_BASE + FREQ0NUM)))
		uartlog("pwm_fan_reset test passed,val=%d  %d  %d \n",readl(REG_PWM_BASE + HLPERIOD0),
			readl(REG_PWM_BASE + PERIOD0),
			readl(REG_PWM_BASE + FREQ0NUM));
	else
		uartlog("pwm_fan_reset test failed\n");

	return 0;
}

// 测试PWM模块的时钟门控功能
#ifndef PLATFORM_FPGA
int pwm_clk_gate(int argc, char **argv)
{
	int clk_enable = strtoul(argv[1], NULL, 10);
	uartlog("pwm_clk_gate test entry, clk_enable=%d pinmux=0x%x\n", clk_enable, readl(PINMUX_PWM3_FAN0));
	writel(REG_PWM_BASE + PERIOD0, 100);
	writel(REG_PWM_BASE + HLPERIOD0, 50);

	if(clk_enable == 1){
		while(1) {
			writel(CLK_EN_REG0, readl(CLK_EN_REG0)&~(1<<28)); // 关闭PWM时钟
			sleep(50000);
			writel(CLK_EN_REG0, readl(CLK_EN_REG0)|(1<<28)); // 打开PWM时钟
			writel(REG_PWM_BASE + HLPERIOD0, 25);
			writel(REG_PWM_BASE + HLPERIOD1, 25);
			sleep(50000);
			break;
		}
		uartlog("pwm_clk_gate test pass, PERIOD0=%d  HLPERIOD0=%d \n",readl(REG_PWM_BASE + PERIOD0), readl(REG_PWM_BASE + HLPERIOD0));
	}else if(clk_enable == 0) {
		writel(CLK_EN_REG0, readl(CLK_EN_REG0)&~(1<<28));
		uartlog("pwm_clk_gate test failed, PERIOD0=%d  HLPERIOD0=%d \n",readl(REG_PWM_BASE + PERIOD0), readl(REG_PWM_BASE + HLPERIOD0));
	}
	return 0;
}

/*
int fan_clk_gate(int argc, char **argv)
{

	int clk_enable = strtoul(argv[1], NULL, 10);
	uartlog("fan_clk_gate test entry, clk_enable=%d \n", clk_enable);
	writel(REG_PWM_BASE + FREQ0NUM, 100000000);
	writel(REG_PWM_BASE + FREQ1NUM, 100000000);

	if(clk_enable == 1){
		while(1) {
			writel(CLK_EN_REG0, readl(CLK_EN_REG0)&~(1<<28));
			sleep(50000);
			writel(CLK_EN_REG0, readl(CLK_EN_REG0)|(1<<28));
			sleep(50000);
			uartlog("cxk gate freqdata0=%d freqdata1=%d\n",readl(REG_PWM_BASE+FREQ0DATA), readl(REG_PWM_BASE+FREQ1DATA));
			break;
		}
		uartlog("pwm_fan_reset test pass, PERIOD0=%d  HLPERIOD0=%d \n",readl(REG_PWM_BASE + PERIOD0), readl(REG_PWM_BASE + HLPERIOD0));
	}else if(clk_enable == 0) {
		writel(CLK_EN_REG0, readl(CLK_EN_REG0)&~(1<<28));
		while(1) {
			uartlog("cxk ungate freqdata0=%d freqdata1=%d\n",readl(REG_PWM_BASE+FREQ0DATA), readl(REG_PWM_BASE+FREQ1DATA));
			break;
		}
	}
	return 0;
}
*/

#endif


static struct cmd_entry test_cmd_list[] __attribute__ ((unused)) = {
	{"do_pwm", do_pwm, 3, "do_pwm [num period hlperiod;]"},
	{"do_fan", do_fan, 2, "do_fan [num freqnum]"},
	{"pwm_fan_reset", pwm_fan_reset, 0, "pwm_fan_reset"},
#ifndef PLATFORM_FPGA
	{"pwm_clk_gate", pwm_clk_gate, 1, "pwm_clk_gate"},
#endif
	{NULL, NULL, 0, NULL}
};


int testcase_pwm(void)
{
	int i = 0;
	uartlog("-----------testcase_pwm entry------------\n");
	/*set pinmux for fan0 fan1*/
	writel(PINMUX_RGMII0_MDIO_PWM0, readl(PINMUX_RGMII0_MDIO_PWM0)&~(0x3 << 20));
	writel(PINMUX_PWM3_FAN0, 0x0e100a00);
	writel(PINMUX_FAN3_IIC0_SDA, readl(PINMUX_FAN3_IIC0_SDA)|0x10);

	for(i = 0;i < ARRAY_SIZE(test_cmd_list) - 1;i++) {
		command_add(&test_cmd_list[i]);
	}
	cli_simple_loop();
	return 0;
}

#ifndef BUILD_TEST_CASE_ALL
int testcase_main()
{
  return testcase_pwm();
}
#endif

