#include <stdio.h>
#include <string.h>
#include <boot_test.h>
#include <system_common.h>
#include "reg_spi.h"
#include "smem_utils.h"
#include "spi_flash.h"
#include "bm_spi_nand.h"
#include "command.h"
#include "cli.h"

#include "asm/encoding.h"
#include "asm/csr.h"

// #define CSR_MHCR            0x7c1

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))

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

int spi_flash_spic_fifo_rw_test(int argc, char **argv)
{
  u32 data_dw = 0x76543210;
  u16 data_w = 0xabcd;
  u8 data_b = 0xef;

  writel(spi_base + REG_BM1680_SPI_FIFO_PT, 0);    //do flush FIFO before test

  writel(spi_base + REG_BM1680_SPI_FIFO_PORT, data_dw);
  uartlog("data_dw filled 0x%x, fifo pt: 0x%08x\n", data_dw, readl(spi_base + REG_BM1680_SPI_FIFO_PT));

  writew(spi_base + REG_BM1680_SPI_FIFO_PORT, data_w);
  uartlog("data_w filled 0x%x, fifo pt: 0x%08x\n", data_w, readl(spi_base + REG_BM1680_SPI_FIFO_PT));

  writeb(spi_base + REG_BM1680_SPI_FIFO_PORT, data_b);
  uartlog("data_b filled 0x%x, fifo pt: 0x%08x\n", data_b, readl(spi_base + REG_BM1680_SPI_FIFO_PT));
  // data_b = 0x89;
  // writeb(spi_base + REG_BM1680_SPI_FIFO_PORT, data_b);
  // uartlog("data_b filled 0x%x, fifo pt: 0x%08x\n", data_b, readl(spi_base + REG_BM1680_SPI_FIFO_PT));

  // data_b = readb(spi_base + REG_BM1680_SPI_FIFO_PORT);
  // uartlog("data_b read 0x%x, fifo pt: 0x%08x\n", data_b, readl(spi_base + REG_BM1680_SPI_FIFO_PT));
  data_b = readb(spi_base + REG_BM1680_SPI_FIFO_PORT);
  uartlog("data_b read 0x%x, fifo pt: 0x%08x\n", data_b, readl(spi_base + REG_BM1680_SPI_FIFO_PT));

  data_w = readw(spi_base + REG_BM1680_SPI_FIFO_PORT);
  uartlog("data_w read 0x%x, fifo pt: 0x%08x\n", data_w, readl(spi_base + REG_BM1680_SPI_FIFO_PT));

  data_dw = readl(spi_base + REG_BM1680_SPI_FIFO_PORT);
  uartlog("data_dw read 0x%x, fifo pt: 0x%08x\n", data_dw, readl(spi_base + REG_BM1680_SPI_FIFO_PT));

  writel(spi_base + REG_BM1680_SPI_FIFO_PT, 0);    //do flush FIFO after test

  // uartlog("%s done!\n", __func__);

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

    if (ret != 0) {
      uartlog("page program test memcmp failed: 0x%08x\n", ret);
      dump_hex((char *)"wdata", (void *)wdata, SPI_PAGE_SIZE);
      dump_hex((char *)"rdata", (void *)rdata, SPI_PAGE_SIZE);
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
  u32 size = SPI_MAX_SIZE;
  u8 *wdata = (u8 *)DO_SPI_FW_PROG_BUF_ADDR;
  u32 sector_addr = 0x0;

  do_sector_erase(spi_base, sector_addr, 16*256);

  while (off < size) {
    if ((size - off) >= SPI_PAGE_SIZE)
      xfer_size = SPI_PAGE_SIZE;
    else
      xfer_size = size - off;
    spi_flash_write_by_page(spi_base, sector_addr+off, wdata + off, xfer_size);

    u8 rdata[SPI_PAGE_SIZE];
    memset(rdata, 0x0, SPI_PAGE_SIZE);
    spi_flash_read_by_page(spi_base, sector_addr+off, rdata, xfer_size);
    int ret = memcmp(rdata, wdata + off, SPI_PAGE_SIZE);
    if (ret != 0) {
      uartlog("page program test memcmp failed: 0x%08x\n", ret);
      dump_hex((char *)"wdata", (void *)wdata, xfer_size);
      dump_hex((char *)"rdata", (void *)rdata, xfer_size);
      return ret;
    }

    uartlog("page read compare ok @%d\n", off);
    off += xfer_size;
  }

  // uartlog("%s done!\n", __func__);
  return 0;
}

int spi_flash_program_test(void)
{
  // uartlog("%s\n", __func__);
  int err = 0;
  err = spi_flash_program_pdl();

  return err;
}

void print_16bytes(u64 start_addr)
{
  // int cnt = 0;
  u64 base = spi_base + start_addr/2;
  int i;
  for (i=0; i<16; i+=4) {
    u32 data = readl(base + i);

    int j;
    for (j=0; j<4; j++)
      uartlog("%02x\n", (data >> (j*8)) & 0xff);
    // uartlog(" %08x", data);
    // cnt ++;
    // if (cnt % 4 == 0)
    //   uartlog("\n");
  }
}

int spi_dmmr_r_test(int argc, char **argv)
{
  // set clk div 00, 01, 10, 11
  int i;
  for (i=0; i<4; i++) {
    // no read
    // clear and set
    writel(SPI_BASE+REG_BM1680_SPI_CTRL, readl(SPI_BASE + REG_BM1680_SPI_CTRL) & ~(0b11));
    writel(SPI_BASE+REG_BM1680_SPI_CTRL, readl(SPI_BASE + REG_BM1680_SPI_CTRL) | i);

    // 16M file
    uartlog("First 16 bytes: \n");
    print_16bytes(0);

    uartlog("Middle 16 bytes: \n");
    print_16bytes(5592405);

    uartlog("Last 16 bytes: \n");
    print_16bytes(5592405*2-30);
  }

  return 0;
}

static struct cmd_entry test_cmd_list[] __attribute__ ((unused)) = {
	{"id_check", spi_flash_id_check, 0, NULL},
  {"fifo_test", spi_flash_spic_fifo_rw_test, 1, NULL},
  {"read_test", spi_flash_read_test, 1, NULL},
  {"erase_test", spi_flash_erase_test, 1, NULL},
  {"write_test", spi_flash_write_test, 1, NULL},
  {"flash_test", spi_flash_rw_test, 1, NULL},
  {"dmmr_read_test", spi_dmmr_r_test, 1, NULL},
	{NULL, NULL, 0, NULL}
};

int testcase_spi(void)
{
	writel(CLK_EN_REG0,readl(CLK_EN_REG0)&~(1 << CLK_EN_SPI_BIT));
	mdelay(1);
	writel(CLK_EN_REG0,readl(CLK_EN_REG0)|(1 << CLK_EN_SPI_BIT));

	writel(SOFT_RESET_REG0,readl(SOFT_RESET_REG0)&~(1<<SOFT_RESET_SPI_BIT));
	mdelay(1);
	writel(SOFT_RESET_REG0,readl(SOFT_RESET_REG0)|(1<<SOFT_RESET_SPI_BIT));

  spi_flash_init(spi_base);
  
  uartlog("SPI_CTRL: %0x\n", readl(SPI_BASE + REG_BM1680_SPI_CTRL));

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