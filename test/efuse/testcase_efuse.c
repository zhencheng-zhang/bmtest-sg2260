#include <stdio.h>
#include <boot_test.h>
#include <system_common.h>
#include "efuse.h"
#include "efuse_info.h"
#include "string.h"

static void print_efuse()
{
	u32 val,i;

	uartlog("0 efuse_ecc_read:\n");
	for(i=0; i < 14; i++){
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

	efuse_embedded_write(0x6,0x12567f);

	uartlog("\n 1 efuse had been changed by embedwrite, efuse_ecc_read:\n");
	for(i=0; i < 14; i++){
		val = efuse_ecc_read(i);
		uartlog(" %8x", val);
		if ((i + 1) % 4 == 0)
			uartlog("\n");
	}

	uartlog("\n 1 efuse had been changed by embedwrite, efuse_embedded_read:\n");
	for (i = 0; i < 16; i++) {
		val = efuse_embedded_read(i);
		uartlog(" %8x", val);
		if ((i + 1) % 4 == 0)
			uartlog("\n");
	}

#if 1
	// bit 6 in soft_reset_reg
	writel(SOFT_RESET_REG0,readl(SOFT_RESET_REG1)&~(1<<SOFT_RESET_EFUSE0_BIT));
	mdelay(1);
	writel(SOFT_RESET_REG0,readl(SOFT_RESET_REG1)|(1<<SOFT_RESET_EFUSE0_BIT));
#endif

	efuse_embedded_write(0x5,0x12567f);
	uartlog("\n 2 efuse_embedded_read:\n");
	for (i = 0; i < 128; i++) {
		val = efuse_embedded_read(i);
		uartlog(" %8x", val);
		if ((i + 1) % 4 == 0)
			uartlog("\n");
	}
	uartlog("\n 2 efuse_ecc_read:\n");
	for(i=0; i < 14; i++){
		val = efuse_ecc_read(i);
		uartlog(" %8x", val);
		if ((i + 1) % 4 == 0)
			uartlog("\n");
	}
}

static void print_efuse_info()
{
	efuse_info_t e;
	efuse_info_read(&e);
	uartlog("efuse_info_read:\n"
			"  bad_npu0: %d\n"
			"  bad_npu1: %d\n"
			"  serdes_pcs: 0x%02x%02x%02x%02x%02x\n"
			"  signature: %s\n",
			e.bad_npus[0],
			e.bad_npus[1],
			e.serdes_pcs[4],
			e.serdes_pcs[3],
			e.serdes_pcs[2],
			e.serdes_pcs[1],
			e.serdes_pcs[0],
			e.signature);
}

int testcase_efuse(void)
{
	writel(SOFT_RESET_REG0,readl(SOFT_RESET_REG0)&~(1<<SOFT_RESET_EFUSE0_BIT));
	mdelay(1);
	writel(SOFT_RESET_REG0,readl(SOFT_RESET_REG0)|(1<<SOFT_RESET_EFUSE0_BIT));

	uartlog("--after soft_reset_reg0---\n");

	efuse_embedded_write(0x4,0x12345677);
	efuse_embedded_write(0x5,0x1234567f);

	uartlog("--after efuse_embedded_write--\n");

	writel(CLK_EN_REG0,readl(CLK_EN_REG0)&~(1<<CLK_EN_EFUSE_BIT));
	mdelay(1);
	uartlog("cyx clk gating\n");
	writel(CLK_EN_REG0,readl(CLK_EN_REG0)|(1<<CLK_EN_EFUSE_BIT));

	uartlog("---In efuse case----\n");

	//efuse_info_write(&e);
	print_efuse();
	print_efuse_info();

	return 0;
}

#ifndef BUILD_TEST_CASE_ALL
int testcase_main()
{
  return testcase_efuse();
}
#endif
