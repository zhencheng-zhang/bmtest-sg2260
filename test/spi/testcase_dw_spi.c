#include <stdio.h>
#include <string.h>
#include <boot_test.h>
#include <system_common.h>
#include "command.h"
#include "cli.h"
#include "bm_spi_nand.h"

#include "asm/encoding.h"
#include "asm/csr.h"

#include "bm_dw_spi.h"

// #define CSR_MHCR            0x7c1

// #define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))

u32 spi_flash_model_support_list[] = { \
		SPI_ID_M25P128,  \
		SPI_ID_N25Q128,  \
		SPI_ID_W25Q128FV,\
		SPI_ID_GD25LB512ME, \
};

static u64 spi_base = SPI_BASE;

int spi_flash_id_check(int argc, char **argv)
{
	// uartlog("\n--%s\n", __func__);

	u32 flash_id = 0;

	flash_id = spi_flash_read_id(spi_base);
	u32 i = 0;
	for (i=0; i < sizeof(spi_flash_model_support_list)/sizeof(u32); i++) {
		if (flash_id == spi_flash_model_support_list[i]) {
		uartlog("read id test success, read val:0x%08x\n", flash_id);
		return 0;
		}
	}

	uartlog("read id check failed, read val:0x%08x\n", flash_id);
	return 0;
}

int spi_flash_read_test(int argc, char **argv)
{
	u32 sector_addr = 0x0;
	u8 rdata[SPI_PAGE_SIZE];
	memset(rdata, 0x0, SPI_PAGE_SIZE);

	spi_flash_read_by_page(spi_base, sector_addr, rdata, SPI_PAGE_SIZE);

	dump_hex((char *)"rdata", (void *)rdata, SPI_PAGE_SIZE);

	return 0;
}

int spi_flash_erase_test(int argc, char **argv)
{
	u32 sector_num = 1;
	u32 sector_addr = 0x0;
	do_sector_erase(spi_base, sector_addr, sector_num);

	// TODO: check erase: erase area is 0xff

	return 0;
}

int spi_flash_write_test(int argc, char **argv)
{
	u32 sector_num = 1;
	u32 sector_addr = 0x0;
	do_sector_erase(spi_base, sector_addr, sector_num);

	u8 wdata[SPI_PAGE_SIZE];
	for (int i = 0; i < SPI_PAGE_SIZE; i++) {
		wdata[i] = i & 0xff;//rand();
	}

	spi_flash_write_by_page(spi_base, sector_addr, wdata, SPI_PAGE_SIZE);

	return 0;
}

int spi_flash_rw_test(int argc, char **argv)
{
	u32 sector_num = 1;
	u32 sector_addr = 0x0;
	do_sector_erase(spi_base, sector_addr, sector_num);

	u8 wdata[SPI_PAGE_SIZE];
	for (int i = 0; i < SPI_PAGE_SIZE; i++) {
		wdata[i] = i & 0xff;//rand();
	}

	u32 count = 16;//(SPI_SECTOR_SIZE * sector_num) / SPI_PAGE_SIZE;

	for (int i = 0; i < count; i++) {
		u32 off = i * SPI_PAGE_SIZE;
		spi_flash_write_by_page(spi_base, sector_addr + off, wdata, SPI_PAGE_SIZE);

	#if 1
	// CE_CTRL  TRAN_NUM
		uartlog(" val=0x%x 0x%x 0x%x\n",readl(spi_base),readl(spi_base+0x4),readl(spi_base+0x14));
	#endif

		u8 rdata[SPI_PAGE_SIZE];
		memset(rdata, 0x0, SPI_PAGE_SIZE);
		spi_flash_read_by_page(spi_base, sector_addr + off, rdata, SPI_PAGE_SIZE);
		int ret = memcmp(rdata, wdata, SPI_PAGE_SIZE);

		dump_hex((char *)"wdata", (void *)wdata, SPI_PAGE_SIZE);
		dump_hex((char *)"rdata", (void *)rdata, SPI_PAGE_SIZE);

		if (ret != 0) {
		uartlog("page program test memcmp failed: 0x%08x\n", ret);
		return ret;
		}
		uartlog("page count %d rd wr cmp ok\n",i);
	}
	// uartlog("%s done!\n", __func__);

	return 0;
}

int spi_flash_full_chip_scan(u64 spi_base)
{
	u32 off = 0;
	u32 xfer_size = 0;
	u32 size = SPI_PAGE_SIZE * 32;  //8kB
	// u8 *wdata = (u8 *)DO_SPI_FW_PROG_BUF_ADDR;
	u32 sector_addr = 0x0;

	u8 wdata[SPI_PAGE_SIZE];
	for (int i = 0; i < SPI_PAGE_SIZE; i++) {
		wdata[i] = i & 0xff;//rand();
	}

	do_sector_erase(spi_base, sector_addr, 2);

	while (off < size) {
		if ((size - off) >= SPI_PAGE_SIZE)
		xfer_size = SPI_PAGE_SIZE;
		else
		xfer_size = size - off;
		spi_flash_write_by_page(spi_base, sector_addr+off, wdata, xfer_size);

		u8 rdata[SPI_PAGE_SIZE];
		memset(rdata, 0x0, SPI_PAGE_SIZE);
		spi_flash_read_by_page(spi_base, sector_addr+off, rdata, xfer_size);
		int ret = memcmp(rdata, wdata, SPI_PAGE_SIZE);
		if (ret != 0) {
		uartlog("page program test memcmp failed in %d [%x, %x]\n", ret, wdata[ret], rdata[ret]);
		dump_hex((char *)"wdata", (void *)wdata, xfer_size);
		dump_hex((char *)"rdata", (void *)rdata, xfer_size);
		return ret;
		}

		uartlog("page read compare ok @%d\n", off);
		uartlog("------------one page prog done------------\n");
		off += xfer_size;
	}

	// uartlog("%s done!\n", __func__);
	return 0;
}

int spi_flash_full_chip_test(int argc, char **argv)
{
	spi_flash_full_chip_scan(spi_base);

	return 0;
}

// int spi_flash_program_test(void)
// {
// 	// uartlog("%s\n", __func__);
// 	int err = 0;
// 	err = spi_flash_program_pdl();

// 	return err;
// }

static struct cmd_entry test_cmd_list[] __attribute__ ((unused)) = {
	{"id_check", spi_flash_id_check, 0, NULL},
	{"read_test", spi_flash_read_test, 1, NULL},
	{"erase_test", spi_flash_erase_test, 1, NULL},
	{"write_test", spi_flash_write_test, 1, NULL},
	{"flash_test", spi_flash_rw_test, 1, NULL},
	{"full_chip_test", spi_flash_full_chip_test, 1, NULL},
	{NULL, NULL, 0, NULL}
};

int testcase_spi(void)
{
	spi_init(1);

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
	return testcase_spi();
}
#endif