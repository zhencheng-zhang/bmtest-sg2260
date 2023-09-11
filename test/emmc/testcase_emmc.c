#include <stdint.h>
#include <assert.h>
#include <string.h>
#include "boot_test.h"
#include "uart.h"
#include "system_common.h"
#include "mmio.h"
#include "bm_sdhci.h"
#include "mmc.h"
#include "pll.h"

#define FORCE_1LINE				0
#define CLOSE_INTERRUPT_TEST	0
#define CLOSE_ERASE_TEST		0

#define EMMC_CLK 100000000  //ck add
#define EMMC_BASE 0x0704002A000 // ck add

typedef int (*mmc_thandler_t)(struct mmc_host *host);

struct mmc_testcase {
	mmc_thandler_t handler;
	char *name;
};

static int mmc_phy_init(struct mmc_host *host);
static int mmc_pio_test(struct mmc_host *host);
static int mmc_sdma_test(struct mmc_host *host);
static int mmc_adma2_test(struct mmc_host *host);
static int mmc_adma3_test(struct mmc_host *host);
#if !CLOSE_INTERRUPT_TEST
static int mmc_interrupt_test(struct mmc_host *host);
#endif
static int mmc_1_4_line_transfer_test(struct mmc_host *host);
#ifdef PLATFORM_ASIC
static int mmc_tuning_test(struct mmc_host *host);
static int mmc_hs200_test(struct mmc_host *host);
#endif
#ifndef PLATFORM_PALLADIUM
#ifndef PLATFORM_FPGA
static int mmc_ddr52_test(struct mmc_host *host);
#endif
#endif

static int mmc_identify_test(struct mmc_host *host);
static int mmc_single_blk_rwe_test(struct mmc_host *host);
static int mmc_multi_blk_rwe_test(struct mmc_host *host);
static int mmc_register_rw_test(struct mmc_host *host);

static struct mmc_testcase mmc_testcases[] = {
	{ .handler = mmc_register_rw_test, .name = "Register read/write test" },
	{ .handler = mmc_phy_init, .name = "PHY initialization test" },
	{ .handler = mmc_identify_test, .name = "Card identification mode test" },

	
	{ .handler = mmc_1_4_line_transfer_test, .name = "1 and 4 dat lines transfer test" },
	{ .handler = mmc_pio_test, .name = "PIO test" },
	{ .handler = mmc_sdma_test, .name = "SDMA test" },
	{ .handler = mmc_adma2_test, .name = "ADMA2 test" },
	{ .handler = mmc_adma3_test, .name = "ADMA3 test" },
#if !CLOSE_INTERRUPT_TEST
	{ .handler = mmc_interrupt_test, .name = "Interrupt test" },
#endif
#ifdef PLATFORM_ASIC
	{ .handler = mmc_tuning_test, .name = "Tuning test" },
	{ .handler = mmc_hs200_test, .name = "HS200 test" },
#endif
#ifndef PLATFORM_PALLADIUM
#ifndef PLATFORM_FPGA
	{ .handler = mmc_ddr52_test, .name = "DDR52 test" },
#endif
#endif
	{ .handler = mmc_single_blk_rwe_test, .name = "Single block read/write test" },
	{ .handler = mmc_multi_blk_rwe_test, .name = "Multiple block read/write test" },
};

#define REG_TOP_FPLL_CTL	0x500100F0

#define TOP_VPLL_CTL     0x500100F8
#define TOP_ECLK_DIV     0x500080C4
#define TOP_SCLK_DIV     0x500080C8

#ifdef PLATFORM_ASIC
/* return value unit: Mhz */
static uint32_t bm_get_pll_clock(unsigned int vpll_ctl)
{
	uint32_t ref_freq = 50;
	uint32_t fbdiv, postdiv2, postdiv1, refdiv, vpll_freq;
	uint32_t reg;

	reg = mmio_read_32(vpll_ctl);
	refdiv = reg & 0x1f;
	postdiv1 = (reg >> 8) & 0x7;
	postdiv2 = (reg >> 12) & 0x7;
	fbdiv = (reg >> 16) & 0xfff;

	vpll_freq = ref_freq / refdiv * fbdiv / postdiv1/ postdiv2;

	pr_debug("%s vpll_freq:%u\n", __func__, vpll_freq);

	return vpll_freq;
}

