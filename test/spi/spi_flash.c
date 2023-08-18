#include <stdio.h>
#include <string.h>
// #include "plat_def.h"
#include "reg_spi.h"
#include "spi_flash.h"
#include "timer.h"

//#define writel(a, v)		writel((u64)a, v)
//#define readl(a)		readl((u64)a)

// interrupt flag
static volatile int int_flag = 0;

void write_int_flag(int val)
{
  int_flag = val;
}

int read_int_flag(void)
{
  return int_flag;
}

int spi_irq_handler(int irqn, void *priv)
{
  void* spi_base = priv;
  /* avoid always in trap interrupt handler */
  writel(spi_base + REG_BM1680_SPI_INT_STS, readl(spi_base + REG_BM1680_SPI_INT_STS) & (~BM1680_SPI_INT_TRAN_DONE));

  uartlog("In spi_irq_handler, INT_EN:%x, INT_NUM: %d \n", readl(spi_base + REG_BM1680_SPI_INT_EN), SPI_INTR);
	return 0;
}

static inline u32 _check_reg_bits(volatile u32 *addr, u64 offset, u32 mask, u32 wait_loop)
{
  u32 try_loop = 0;
  volatile u32 reg_val;
  while (1){
    reg_val = *((volatile u32 *)((u8 *)addr + offset));
    // if (reg_val)
    //   uartlog("reg_val: 0x%8x\n", reg_val);
    if ((reg_val & mask) != 0) {
      *((volatile u32 *)((u8 *)addr + offset)) = reg_val & mask;
      uartlog("-----------check reg bit right\n");
      return reg_val & mask;
      // return 1;
    }
    if (wait_loop != 0) {
      try_loop++;    //wait_loop == 0 means wait for ever
      if (try_loop >= wait_loop) {
        break;      //timeout break
      }
    }
  }
  return 0;
}


u8 spi_non_data_tran(u64 spi_base, u8* cmd_buf, u32 with_cmd, u32 addr_bytes)
{
  u32* p_data = (u32*)cmd_buf;

  if (addr_bytes > 3) {
    uartlog("non-data: addr bytes should be less than 3 (%d)\n", addr_bytes);
    return -1;
  }

  /* init tran_csr */
  u32 tran_csr = 0;
  tran_csr = readl(spi_base + REG_BM1680_SPI_TRAN_CSR);
  tran_csr &= ~(BM1680_SPI_TRAN_CSR_TRAN_MODE_MASK
                  | BM1680_SPI_TRAN_CSR_ADDR_BYTES_MASK
                  | BM1680_SPI_TRAN_CSR_FIFO_TRG_LVL_MASK
                  | BM1680_SPI_TRAN_CSR_WITH_CMD);
  tran_csr |= (addr_bytes << BM1680_SPI_TRAN_CSR_ADDR_BYTES_SHIFT);
  tran_csr |= BM1680_SPI_TRAN_CSR_FIFO_TRG_LVL_1_BYTE;
  tran_csr |= (with_cmd) ? BM1680_SPI_TRAN_CSR_WITH_CMD : 0;

  writel(spi_base + REG_BM1680_SPI_FIFO_PT, 0);    //do flush FIFO before filling fifo

  writel(spi_base + REG_BM1680_SPI_FIFO_PORT, p_data[0]);

  /* issue tran */
  writel(spi_base + REG_BM1680_SPI_INT_STS, 0);   //clear all int
  tran_csr |= BM1680_SPI_TRAN_CSR_GO_BUSY;
  writel(spi_base + REG_BM1680_SPI_TRAN_CSR, tran_csr);

  /* wait tran done */
  u32 int_stat = _check_reg_bits((volatile u32*)spi_base, REG_BM1680_SPI_INT_STS,
                      BM1680_SPI_INT_TRAN_DONE, 100000);

  if (int_stat == 0) {
    uartlog("non data timeout, int stat: 0x%08x\n", int_stat);
    // return -1;
  }
  writel(spi_base + REG_BM1680_SPI_FIFO_PT, 0);    //should flush FIFO after tran

  return 0;
}

