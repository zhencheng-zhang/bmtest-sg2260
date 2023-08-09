#include <stdio.h>
#include <string.h>
#include <boot_test.h>
#include <system_common.h>
#include "reg_spi.h"
#include "smem_utils.h"
#include "spi_flash.h"


#define SOFT_RESET_REG0 	0x50010c00
#define SOFT_RESET_REG1 	0x50010c04     //bit[5] is spi reset
#define CLK_EN_REG0 		0x50010800
#define CLK_EN_REG1 		0x50010804


int spi_flash_spic_fifo_rw_test(u64 spi_base)
{
  u32 data_dw = 0x76543210;
  u16 data_w = 0xabcd;
  u8 data_b = 0xef;
  uartlog("\n--spi ctrl fifo rw test\n");

  writel(spi_base + REG_BM1680_SPI_FIFO_PT, 0);    //do flush FIFO before test

  writel(spi_base + REG_BM1680_SPI_FIFO_PORT, data_dw);
  uartlog("data_dw filled 0x%x, fifo pt: 0x%08x\n", data_dw, readl(spi_base + REG_BM1680_SPI_FIFO_PT));

  writew(spi_base + REG_BM1680_SPI_FIFO_PORT, data_w);
  uartlog("data_w filled 0x%x, fifo pt: 0x%08x\n", data_w, readl(spi_base + REG_BM1680_SPI_FIFO_PT));

  writeb(spi_base + REG_BM1680_SPI_FIFO_PORT, data_b);
  uartlog("data_b filled 0x%x, fifo pt: 0x%08x\n", data_b, readl(spi_base + REG_BM1680_SPI_FIFO_PT));
  data_b = 0x89;
  writeb(spi_base + REG_BM1680_SPI_FIFO_PORT, data_b);
  uartlog("data_b filled 0x%x, fifo pt: 0x%08x\n", data_b, readl(spi_base + REG_BM1680_SPI_FIFO_PT));

  data_b = readb(spi_base + REG_BM1680_SPI_FIFO_PORT);
  uartlog("data_b read 0x%x, fifo pt: 0x%08x\n", data_b, readl(spi_base + REG_BM1680_SPI_FIFO_PT));
  data_b = readb(spi_base + REG_BM1680_SPI_FIFO_PORT);
  uartlog("data_b read 0x%x, fifo pt: 0x%08x\n", data_b, readl(spi_base + REG_BM1680_SPI_FIFO_PT));

  data_w = readw(spi_base + REG_BM1680_SPI_FIFO_PORT);
  uartlog("data_w read 0x%x, fifo pt: 0x%08x\n", data_w, readl(spi_base + REG_BM1680_SPI_FIFO_PT));

  data_dw = readl(spi_base + REG_BM1680_SPI_FIFO_PORT);
  uartlog("data_dw read 0x%x, fifo pt: 0x%08x\n", data_dw, readl(spi_base + REG_BM1680_SPI_FIFO_PT));

  writel(spi_base + REG_BM1680_SPI_FIFO_PT, 0);    //do flush FIFO after test

  uartlog("%s done!\n", __func__);

  return 0;
}

int spi_flash_rw_test(u64 spi_base)
{
  u32 sector_num = 1;
  u32 sector_addr = 0x0;
  do_sector_erase(spi_base, sector_addr, sector_num);

  u8 wdata[SPI_PAGE_SIZE];
  for (int i = 0; i < SPI_PAGE_SIZE; i++) {
    wdata[i] = 0x5a;//rand();
  }

  u32 count = 16;//(SPI_SECTOR_SIZE * sector_num) / SPI_PAGE_SIZE;

  for (int i = 0; i < count; i++) {
    u32 off = i * SPI_PAGE_SIZE;
    spi_flash_write_by_page(spi_base, sector_addr + off, wdata, SPI_PAGE_SIZE);

#if 1
	writel(CLK_EN_REG1,readl(CLK_EN_REG1)&~(0x4));
	//mdelay(1);
	uartlog(" clk gating\n");
	writel(CLK_EN_REG1,readl(CLK_EN_REG1)|0x4);

	//writel(SOFT_RESET_REG1,readl(SOFT_RESET_REG1)&~(1<<5));
	//mdelay(1);
	//writel(SOFT_RESET_REG1,readl(SOFT_RESET_REG1)|(1<<5));
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
  uartlog("%s done!\n", __func__);

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

  uartlog("%s done!\n", __func__);
  return 0;
}

int spi_flash_basic_test(void)
{
  u64 spi_base = spi_flash_map_enable(0);

  uartlog(" before spi flash init\n");
  spi_flash_init(spi_base);

  int ret = 0;
  ret |= spi_flash_id_check(spi_base);

  spi_flash_spic_fifo_rw_test(spi_base);

  ret |= spi_flash_rw_test(spi_base);
 // ret |= spi_flash_full_chip_scan(spi_base);

  writel(SOFT_RESET_REG1,readl(SOFT_RESET_REG1)&~(1<<5));
  mdelay(1);
  writel(SOFT_RESET_REG1,readl(SOFT_RESET_REG1)|(1<<5));

  spi_flash_set_dmmr_mode(spi_base, 1);

  uartlog("%s done!\n", __func__);

  return ret;
}

int spi_flash_program_test(void)
{
  uartlog("%s\n", __func__);
  int err = 0;
  err = spi_flash_program_pdl();

  return err;
}

int testcase_spi(void)
{
  int err = 0;

	writel(CLK_EN_REG1,readl(CLK_EN_REG1)&~(0x4));
	mdelay(1);
	writel(CLK_EN_REG1,readl(CLK_EN_REG1)|0x4);

	writel(SOFT_RESET_REG1,readl(SOFT_RESET_REG1)&~(1<<5));
	mdelay(1);
	writel(SOFT_RESET_REG1,readl(SOFT_RESET_REG1)|(1<<5));



  //err |= spi_flash_program_test();
 // uartlog("spi_flash_program_test   %s\n",err ? "failed!":"passed!");
  err |= spi_flash_basic_test();
  uartlog("spi_flash_basic_test   %s\n", err ? "failed!":"passed!");

  return err;
}

module_testcase("1", testcase_spi);