/* parameter clock unit: Mhz */
static void bm_set_sdhci_base_clock(uint32_t clock)
{
	uint32_t clk_div;
	uint32_t vpll_freq;

	vpll_freq = bm_get_pll_clock(TOP_VPLL_CTL);
 
	clk_div = vpll_freq / clock;
 
	mmio_write_32(TOP_ECLK_DIV, (mmio_read_32(TOP_ECLK_DIV) & ~0x1f) | clk_div);
	mmio_write_32(TOP_SCLK_DIV, (mmio_read_32(TOP_SCLK_DIV) & ~0x1f) | clk_div);

/*
	uint32_t clk_div;
	uint32_t vpll_freq;
	uint32_t reg;


	vpll_freq = bm_get_pll_clock(REG_TOP_FPLL_CTL);

	//CLK_EMMC_DIV
	clk_div = vpll_freq / clock;
	reg = mmio_read_32(REG_TOP_CLK_EMMC_DIV);
	mmio_write_32(REG_TOP_CLK_EMMC_DIV, (reg & ~0x001F0000) | (clk_div << 16) | 0x08);

	//CLK_REF_ETH0_DIV
	clk_div = vpll_freq / 25;//25Mhz
	reg = mmio_read_32(REG_TOP_CLK_REF_ETH0_DIV);
	mmio_write_32(REG_TOP_CLK_REF_ETH0_DIV, (reg & ~0x001F0000) | (clk_div << 16) | 0x08);

	//CLK_100K_EMMC_DIV
	clk_div = 25 * 1000 / 100;//100K
	reg = mmio_read_32(REG_TOP_CLK_100K_EMMC_DIV);
	mmio_write_32(REG_TOP_CLK_100K_EMMC_DIV, (reg & ~0x001F0000) | (clk_div << 16) | 0x08);

	//CLK_AXI6_DIV
	clk_div = vpll_freq / 250;//250M
	reg = mmio_read_32(REG_TOP_CLK_AXI6_DIV);
	mmio_write_32(REG_TOP_CLK_AXI6_DIV, (reg & ~0x001F0000) | (clk_div << 16) | 0x08);


	//
	reg = mmio_read_32(REG_TOP_CLK_EN0);
	//CLK_100K_EMMC_EN
	reg  |= (1 << 4);
	//CLK_EMMC_EN
	reg  |= (1 << 3);
	//CLK_AXI_EMMC_EN
	reg  |= (1 << 2);
	//CLK_REF_ETH0_EN
	reg  |= (1 << 11);
	mmio_write_32(REG_TOP_CLK_EN0, reg);

	reg = mmio_read_32(REG_TOP_CLK_EN2);
	//CLK_AXI6_EN
	reg  |= (1 << 9);
	mmio_write_32(REG_TOP_CLK_EN2, reg);
*/
}
#endif
static int bm_mmc_get_clk()
{
	int clk;
#ifdef PLATFORM_ASIC
	bm_set_sdhci_base_clock(200);
	clk = 200 * 1000 * 1000;
	return clk;
#else
	clk = EMMC_CLK; 
	return clk;
#endif
}

#ifdef PLATFORM_PALLADIUM
#define MMC_FORCE_TRANS_FREQ	(26 * 1000 * 1000)
#else
#define MMC_FORCE_TRANS_FREQ	(0)
#endif

#define MMC_TEST_MULI_BLOCK_SZ	(SD_BLOCK_SIZE * 2)
static uint8_t g_buf[MMC_TEST_MULI_BLOCK_SZ] __attribute__((aligned (SD_BLOCK_SIZE)));
static int mmc_ewr_test(struct mmc_host *host, unsigned int start_lba, unsigned int size, uint8_t *buf);
static int mmc_phy_init(struct mmc_host *host)
{
	struct bm_mmc_params params;
#if 0
	uintptr_t addr;
#endif
	params.reg_base = EMMC_BASE;
	params.f_max = bm_mmc_get_clk();
	params.f_min = MMC_INIT_FREQ;
	params.flags = MMC_FLAG_SDMA_TRANS | MMC_FLAG_EMMC_SDR26 | MMC_FLAG_EMMC;
#if	FORCE_1LINE
	params.flags |= MMC_FLAG_BUS_WIDTH_1;
#endif
	params.force_trans_freq = MMC_FORCE_TRANS_FREQ;

	bm_mmc_setup_host(&params, host);
	bm_mmc_hw_reset(host);
#ifndef PLATFORM_PALLADIUM
	bm_mmc_phy_init(host);
#endif
	
#if 0
	addr = (mmio_read_32(params.reg_base+P_VENDOR_SPECIFIC_AREA)&0xFFF) + 0x08;
	addr += params.reg_base;
	printf("mmc_phy_init addr:0x%lx\n",addr);
	mmio_write_32(addr, mmio_read_32(addr) & ~0x01);
#endif	
	if (mmc_rescan(host)) {
		return -1;
	}

	if (mmc_ewr_test(host, 0, sizeof(g_buf), g_buf)) {
		return -1;
	}

	mmc_destory(host);

	return 0;
}