u8 spi_data_out_tran(u64 spi_base, u8* src_buf, u8* cmd_buf, u32 with_cmd, u32 addr_bytes, u32 data_bytes)
{
  u32* p_data = (u32*)cmd_buf;
  u32 cmd_bytes = addr_bytes + ((with_cmd) ? 1 : 0);

  if (data_bytes > 65535) {
    uartlog("data out overflow, should be less than 65535 bytes(%d)\n", data_bytes);
    return -1;
  }

  /* init tran_csr */
  u32 tran_csr = 0;
  tran_csr = readl(spi_base + REG_BM1680_SPI_TRAN_CSR);
  tran_csr &= ~(BM1680_SPI_TRAN_CSR_TRAN_MODE_MASK
                  | BM1680_SPI_TRAN_CSR_ADDR_BYTES_MASK
                  | BM1680_SPI_TRAN_CSR_FIFO_TRG_LVL_MASK
                  | BM1680_SPI_TRAN_CSR_WITH_CMD);
  tran_csr |= (addr_bytes << BM1680_SPI_TRAN_CSR_ADDR_BYTES_SHIFT);
  tran_csr |= (with_cmd) ? BM1680_SPI_TRAN_CSR_WITH_CMD : 0;
  tran_csr |= BM1680_SPI_TRAN_CSR_FIFO_TRG_LVL_8_BYTE;
  tran_csr |= BM1680_SPI_TRAN_CSR_TRAN_MODE_TX;

  writel(spi_base + REG_BM1680_SPI_FIFO_PT, 0);    //do flush FIFO before filling fifo
  if (with_cmd) {
    for (int i = 0; i < ((cmd_bytes - 1) / 4 + 1); i++) {
      writel(spi_base + REG_BM1680_SPI_FIFO_PORT, p_data[i]);
    }
  }

  /* issue tran */
  writel(spi_base + REG_BM1680_SPI_INT_STS, 0);   //clear all int
  writel(spi_base + REG_BM1680_SPI_TRAN_NUM, data_bytes);
  tran_csr |= BM1680_SPI_TRAN_CSR_GO_BUSY;
  writel(spi_base + REG_BM1680_SPI_TRAN_CSR, tran_csr);
  while ((readl(spi_base + REG_BM1680_SPI_FIFO_PT) & 0xf) != 0) {};   //wait for cmd issued

  /* fill data */
  p_data = (u32*)src_buf;
  u32 off = 0;
  u32 xfer_size = 0;
  while (off < data_bytes) {
    if ((data_bytes - off) >= BM1680_SPI_MAX_FIFO_DEPTH) {
      xfer_size = BM1680_SPI_MAX_FIFO_DEPTH;
    } else {
      xfer_size = data_bytes - off;
    }

    int wait = 0;
    while ((readl(spi_base + REG_BM1680_SPI_FIFO_PT) & 0xf) != 0) {
      wait++;
      if (wait > 10000000) {
        uartlog("wait to write FIFO timeout\n");
        return -1;
      }
    }

    for (int i = 0; i < ((xfer_size - 1) / 4 + 1); i++) {
      writel(spi_base + REG_BM1680_SPI_FIFO_PORT, p_data[off / 4 + i]);
    }
    off += xfer_size;
  }

  /* wait tran done */
  u32 int_stat = _check_reg_bits((volatile u32*)spi_base, REG_BM1680_SPI_INT_STS,
                      BM1680_SPI_INT_TRAN_DONE, 100000);
  if (int_stat == 0) {
    uartlog("data out timeout, int stat: 0x%08x\n", int_stat);
    // return -1;
  }
  writel(spi_base + REG_BM1680_SPI_FIFO_PT, 0);  //should flush FIFO after tran
  return 0;
}

