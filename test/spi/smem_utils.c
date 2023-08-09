#include "boot_test.h"
#include "spi_flash.h"

//#define SPI_FLASH_PDL_START_ADDR  0xF0000
#define SPI_FLASH_PDL_START_ADDR  0

int spi_flash_program_pdl(void)
{
  uartlog("\n--%s\n", __func__);
  u64 spi_base = spi_flash_map_enable(1);
  uartlog("%s:%d\n", __func__, __LINE__);
  spi_flash_init(spi_base);

  u32 sector_num = DO_SPI_FW_PROG_BUF_SIZE / SPI_SECTOR_SIZE;
  if (DO_SPI_FW_PROG_BUF_SIZE % SPI_SECTOR_SIZE)
    sector_num++;

  do_sector_erase(spi_base, SPI_FLASH_PDL_START_ADDR, sector_num);
  int ret = 0;
  ret = spi_flash_program(spi_base, SPI_FLASH_PDL_START_ADDR, DO_SPI_FW_PROG_BUF_SIZE);

  return ret;
}


int spi_flash_program_image(u32 offset, u32 len)
{
  uartlog("\n--%s\n", __func__);
  u64 spi_base = spi_flash_map_enable(1);
  spi_flash_init(spi_base);

  u32 sector_num = len / SPI_SECTOR_SIZE;
  if (len % SPI_SECTOR_SIZE)
    sector_num++;

  do_sector_erase(spi_base, offset, sector_num);
  do_sector_erase(spi_base, 0x40000, sector_num);
  int ret = 0;
  ret = spi_flash_program(spi_base, offset, len);

  return ret;
}

void copy_globalmem_to_flash(u32 fa, u64 ga, u32 count)
{
  u64 spi_base = spi_flash_map_enable(0);
  spi_flash_init(spi_base);

  u32 size = count * 4;
  uartlog("%s fa:0x%08x, ga:0x%08lx, size:%d\n", __func__, fa, ga, size);
  u32 sector_num = size / SPI_SECTOR_SIZE;
  if (size % SPI_SECTOR_SIZE)
    sector_num++;
  do_sector_erase(spi_base, fa, sector_num);

  u32 offset = 0;
  u32 xfer_size = 0;
  while (offset < size) {
    if ((size - offset) > SPI_PAGE_SIZE)
      xfer_size = SPI_PAGE_SIZE;
    else
      xfer_size = size - offset;

    u8 data[SPI_PAGE_SIZE];
    memset(data, 0x00, SPI_PAGE_SIZE);
    for (int i = 0; i < xfer_size; i++) {
      data[i] = readb(GLOBAL_MEM_START_ADDR_ARM + ga + offset + i);
    }
#ifdef DEBUG
    dump_hex((char *)"count_rst", (void *)data, xfer_size);
#endif
    spi_flash_write_by_page(spi_base, fa + offset, data, xfer_size);
    offset += xfer_size;
  }
  uartlog("%s done!\n", __func__);
}

void copy_flash_to_globalmem(u64 ga, u32 fa, u32 count)
{
  u64 spi_base = spi_flash_map_enable(0);
  spi_flash_init(spi_base);

  u32 offset = 0;
  u32 xfer_size = 0;
  u32 size = count * 4;
  uartlog("%s fa:0x%08x, ga:0x%08lx, size:%d\n", __func__, fa, ga, size);
  while (offset < size) {
    if ((size - offset) > SPI_PAGE_SIZE)
      xfer_size = SPI_PAGE_SIZE;
    else
      xfer_size = size - offset;

    u8 data[SPI_PAGE_SIZE];
    memset(data, 0x00, SPI_PAGE_SIZE);
    spi_flash_read_by_page(spi_base, fa + offset, data, xfer_size);

    for (int i = 0; i < xfer_size; i++) {
      writeb(GLOBAL_MEM_START_ADDR_ARM + ga + offset + i, data[i]);
    }
#ifdef DEBUG
    dump_hex((char *)"count_rst", (void *)data, xfer_size);
#endif
    offset += xfer_size;
  }
  uartlog("%s done!\n", __func__);
}