static int mmc_pio_test(struct mmc_host *host)
{
	struct bm_mmc_params params;

	params.reg_base = EMMC_BASE;
	params.f_max = bm_mmc_get_clk();
	params.f_min = MMC_INIT_FREQ;
	params.flags = MMC_FLAG_SDMA_TRANS | MMC_FLAG_EMMC_SDR26 | MMC_FLAG_EMMC;
#if	FORCE_1LINE
	params.flags |= MMC_FLAG_BUS_WIDTH_1;
#endif
	params.force_trans_freq = MMC_FORCE_TRANS_FREQ;

	bm_mmc_setup_host(&params, host);
	bm_mmc_hw_reset(host);
#ifndef PLATFORM_PALLADIUM
	bm_mmc_phy_init(host);
#endif
	if (mmc_rescan(host)) {
		return -1;
	}

	if (mmc_ewr_test(host, 0, sizeof(g_buf), g_buf)) {
		return -1;
	}

	mmc_destory(host);

	return 0;
}

static int mmc_sdma_test(struct mmc_host *host)
{
	struct bm_mmc_params params;

	params.reg_base = EMMC_BASE;
	params.f_max = bm_mmc_get_clk();
	params.f_min = MMC_INIT_FREQ;
	params.flags = MMC_FLAG_SDMA_TRANS | MMC_FLAG_EMMC_SDR26 | MMC_FLAG_EMMC;
#if	FORCE_1LINE
	params.flags |= MMC_FLAG_BUS_WIDTH_1;
#endif
	params.force_trans_freq = MMC_FORCE_TRANS_FREQ;

	bm_mmc_setup_host(&params, host);
	bm_mmc_hw_reset(host);
#ifndef PLATFORM_PALLADIUM
	bm_mmc_phy_init(host);
#endif
	if (mmc_rescan(host)) {
		return -1;
	}

	if (mmc_ewr_test(host, 0, sizeof(g_buf), g_buf)) {
		return -1;
	}

	mmc_destory(host);

	return 0;
}

static int mmc_adma2_test(struct mmc_host *host)
{
	struct bm_mmc_params params;

	params.reg_base = EMMC_BASE;
	params.f_max = bm_mmc_get_clk();
	params.f_min = MMC_INIT_FREQ;
	params.flags = MMC_FLAG_ADMA2_TRANS | MMC_FLAG_EMMC_SDR26 | MMC_FLAG_EMMC;
#if	FORCE_1LINE
	params.flags |= MMC_FLAG_BUS_WIDTH_1;
#endif
	params.force_trans_freq = MMC_FORCE_TRANS_FREQ;

	bm_mmc_setup_host(&params, host);
	bm_mmc_hw_reset(host);
#ifndef PLATFORM_PALLADIUM
	bm_mmc_phy_init(host);
#endif
	if (mmc_rescan(host)) {
		return -1;
	}

	if (mmc_ewr_test(host, 0, sizeof(g_buf), g_buf)) {
		return -1;
	}

	mmc_destory(host);

	return 0;
}

static int mmc_adma3_test(struct mmc_host *host)
{
	struct bm_mmc_params params;

	params.reg_base = EMMC_BASE;
	params.f_max = bm_mmc_get_clk();
	params.f_min = MMC_INIT_FREQ;
	params.flags = MMC_FLAG_ADMA3_TRANS | MMC_FLAG_EMMC_SDR26 | MMC_FLAG_EMMC;
#if	FORCE_1LINE
	params.flags |= MMC_FLAG_BUS_WIDTH_1;
#endif
	params.force_trans_freq = MMC_FORCE_TRANS_FREQ;

	bm_mmc_setup_host(&params, host);
	bm_mmc_hw_reset(host);
#ifndef PLATFORM_PALLADIUM
	bm_mmc_phy_init(host);
#endif
	if (mmc_rescan(host)) {
		return -1;
	}

	if (mmc_ewr_test(host, 0, sizeof(g_buf), g_buf)) {
		return -1;
	}

	mmc_destory(host);

	return 0;
}
#if !CLOSE_INTERRUPT_TEST

