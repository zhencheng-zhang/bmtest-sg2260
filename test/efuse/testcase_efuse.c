#include <stdio.h>
#include <boot_test.h>
#include <system_common.h>
#include "efuse.h"
#include "efuse_info.h"
#include "string.h"
#include "command.h"
#include "cli.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static int print_efuse(int argc, char **argv)
{
	u32 val,i;

	uartlog("0 efuse_ecc_read:\n");
	for(i=0; i < 16; i++){
		val = efuse_ecc_read(i);
		uartlog(" %8x", val);
		if ((i + 1) % 4 == 0)
			uartlog("\n");
	}

	uartlog("\n 0 efuse_embedded_read:\n");
	for (i = 0; i < 16; i++) {
		val = efuse_embedded_read(i);
		uartlog(" %8x", val);
		if ((i + 1) % 4 == 0)
	 		uartlog("\n");
	}

	efuse_embedded_write(0x6,0xffffffff);
	efuse_embedded_write(0x7,0xffffffff);
	efuse_embedded_write(0x8,0xffffffff);

	uartlog("\n 1 efuse had been changed by embedwrite ((0x6,0xffffffff)), efuse_ecc_read:\n");
	for(i=0; i < 14; i++){
		val = efuse_ecc_read(i);
		uartlog(" %8x", val);
		if ((i + 1) % 4 == 0)
			uartlog("\n");
	}

	uartlog("\n 1 efuse had been changed by embedwrite ((0x6,0xffffffff)), efuse_embedded_read:\n");
	for (i = 0; i < 16; i++) {
		val = efuse_embedded_read(i);
		uartlog(" %8x", val);
		if ((i + 1) % 4 == 0)
			uartlog("\n");
	}

#if 0
	// bit 6 in soft_reset_reg
	writel(SOFT_RESET_REG0,readl(SOFT_RESET_REG0)&~(1<<SOFT_RESET_EFUSE_BIT));
	mdelay(1);
	writel(SOFT_RESET_REG0,readl(SOFT_RESET_REG0)|(1<<SOFT_RESET_EFUSE_BIT));
#endif

	efuse_embedded_write(0x5,0xffffffff);
	uartlog("\n 2 write((0x5,0xffffffff)) efuse_embedded_read:\n");
	for (i = 0; i < 16; i++) {
		val = efuse_embedded_read(i);
		uartlog(" %8x", val);
		if ((i + 1) % 4 == 0)
			uartlog("\n");
	}
	uartlog("\n 2 write((0x5,0x12567f)) efuse_ecc_read:\n");
	for(i=0; i <16; i++){
		val = efuse_ecc_read(i);
		uartlog(" %8x", val);
		if ((i + 1) % 4 == 0)
			uartlog("\n");
	}

	return 0;
}

static int embedded_write_test(int argc, char **argv)
{
	u32 addr = strtol(argv[1], NULL, 16);
	u32 val = strtol(argv[2], NULL, 16);

	efuse_embedded_write(addr, val);
	uartlog("Embedded write addr[%0x]: %0x\n", addr, val);

	return 0;
}

static int embedded_read_test(int argc, char **argv)
{
	u32 addr = strtol(argv[1], NULL, 16);

	uartlog("Embedded read from addr[%0x]: %0x\n", addr, efuse_embedded_read(addr));

	return 0;
}

// static void print_efuse_info()
// {
// 	efuse_info_t e;
// 	efuse_info_read(&e);
// 	uartlog("efuse_info_read:\n"
// 			"  bad_npu0: %d\n"
// 			"  bad_npu1: %d\n"
// 			"  serdes_pcs: 0x%02x%02x%02x%02x%02x\n"
// 			"  signature: %s\n",
// 			e.bad_npus[0],
// 			e.bad_npus[1],
// 			e.serdes_pcs[4],
// 			e.serdes_pcs[3],
// 			e.serdes_pcs[2],
// 			e.serdes_pcs[1],
// 			e.serdes_pcs[0],
// 			e.signature);
// }

static struct cmd_entry test_cmd_list[] __attribute__((unused)) = {
	{ "eb_write", embedded_write_test, 0, "embedded write [addr] [val]" },
	{ "eb_read", embedded_read_test, 0, "embedded read [addr]" },
	{ "print_efuse", print_efuse, 0, NULL },
	//{ "loop", do_loop, 3, "loop port master/slave baudrate" },
	{ NULL, NULL, 0, NULL },
};

int testcase_efuse(void)
{
	int i, ret = 0;

	printf("enter watchdog test\n");

	writel(SOFT_RESET_REG0,readl(SOFT_RESET_REG0)&~(1<<SOFT_RESET_EFUSE_BIT));
	mdelay(1);
	writel(SOFT_RESET_REG0,readl(SOFT_RESET_REG0)|(1<<SOFT_RESET_EFUSE_BIT));

	writel(CLK_EN_REG0,readl(CLK_EN_REG0)&~(1<<CLK_EN_EFUSE_BIT));
	mdelay(1);
	uartlog("cyx clk gating\n");
	writel(CLK_EN_REG0,readl(CLK_EN_REG0)|(1<<CLK_EN_EFUSE_BIT));

	uartlog("EFUSE_MD: %0x\n", readl(EFUSE_BASE));
	// wait BOOT_DONE in EFUSE_MD setiing to 1
	// while (1) {
	// 	if (readl(EFUSE_BASE) & (1<<BOOT_DONE_BIT))
	// 		break;
	// }


	for (i = 0; i < ARRAY_SIZE(test_cmd_list) - 1; i++) {
		command_add(&test_cmd_list[i]);
	}
	cli_simple_loop();

	return ret;
}

#ifndef BUILD_TEST_CASE_ALL
int testcase_main()
{
  return testcase_efuse();
}
#endif