u8 spi_data_in_tran(u64 spi_base, u8* dst_buf, u8* cmd_buf, u32 with_cmd, u32 addr_bytes, u32 data_bytes)
{
  u32* p_data = (u32*)cmd_buf;
  u32 cmd_bytes = addr_bytes + ((with_cmd) ? 1 : 0);

  if (data_bytes > 65535) {
    uartlog("data in overflow, should be less than 65535 bytes(%d)\n", data_bytes);
    return -1;
  }

  /* init tran_csr */
  u32 tran_csr = 0;
  tran_csr = readl(spi_base + REG_BM1680_SPI_TRAN_CSR);
  tran_csr &= ~(BM1680_SPI_TRAN_CSR_TRAN_MODE_MASK
                  | BM1680_SPI_TRAN_CSR_ADDR_BYTES_MASK
                  | BM1680_SPI_TRAN_CSR_FIFO_TRG_LVL_MASK
                  | BM1680_SPI_TRAN_CSR_WITH_CMD);
  tran_csr |= (addr_bytes << BM1680_SPI_TRAN_CSR_ADDR_BYTES_SHIFT);
  tran_csr |= (with_cmd) ? BM1680_SPI_TRAN_CSR_WITH_CMD : 0;
  tran_csr |= BM1680_SPI_TRAN_CSR_FIFO_TRG_LVL_8_BYTE;
  tran_csr |= BM1680_SPI_TRAN_CSR_TRAN_MODE_RX;

  writel(spi_base + REG_BM1680_SPI_FIFO_PT, 0);    //do flush FIFO before filling fifo
  if (with_cmd) {
    for (int i = 0; i < ((cmd_bytes - 1) / 4 + 1); i++) {
      writel(spi_base + REG_BM1680_SPI_FIFO_PORT, p_data[i]);
    }
  }

  // writel(spi_base + REG_BM1680_SPI_INT_EN, BM1680_SPI_INT_RD_FIFO_EN);

  /* issue tran */
  writel(spi_base + REG_BM1680_SPI_INT_STS, 0);   //clear all int
  writel(spi_base + REG_BM1680_SPI_TRAN_NUM, data_bytes);
  tran_csr |= BM1680_SPI_TRAN_CSR_GO_BUSY;
  writel(spi_base + REG_BM1680_SPI_TRAN_CSR, tran_csr);

  /* check rd int to make sure data out done and in data started */
  u32 int_stat = _check_reg_bits((volatile u32*)spi_base, REG_BM1680_SPI_INT_STS,
                      BM1680_SPI_INT_RD_FIFO, 10000000);
  if (int_stat == 0) {
    uartlog("no read FIFO int\n");
    // return -1;
  }

  /* get data */
  p_data = (u32*)dst_buf;
  u32 off = 0;
  u32 xfer_size = 0;
  while (off < data_bytes) {
    if ((data_bytes - off) >= BM1680_SPI_MAX_FIFO_DEPTH) {
      xfer_size = BM1680_SPI_MAX_FIFO_DEPTH;
    } else {
      xfer_size = data_bytes - off;
    }

    int wait = 0;
    while (readl(spi_base + REG_BM1680_SPI_FIFO_PT) != xfer_size) {
      wait++;
      if (wait > 10000000) {
        uartlog("wait to read FIFO timeout\n");
        return -1;
      }
    }
    for (int i = 0; i < ((xfer_size - 1) / 4 + 1); i++) {
      p_data[off / 4 + i] = readl(spi_base + REG_BM1680_SPI_FIFO_PORT);
    }
    off += xfer_size;
  }

  /* wait tran done */
  int_stat = _check_reg_bits((volatile u32*)spi_base, REG_BM1680_SPI_INT_STS,
                      BM1680_SPI_INT_TRAN_DONE, 100000);
  if (int_stat == 0) {
    uartlog("data in timeout, int stat: 0x%08x\n", int_stat);
    // return -1;
  }
  writel(spi_base + REG_BM1680_SPI_FIFO_PT, 0);  //should flush FIFO after tran
  return 0;
}

/*
 * spi_in_out_tran is a workaround fucntion for current 32-bit access to spic fifo:
 * AHB bus could only do 32-bit access to spic fifo, so cmd without 3-bytes addr will leave 3-byte
 * data in fifo, so set tx to mark that these 3-bytes data would be sent out.
 * So send_bytes should be 3 (wirte 1 dw into fifo) or 7(write 2 dw), get_bytes sould be the same value.
 * software would mask out unuseful data in get_bytes.
 */
