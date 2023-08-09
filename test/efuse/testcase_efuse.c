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
	writel(SOFT_RESET_REG1,readl(SOFT_RESET_REG1)&~(1<<2));
	mdelay(1);
	writel(SOFT_RESET_REG1,readl(SOFT_RESET_REG1)|(1<<2));
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
	//efuse_embedded_write(4, (u32)0x80000000);
	//efuse_embedded_write(11, (u32)0x20505958);
	writel(SOFT_RESET_REG1,readl(SOFT_RESET_REG1)&~(1<<2));
	mdelay(1);
	writel(SOFT_RESET_REG1,readl(SOFT_RESET_REG1)|(1<<2));

	//efuse_info_t e;
	//efuse_info_init(&e);
	//e.bad_npus[0] = 10;
	//e.bad_npus[1] = 24;
	//memset(e.serdes_pcs, 0x00, sizeof(e.serdes_pcs));
	//strcpy(e.signature, "'s Chip");
	efuse_embedded_write(0x4,0x12345677);
	efuse_embedded_write(0x5,0x1234567f);

	writel(CLK_EN_REG0,readl(CLK_EN_REG0)&~(1<<29));
	mdelay(1);
	uartlog("cyx clk gating\n");
	writel(CLK_EN_REG0,readl(CLK_EN_REG0)|(1<<29));

	//efuse_info_write(&e);
	print_efuse();
	print_efuse_info();

	return 0;
}

module_testcase("1", testcase_efuse);