static int mmc_interrupt_test(struct mmc_host *host)
{
	struct bm_mmc_params params;

	params.reg_base = EMMC_BASE;
	params.f_max = bm_mmc_get_clk();
	params.f_min = MMC_INIT_FREQ;
	params.flags = MMC_FLAG_SDMA_TRANS | MMC_FLAG_EMMC_SDR26 | MMC_FLAG_EMMC | MMC_FLAG_SUPPORT_IRQ;
#if	FORCE_1LINE
	params.flags |= MMC_FLAG_BUS_WIDTH_1;
#endif
	params.force_trans_freq = MMC_FORCE_TRANS_FREQ;

	bm_mmc_setup_host(&params, host);
	bm_mmc_hw_reset(host);
#ifndef PLATFORM_PALLADIUM
	bm_mmc_phy_init(host);
#endif
	if (mmc_rescan(host)) {
		return -1;
	}

	if (mmc_ewr_test(host, 0, sizeof(g_buf), g_buf)) {
		return -1;
	}

	mmc_destory(host);

	return 0;
}
#endif
#ifdef PLATFORM_ASIC
static int mmc_tuning_test(struct mmc_host *host)
{
	struct bm_mmc_params params;

	params.reg_base = EMMC_BASE;
	params.f_max = bm_mmc_get_clk();
	params.f_min = MMC_INIT_FREQ;
	params.flags = MMC_FLAG_SDMA_TRANS | MMC_FLAG_EMMC_HS200 | MMC_FLAG_EMMC;
#if	FORCE_1LINE
	params.flags |= MMC_FLAG_BUS_WIDTH_1;
#endif
	params.force_trans_freq = MMC_FORCE_TRANS_FREQ;

	bm_mmc_setup_host(&params, host);
	bm_mmc_hw_reset(host);
#ifndef PLATFORM_PALLADIUM
	bm_mmc_phy_init(host);
#endif
	if (mmc_rescan(host)) {
		return -1;
	}

	mmc_ewr_test(host, 0, sizeof(g_buf), g_buf);

	if (mmc_execute_tuning(host->card)) {
		return -1;
	}

	if (mmc_ewr_test(host, 0, sizeof(g_buf), g_buf)) {
		return -1;
	}

	mmc_destory(host);

	return 0;
}
#endif
static int mmc_1_4_line_transfer_test(struct mmc_host *host)
{
	struct bm_mmc_params params;

	params.reg_base = EMMC_BASE;
	params.f_max = bm_mmc_get_clk();
	params.f_min = MMC_INIT_FREQ;
	params.flags = MMC_FLAG_SDMA_TRANS | MMC_FLAG_EMMC_SDR26 | MMC_FLAG_EMMC | MMC_FLAG_BUS_WIDTH_1;
	params.force_trans_freq = MMC_FORCE_TRANS_FREQ;

	bm_mmc_setup_host(&params, host);
	bm_mmc_hw_reset(host);
#ifndef PLATFORM_PALLADIUM
	bm_mmc_phy_init(host);
#endif
	if (mmc_rescan(host)) {
		return -1;
	}

	if (mmc_ewr_test(host, 0, sizeof(g_buf), g_buf)) {
		return -1;
	}
	mmc_destory(host);

	params.flags = MMC_FLAG_SDMA_TRANS | MMC_FLAG_EMMC_SDR26 | MMC_FLAG_EMMC;
#if	FORCE_1LINE
	params.flags |= MMC_FLAG_BUS_WIDTH_1;
#endif
	bm_mmc_setup_host(&params, host);
	bm_mmc_hw_reset(host);
#ifndef PLATFORM_PALLADIUM
	bm_mmc_phy_init(host);
#endif
	if (mmc_rescan(host)) {
		return -1;
	}

	if (mmc_ewr_test(host, 0, sizeof(g_buf), g_buf)) {
		return -1;
	}

	mmc_destory(host);
	return 0;
}
#ifdef PLATFORM_ASIC