u8 spi_in_out_tran(u64 spi_base, u8* dst_buf, u8* src_buf,  u32 with_cmd, u32 addr_bytes, u32 send_bytes, u32 get_bytes)
{
  u32* p_data = (u32*)src_buf;

  if (send_bytes != get_bytes) {
    uartlog("data in&out: get_bytes should be the same as send_bytes\n");
    return -1;
  }

  if ((send_bytes > BM1680_SPI_MAX_FIFO_DEPTH) || (get_bytes > BM1680_SPI_MAX_FIFO_DEPTH)) {
    uartlog("data in&out: FIFO will overflow\n");
    return -1;
  }

  /* init tran_csr */
  u32 tran_csr = 0;
  tran_csr = readl(spi_base + REG_BM1680_SPI_TRAN_CSR);
  tran_csr &= ~(BM1680_SPI_TRAN_CSR_TRAN_MODE_MASK
                  | BM1680_SPI_TRAN_CSR_ADDR_BYTES_MASK
                  | BM1680_SPI_TRAN_CSR_FIFO_TRG_LVL_MASK
                  | BM1680_SPI_TRAN_CSR_WITH_CMD);
  tran_csr |= (addr_bytes << BM1680_SPI_TRAN_CSR_ADDR_BYTES_SHIFT);
  tran_csr |= BM1680_SPI_TRAN_CSR_FIFO_TRG_LVL_1_BYTE;
  tran_csr |= BM1680_SPI_TRAN_CSR_WITH_CMD;
  tran_csr |= BM1680_SPI_TRAN_CSR_TRAN_MODE_TX;
  tran_csr |= BM1680_SPI_TRAN_CSR_TRAN_MODE_RX;

  writel(spi_base + REG_BM1680_SPI_FIFO_PT, 0);    //do flush FIFO before filling fifo
  u32 total_out_bytes = addr_bytes + send_bytes +((with_cmd) ? 1 : 0);
  // in spi_flash_read_id: total_out_bytes=4
  for (int i = 0; i < ((total_out_bytes - 1) / 4 + 1); i++) {
    writel(spi_base + REG_BM1680_SPI_FIFO_PORT, p_data[i]);
  }

  uartlog("----%s\n", __func__);

  /* issue tran */
  writel(spi_base + REG_BM1680_SPI_INT_STS, 0);   //clear all int

  writel(spi_base + REG_BM1680_SPI_TRAN_NUM, get_bytes);
  tran_csr |= BM1680_SPI_TRAN_CSR_GO_BUSY;
  writel(spi_base + REG_BM1680_SPI_TRAN_CSR, tran_csr);

  // trans cmd first
  /* wait tran done and get data */
  u32 int_stat = _check_reg_bits((volatile u32*)spi_base, REG_BM1680_SPI_INT_STS,
                      BM1680_SPI_INT_TRAN_DONE, 100000);
  
  if (int_stat == 0) {
    uartlog("data in timeout\n");
    // return -1;
  }

  p_data = (u32*)dst_buf;
  for (int i = 0; i < ((get_bytes - 1) / 4 + 1); i++) {
    p_data[i] = readl(spi_base + REG_BM1680_SPI_FIFO_PORT);
  }
  writel(spi_base + REG_BM1680_SPI_FIFO_PT, 0);  //should flush FIFO after tran

  return 0;
}

u8 spi_write_en(u64 spi_base)
{
  u8 cmd_buf[4];
  memset(cmd_buf, 0, sizeof(cmd_buf));

  cmd_buf[0] = SPI_CMD_WREN;
  spi_non_data_tran(spi_base, cmd_buf, 1, 0);

  return 0;
}

u8 spi_write_dis(u64 spi_base)
{
  u8 cmd_buf[4];
  memset(cmd_buf, 0, sizeof(cmd_buf));

  cmd_buf[0] = SPI_CMD_WRDI;
  spi_non_data_tran(spi_base, cmd_buf, 1, 0);

  return 0;
}

u32 spi_flash_read_id(u64 spi_base)
{
  u8 cmd_buf[4];
  u8 data_buf[4];
  memset(cmd_buf, 0, sizeof(cmd_buf));
  memset(data_buf, 0, sizeof(data_buf));

  cmd_buf[0] = SPI_CMD_RDID;
  cmd_buf[3] = 0;
  cmd_buf[2] = 0;
  cmd_buf[1] = 0;

  spi_in_out_tran(spi_base, data_buf, cmd_buf, 1, 0, 3, 3);
  u32 read_id = 0;
  read_id = (data_buf[2] << 16) | (data_buf[1] << 8) | (data_buf[0]);

  return read_id;
}

static u8 spi_read_status(u64 spi_base)
{
  u8 cmd_buf[4];
  u8 data_buf[4];
  memset(cmd_buf, 0, sizeof(cmd_buf));
  memset(data_buf, 0, sizeof(data_buf));

  cmd_buf[0] = SPI_CMD_RDSR;
  cmd_buf[3] = 0;
  cmd_buf[2] = 0;
  cmd_buf[1] = 0;
  spi_in_out_tran(spi_base, data_buf, cmd_buf, 1, 0, 3, 3);

  return data_buf[0];
}