static int mmc_hs200_test(struct mmc_host *host)
{
	struct bm_mmc_params params;

	params.reg_base = EMMC_BASE;
	params.f_max = bm_mmc_get_clk();
	params.f_min = MMC_INIT_FREQ;
	params.flags = MMC_FLAG_SDMA_TRANS | MMC_FLAG_EMMC_HS200 | MMC_FLAG_EMMC;
#if	FORCE_1LINE
	params.flags |= MMC_FLAG_BUS_WIDTH_1;
#endif
	params.force_trans_freq = MMC_FORCE_TRANS_FREQ;

	bm_mmc_setup_host(&params, host);
	bm_mmc_hw_reset(host);
#ifndef PLATFORM_PALLADIUM
	bm_mmc_phy_init(host);
#endif
	if (mmc_rescan(host)) {
		return -1;
	}

	if (mmc_ewr_test(host, 0, sizeof(g_buf), g_buf)) {
		return -1;
	}

	mmc_destory(host);

	return 0;
}
#endif
#ifndef PLATFORM_PALLADIUM
#ifndef PLATFORM_FPGA
static int mmc_ddr52_test(struct mmc_host *host)
{
	struct bm_mmc_params params;

	params.reg_base = EMMC_BASE;
	params.f_max = bm_mmc_get_clk();
	params.f_min = MMC_INIT_FREQ;
	params.flags = MMC_FLAG_SDMA_TRANS | MMC_FLAG_EMMC_DDR52 | MMC_FLAG_EMMC;
#if	FORCE_1LINE
	params.flags |= MMC_FLAG_BUS_WIDTH_1;
#endif
	params.force_trans_freq = MMC_FORCE_TRANS_FREQ;

	bm_mmc_setup_host(&params, host);
	bm_mmc_hw_reset(host);
#ifndef PLATFORM_PALLADIUM
	bm_mmc_phy_init(host);
#endif
	if (mmc_rescan(host)) {
		return -1;
	}

	if (mmc_ewr_test(host, 0, sizeof(g_buf), g_buf)) {
		return -1;
	}

	mmc_destory(host);

	return 0;
}
#endif
#endif

static int mmc_identify_test(struct mmc_host *host)
{
	struct bm_mmc_params params;

	params.reg_base = EMMC_BASE;
	params.f_max = bm_mmc_get_clk();
	params.f_min = MMC_INIT_FREQ;
	params.flags = MMC_FLAG_SDMA_TRANS | MMC_FLAG_EMMC_SDR26 | MMC_FLAG_EMMC| MMC_FLAG_BUS_WIDTH_1;
#if	FORCE_1LINE
	params.flags |= MMC_FLAG_BUS_WIDTH_1;
#endif
	params.force_trans_freq = MMC_FORCE_TRANS_FREQ;

	bm_mmc_setup_host(&params, host);
	bm_mmc_hw_reset(host);
#ifndef PLATFORM_PALLADIUM
	bm_mmc_phy_init(host);
#endif
	if (mmc_rescan(host)) {
		return -1;
	}

	printf("manfid:     0x%x\n", host->card->cid.manfid);
	printf("prod_name:  %s\n", host->card->cid.prod_name);
	printf("prv:        0x%x\n", host->card->cid.prv);
	printf("serial:     0x%x\n", host->card->cid.serial);
	printf("oemid:      0x%x\n", host->card->cid.oemid);
	printf("year:       0x%x\n", host->card->cid.year);
	printf("month:      0x%x\n", host->card->cid.month);
	printf("hwrev:      0x%x\n", host->card->cid.hwrev);
	printf("fwrev:      0x%x\n", host->card->cid.fwrev);

	mmc_destory(host);
	return 0;
}

static int mmc_single_blk_rwe_test(struct mmc_host *host)
{
	struct bm_mmc_params params;

	params.reg_base = EMMC_BASE;
	params.f_max = bm_mmc_get_clk();
	params.f_min = MMC_INIT_FREQ;
	params.flags = MMC_FLAG_SDMA_TRANS | MMC_FLAG_EMMC_SDR26 | MMC_FLAG_EMMC;
#if	FORCE_1LINE
	params.flags |= MMC_FLAG_BUS_WIDTH_1;
#endif
	params.force_trans_freq = MMC_FORCE_TRANS_FREQ;

	bm_mmc_setup_host(&params, host);
	bm_mmc_hw_reset(host);
#ifndef PLATFORM_PALLADIUM
	bm_mmc_phy_init(host);
#endif
	if (mmc_rescan(host)) {
		return -1;
	}

	if (mmc_ewr_test(host, 0, 0x200, g_buf)) {
		return -1;
	}

	mmc_destory(host);

	return 0;
}