u8 spi_reg_status(u64 spi_base, u8 cmd)
{
  u8 cmd_buf[4];
  u8 data_buf[4];
  memset(cmd_buf, 0, sizeof(cmd_buf));
  memset(data_buf, 0, sizeof(data_buf));

  cmd_buf[0] = cmd;
  cmd_buf[3] = 0;
  cmd_buf[2] = 0;
  cmd_buf[1] = 0;
  spi_in_out_tran(spi_base, data_buf, cmd_buf, 1, 0, 3, 3);

  return data_buf[0];
}

u8 spi_data_read(u64 spi_base, u8* dst_buf, u32 addr, u32 size)
{
  u8 cmd_buf[4];

  cmd_buf[0] = SPI_CMD_READ;
  cmd_buf[1] = ((addr) >> 16) & 0xFF;
  cmd_buf[2] = ((addr) >> 8) & 0xFF;
  cmd_buf[3] = (addr) & 0xFF;
  spi_data_in_tran(spi_base, dst_buf, cmd_buf, 1, 3, size);

  return 0;
}

u8 spi_flash_page_program(u64 spi_base, u8 *src_buf, u32 addr, u32 size)
{
  u8 cmd_buf[4];
  memset(cmd_buf, 0, sizeof(cmd_buf));

  cmd_buf[0] = SPI_CMD_PP;
  cmd_buf[1] = (addr >> 16) & 0xFF;
  cmd_buf[2] = (addr >> 8) & 0xFF;
  cmd_buf[3] = addr & 0xFF;

  spi_data_out_tran(spi_base, src_buf, cmd_buf, 1, 3, size);

  return 0;
}

void spi_flash_sector_erase(u64 spi_base, u32 addr)
{
  u8 cmd_buf[4];
  memset(cmd_buf, 0, sizeof(cmd_buf));

  cmd_buf[0] = SPI_CMD_SE;
  cmd_buf[1] = (addr >> 16) & 0xFF;
  cmd_buf[2] = (addr >> 8) & 0xFF;
  cmd_buf[3] = addr & 0xFF;
  spi_non_data_tran(spi_base, cmd_buf, 1, 3);

  return;
}

void spi_flash_bulk_erase(u64 spi_base)
{
  u8 cmd_buf[4];
  memset(cmd_buf, 0, sizeof(cmd_buf));

  cmd_buf[0] = SPI_CMD_BE;
  spi_non_data_tran(spi_base, cmd_buf, 1, 0);

  return;
}

int do_full_chip_erase(u64 spi_base)
{
  spi_write_en(spi_base);
  u8 spi_status = spi_read_status(spi_base);
  if ((spi_status & SPI_STATUS_WEL) == 0) {
    uartlog("write en failed, get status: 0x%02x\n", spi_status);
    return -1;
  }

  spi_flash_bulk_erase(spi_base);

  while(1) {
    u32 wait = 0;
    spi_status = spi_read_status(spi_base);
    if (((spi_status * SPI_STATUS_WIP) == 0) || (wait > 100000000000)) {
      uartlog("full chip erase done, get status: 0x%02x, wait: %d\n", spi_status, wait);
      break;
    }
    if ((wait++ % 100000) == 0)
      uartlog("device busy, get status: 0x%02x\n", spi_status);
  }

  return 0;
}

int do_sector_erase(u64 spi_base, u32 addr, u32 sector_num)
{
  u32 sector_addr = addr - (addr % SPI_SECTOR_SIZE);
  uartlog("do sector erase @0x%08x (sector_addr:0x%08x)\n", addr, sector_addr);

  for (int i = 0; i < sector_num; i++) {
    spi_write_en(spi_base);
    u32 spi_status = spi_read_status(spi_base);
    if ((spi_status & SPI_STATUS_WEL) == 0) {
      uartlog("write en failed, get status: 0x%02x i=%d\n", spi_status,i);
      return -1;
    }

    u32 offset = i * SPI_SECTOR_SIZE;
    spi_flash_sector_erase(spi_base, sector_addr + offset);
    u32 wait = 0;
    while(1) {
      spi_status = spi_read_status(spi_base);
      if (((spi_status & SPI_STATUS_WIP) == 0 ) || (wait > 10000000000)) {
        uartlog("sector erase done, get status: 0x%02x, wait: %d\n", spi_status, wait);
        break;
      }
      if ((wait++ % 100000) == 0)
        uartlog("device busy, get status: 0x%02x\n", spi_status);
    }
  }

  return 0;
}