static int mmc_multi_blk_rwe_test(struct mmc_host *host)
{
	struct bm_mmc_params params;

	params.reg_base = EMMC_BASE;
	params.f_max = bm_mmc_get_clk();
	params.f_min = MMC_INIT_FREQ;
	params.flags = MMC_FLAG_SDMA_TRANS | MMC_FLAG_EMMC_SDR26 | MMC_FLAG_EMMC;
#if	FORCE_1LINE
	params.flags |= MMC_FLAG_BUS_WIDTH_1;
#endif
	params.force_trans_freq = MMC_FORCE_TRANS_FREQ;

	bm_mmc_setup_host(&params, host);
	bm_mmc_hw_reset(host);
#ifndef PLATFORM_PALLADIUM
	bm_mmc_phy_init(host);
#endif
	if (mmc_rescan(host)) {
		return -1;
	}

	if (mmc_ewr_test(host, 0, sizeof(g_buf), g_buf)) {
		return -1;
	}

	mmc_destory(host);

	return 0;
}

static int mmc_register_rw_test(struct mmc_host *host)
{
	uint64_t base = EMMC_BASE;
	uint64_t addr = base;
	uint32_t count = 20;
	int i;

	mmio_write_16(base + SDHCI_CLK_CTRL, 0x3);
	mmio_write_32(base, 0x12345678);
	for (i = 0; i < count; i++) {
		if (i % 4 == 0) {
			if (i != 0)
				printf("\n");
			printf("0x%08lx\t", addr);
			addr += 0x10;
		}
		printf("%08x ", mmio_read_32(base + i * 4));
	}
	printf("\n");

	return 0;
}


#ifndef BUILD_TEST_CASE_all
static struct mmc_host g_host;
int testcase_emmc(void)
{
	int i;
	

	printf("\n------------ mmc test start ------------\n\n");

	for (i = 0; i < sizeof(mmc_testcases) / sizeof(mmc_testcases[0]); ++i) {
		printf("[%d] %s start\n", i, mmc_testcases[i].name);
		if (mmc_testcases[i].handler(&g_host)) {
			printf("[%d] %s failed\n", i, mmc_testcases[i].name);
			return -1;
		}
		printf("[%d] %s done\n\n", i, mmc_testcases[i].name);
	}

	printf("\n------------ mmc test done ------------\n");

	return 0;
}
#endif

static int mmc_ewr_test(struct mmc_host *host, unsigned int start_lba, unsigned int size, uint8_t *buf)
{
	int i;
	uint8_t pattern = 0x5a;

	pr_debug("\r\n================write/read lba=0x%x size=%d buf:%p================== \r\n", start_lba, size, buf);
	memset(buf, pattern, size);
	mmc_write_blocks(host, start_lba, (uintptr_t)buf, size);
	memset(buf, 0x0, size);
	mmc_read_blocks(host, start_lba, (uintptr_t)buf, size);

	for (i = 0; i < size; ++i) {
		if (buf[i] != pattern) {
			printf("write failed at i=%d 0x%x != 0x%x\n", i, buf[i], pattern);
			assert(0);
		}
	}
#if !CLOSE_ERASE_TEST

	pr_debug("\r\n================erase lba=0x%x size=%d buf:%p================== \r\n", start_lba, size, buf);
	mmc_erase_blocks(host, start_lba, size);
	mmc_read_blocks(host, start_lba, (uintptr_t)buf, size);
	for (i = 0; i < size; ++i) {
		if (buf[i] != 0x0 && buf[i] != 0xff) {
			printf("erase failed at i=%d erase result 0x%x \n", i, buf[i]);
			assert(0);
		}
	}
#endif

	pr_debug("\r\n======================== end ========================= \r\n");

	return 0;
}

#ifndef BUILD_TEST_CASE_ALL
int testcase_main()
{
  return testcase_emmc();
}
#endif