int do_page_prog(u64 spi_base, u8 *src_buf, u32 addr, u32 size)
{
  if (size > SPI_PAGE_SIZE) {
    uartlog("size larger than a page\n");
    return -1;
  }

  if ((addr % SPI_PAGE_SIZE) != 0) {
    uartlog("addr not alignned to page\n");
    return -1;
  }

  spi_write_en(spi_base);

  u8 spi_status = spi_read_status(spi_base);
  if ((spi_status & SPI_STATUS_WEL) == 0) {
    uartlog("write en failed, get status: 0x%02x\n", spi_status);
    return -1;
  }

  if (spi_status != 0x02) {
    uartlog("spi status check failed, get status: 0x%02x\n", spi_status);
    return -1;
  }

  spi_flash_page_program(spi_base, src_buf, addr, size);

  u32 wait = 0;
  while(1) {
    spi_status = spi_read_status(spi_base);
    if (((spi_status & SPI_STATUS_WIP) == 0 ) || (wait > 10000000)) {
      uartlog("page prog done, get status: 0x%02x\n", spi_status);
      break;
    }
    wait++;
    if ((wait % 10000) == 0) {
      uartlog("device busy, get status: 0x%02x\n", spi_status);
    }
  }

  return 0;
}

int spi_flash_write_by_page(u64 spi_base, u32 fa, u8 *data, u32 size)
{
  u8 cmp_buf[SPI_PAGE_SIZE];
  memset(cmp_buf, 0x11, sizeof(cmp_buf));

  u8 page_num = size / SPI_PAGE_SIZE;
  if (size % SPI_PAGE_SIZE) {
    page_num++;
  }

  u32 offset = 0;
  for (int i = 0; i < page_num; i++) {
    offset = i * SPI_PAGE_SIZE;
    do_page_prog(spi_base, data + offset, fa + offset, SPI_PAGE_SIZE);

#ifndef DEBUG
    spi_data_read(spi_base, cmp_buf, fa + offset, SPI_PAGE_SIZE);
    int ret = memcmp(data, cmp_buf, SPI_PAGE_SIZE);
    if (ret != 0) {
      uartlog("page program test memcmp failed: 0x%08x\n", ret);
      //dump_hex((char *)"src_buf", (void *)data, SPI_PAGE_SIZE);
      //dump_hex((char *)"cmp_buf", (void *)cmp_buf, SPI_PAGE_SIZE);
      return ret;
    }else{
	    uartlog("page program test memcmp success: 0x%08x\n", ret);
    }
#endif
  }

  return 0;
}

void spi_flash_read_by_page(u64 spi_base, u32 fa, u8 *data, u32 size)
{
  u8 page_num = size / SPI_PAGE_SIZE;

  u32 offset = 0;
  for (int i = 0; i < page_num; i++) {
    offset = i * SPI_PAGE_SIZE;
    spi_data_read(spi_base, data + offset, fa + offset, SPI_PAGE_SIZE);
  }

  u32 remainder = size % SPI_PAGE_SIZE;
  if (remainder) {
    offset = page_num * SPI_PAGE_SIZE;
    spi_data_read(spi_base, data + offset, fa + offset, remainder);
  }
#ifdef DEBUG
  //dump_hex((char *)"cmp_buf", (void *)data, size);
#endif
}

void spi_flash_soft_reset(u64 spi_base)
{
  //SCK frequency = HCLK frequency / (2(SckDiv+ 1))
  //0x8C003 is default value, 0x200000 is softrst
  writel(spi_base + REG_BM1680_SPI_CTRL, readl(spi_base + REG_BM1680_SPI_CTRL) | 0x1<<21 | 0x3);
  //uartlog("%s:%d\n", __func__, __LINE__);
  return;
}

void spi_flash_set_dmmr_mode(u64 spi_base, u32 en)
{
  u32 reg_val = (en) ? BM1680_SPI_DMMR_EN : 0;

  writel(spi_base + REG_BM1680_SPI_DMMR, reg_val);
 // uartlog("%s:%d\n", __func__, __LINE__);
  return;
}

u64 spi_flash_map_enable(u8 enable)
{
  u64 spi_base = 0;
  //u32 reg = 0;
  if (enable) {
    //reg = readl(TOP_CTLR_BASE_ADDR + REG_BM1680_TOP_IP_EN);
    //reg &= ~BM1680_CHL_IP_EN_SF_REMAP_EN;
    //writel(TOP_CTLR_BASE_ADDR + REG_BM1680_TOP_IP_EN, reg);
#if 0
    spi_base = SPI_CTLR_BASE_ADDR; //0xFFF00000
#else
    spi_base = SPI_BASE; //SPI_CTLR_BASE_ADDR_REMAP; //0x44000000
#endif
  } else {
    //reg = readl(TOP_CTLR_BASE_ADDR + REG_BM1680_TOP_IP_EN);
    //reg |= BM1680_CHL_IP_EN_SF_REMAP_EN;
    //writel(TOP_CTLR_BASE_ADDR + REG_BM1680_TOP_IP_EN, reg);
    spi_base = SPI_BASE;//SPI_CTLR_BASE_ADDR_REMAP; //0x44000000
  }


  return spi_base;
}

void spi_flash_init(u64 spi_base)
{
  u32 tran_csr = 0;

  spi_flash_set_dmmr_mode(spi_base, 0);
  spi_flash_soft_reset(spi_base);

  //uartlog("%s:%d\n", __func__, __LINE__);

  /* conf spi controller regs */
  tran_csr |= (0x03 << BM1680_SPI_TRAN_CSR_ADDR_BYTES_SHIFT);
  tran_csr |= BM1680_SPI_TRAN_CSR_FIFO_TRG_LVL_4_BYTE;
  tran_csr |= BM1680_SPI_TRAN_CSR_WITH_CMD;
  writel(spi_base + REG_BM1680_SPI_TRAN_CSR, tran_csr);
  //uartlog("%s:%d\n", __func__, __LINE__);
  writel(spi_base + REG_BM1680_SPI_INT_EN, BM1680_SPI_INT_TRAN_DONE_EN);
  request_irq(SPI_INTR, spi_irq_handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_MASK, "spi int", (void *)spi_base);
#ifdef DEBUG
  printf("check spi reg con[0x%08x]: 0x%08x\n", spi_base + REG_BM1680_SPI_CTRL,
           readl(spi_base + REG_BM1680_SPI_CTRL));
  printf("check spi reg tran csr[0x%08x]: 0x%08x\n", spi_base + REG_BM1680_SPI_TRAN_CSR,
           readl(spi_base + REG_BM1680_SPI_TRAN_CSR));
#endif
}

int spi_flash_program(u64 spi_base, u32 sector_addr, u32 len)
{
  u32 off = 0;
  u32 xfer_size = 0;
  u32 size = len; //DO_SPI_FW_PROG_BUF_SIZE;
  u8 *fw_buf = (u8 *)DO_SPI_FW_PROG_BUF_ADDR;

  while (off < size) {
    if ((size - off) >= SPI_PAGE_SIZE)
      xfer_size = SPI_PAGE_SIZE;
    else
      xfer_size = size - off;

    uartlog("page prog (%d / %d)\n", off, size);
    if (do_page_prog(spi_base, fw_buf + off, sector_addr + off, xfer_size) != 0) {
      uartlog("page prog failed @0x%lx\n", spi_base + sector_addr + off);
      return -1;
    }

    u8 cmp_buf[SPI_PAGE_SIZE];
    memset(cmp_buf, 0x0, SPI_PAGE_SIZE);
    spi_data_read(spi_base, cmp_buf, sector_addr + off, xfer_size);
    int ret = memcmp(fw_buf + off, cmp_buf, xfer_size);
    if (ret != 0) {
      uartlog("memcmp failed\n");
      //dump_hex((char *)"fw_buf", (void *)(fw_buf + off), 16);
      //dump_hex((char *)"cmp_buf", (void *)cmp_buf, 32);
      return ret;
    }
    uartlog("page read compare ok @%d\n", off);
    off += xfer_size;
  }

  uartlog("--%s done!\n", __func__);

  return 0;
}