#include <common.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "timer.h"
#include "system_common.h"
#include "mmio.h"
#include "module_testcase.h"
#include "bm_dw_spi.h"
#include "dma.h"
#include "command.h"
#include "cli.h"
#include "system_common.h"

#ifdef CONFIG_USE_SPI_DMA
uint32_t default_tx[] = {
	0x01, 0x00, 0x40, 0x00, 0x00, 0x40, 0x82, 0x01, 0x00, 0x40, 0xff, 0xff,0xff, 0xff, 0xff, 0xff,
};
uint32_t default_rx[ARRAY_SIZE(default_tx)] = {0, };
#else

#if CONFIG_TEST_WITH_ADAU1372
uint8_t default_tx[] = {
	0x01, 0x00, 0x40, 0xff, 0xff,0xff, 0xff, 0xff, 0xff,
};
#else
uint8_t default_tx[] = {
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
	0x07, 0x08, 0x09, 0x10, 0x11, 0x12,
	0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
	0x19, 0x20, 0x21, 0x22, 0x23, 0x24,
	0x25, 0x26, 0x26, 0x28, 0x29, 0x30,
	0xF0, 0x0D,
};
#endif
uint8_t default_rx[ARRAY_SIZE(default_tx)] = {0, };
#endif

// dws->ip == PSSI
struct dw_spi {
	/* Current message transfer state info */
	u16			len;
	void		*tx;
	void		*tx_end;
	unsigned int tx_len;
	void		*rx;
	void		*rx_end;
	unsigned int rx_len;
	u8			n_bytes;
	u32			fifo_len;
	u32			reg_io_width;	/* DR I/O width in bytes */
};

static struct dw_spi dws;

bool loop_back = true;

static int spi_irq_handler(int irqn, void *priv)
{
	uartlog("In spi_irq_handler\n");

	return 0;
}

static void dw_spi_enable(bool enable)
{
	if (enable == true)
		SPI_WRITE(DW_SPI_SSIENR, 0x1);
	else
		SPI_WRITE(DW_SPI_SSIENR, 0x0);

	// udelay(5);

	// printf("%sabling SPI\n", enable ? "En" : "Dis");
}

void dw_spi_set_cs(int cs_bit, bool enable)
{
	if (enable)
		SPI_WRITE(DW_SPI_SER, BIT(cs_bit));
	else
		SPI_WRITE(DW_SPI_SER, 0);
}

static inline void dw_write_io_reg(u32 offset, u32 val)
{
	switch (dws.reg_io_width) {
	case 1:
		writeb(SPI_BASE + offset, val);
	case 2:
		writew(SPI_BASE + offset, val);
		break;
	case 4:
	default:
		writel(SPI_BASE + offset, val);
		break;
	}
}

static inline u32 dw_read_io_reg(u32 offset)
{
	switch (dws.reg_io_width) {
	case 1:
		return readb(SPI_BASE + offset);
	case 2:
		return readw(SPI_BASE + offset);
	case 4:
	default:
		return readl(SPI_BASE + offset);
	}
}

static inline bool dw_spi_ctlr_busy(void)
{
	return SPI_READ(DW_SPI_SR) & DW_SPI_SR_BUSY;
}

static u32 min(u32 a, u32 b)
{
	return a > b ? b : a;
}

static int dw_spi_wait_mem_op_done(void)
{
	int retry = DW_SPI_WAIT_RETRIES;

	while (dw_spi_ctlr_busy() && retry--)
		udelay(1);
		// spi_delay_exec(&delay, NULL);

	if (retry < 0) {
		uartlog("Mem op hanged up\n");
		return -1;
	}

	return 0;
}


/**
 * tmode: transmit mode
 * ndf: number of frames
*/
void dw_spi_update_config(int tmod, int ndf)
{
	SPI_WRITE(DW_SPI_CTRL0, SPI_READ(DW_SPI_CTRL0) & (~SPI_TMOD_MASK));
	SPI_WRITE(DW_SPI_CTRL0, SPI_READ(DW_SPI_CTRL0) | tmod << 8);

	SPI_WRITE(DW_SPI_CTRL1, SPI_READ(DW_SPI_CTRL1) & (~DW_SPI_NDF_MASK));
	SPI_WRITE(DW_SPI_CTRL1, SPI_READ(DW_SPI_CTRL1) | ndf ? ndf-1 : 0);
}

static inline void dw_spi_mask_intr(u32 mask)
{
	u32 new_mask;

	new_mask = SPI_READ(DW_SPI_IMR) & ~mask;
	SPI_WRITE(DW_SPI_IMR, new_mask);
}


/*
 * This disables the SPI controller, interrupts, clears the interrupts status
 * and CS, then re-enables the controller back. Transmit and receive FIFO
 * buffers are cleared when the device is disabled.
 */
static inline void dw_spi_reset_chip(void)
{
	dw_spi_enable(0);
	dw_spi_mask_intr(0xff);
	SPI_READ(DW_SPI_ICR);
	SPI_WRITE(DW_SPI_SER, 0);
	dw_spi_enable(1);
}

int dw_spi_check_status(bool raw)
{
	u32 irq_status;
	int ret = 0;

	if (raw)
		irq_status = SPI_READ(DW_SPI_RISR);
	else
		irq_status = SPI_READ(DW_SPI_ISR);

	if (irq_status & DW_SPI_INT_RXOI) {
		uartlog("RX FIFO overflow detected\n");
		ret = -1;
	}

	if (irq_status & DW_SPI_INT_RXUI) {
		uartlog("RX FIFO underflow detected\n");
		ret = -1;
	}

	if (irq_status & DW_SPI_INT_TXOI) {
		uartlog("TX FIFO overflow detected\n");
		ret = -1;
	}

	/* Generically handle the erroneous situation */
	if (ret) {
		dw_spi_reset_chip();
	}

	return ret;
}

static int dw_spi_write_then_read(void)
{
	u32 room, entries, sts;
	unsigned int len;
	u8 *buf;
	/*
	 * At initial stage we just pre-fill the Tx FIFO in with no rush,
	 * since native CS hasn't been enabled yet and the automatic data
	 * transmission won't start til we do that.
	 */
	len = min(dws.fifo_len, dws.tx_len);
	uartlog("tx len: %d\n", len);
	buf = dws.tx;
	while (len--)
		dw_write_io_reg(DW_SPI_DR, *buf++);

	/*
	 * After setting any bit in the SER register the transmission will
	 * start automatically. We have to keep up with that procedure
	 * otherwise the CS de-assertion will happen whereupon the memory
	 * operation will be pre-terminated.
	 */
	len = dws.tx_len - ((void *)buf - dws.tx);
	// TODO: cs_bit?
	dw_spi_set_cs(0, true);
	while (len) {
		entries = SPI_READ(DW_SPI_TXFLR);
		// uartlog("txflr: %d\n", entries);
		if (!entries) {
			uartlog("CS de-assertion on Tx\n");
			return -1;
		}
		room = min(dws.fifo_len - entries, len);
		for (; room; --room, --len)
			dw_write_io_reg(DW_SPI_DR, *buf++);
	}
	uartlog("0 txflr: %d   rxflr: %d\n", SPI_READ(DW_SPI_TXFLR), SPI_READ(DW_SPI_RXFLR));
	udelay(1);
	uartlog("1 txflr: %d   rxflr: %d\n", SPI_READ(DW_SPI_TXFLR), SPI_READ(DW_SPI_RXFLR));
	// uartlog("len = 0\n");
	// int retry = 10000;
	// while (dw_spi_ctlr_busy() && retry--);

	// if (retry < 0)
	// 	uartlog("Mem op hanged up\n");
	// uartlog("TX end\n");

	/*
	 * Data fetching will start automatically if the EEPROM-read mode is
	 * activated. We have to keep up with the incoming data pace to
	 * prevent the Rx FIFO overflow causing the inbound data loss.
	 */
	len = dws.rx_len;
	buf = dws.rx;
	// uartlog("rx len: %d\n", len);
	while (len) {
		entries = SPI_READ(DW_SPI_RXFLR);
		uartlog("rxflr %x\n", entries);
		if (!entries) {
			sts = SPI_READ(DW_SPI_RISR);
			uartlog("sts %x\n", sts);
			if (sts & DW_SPI_INT_RXOI) {
				uartlog("FIFO overflow on Rx\n");
				return -1;
			}
			// continue;
			return 0;
		}
		entries = min(entries, len);
		for (; entries; --entries, --len)
			*buf++ = dw_read_io_reg(DW_SPI_DR);
		uartlog("rx len: %d\n", len);
	}
	// uartlog("rx end\n");

	return 0;
}

static void dw_spi_stop_mem_op(void)
{
	dw_spi_enable(0);
	dw_spi_set_cs(0, false);
	dw_spi_enable(1);
}

/*
 * The SPI memory operation implementation below is the best choice for the
 * devices, which are selected by the native chip-select lane. It's
 * specifically developed to workaround the problem with automatic chip-select
 * lane toggle when there is no data in the Tx FIFO buffer. Luckily the current
 * SPI-mem core calls exec_op() callback only if the GPIO-based CS is
 * unavailable.
 */
static int dw_spi_exec_mem_op(enum spi_mem_data_dir dir)
{
	int ret;

	/*
	 * Collect the outbound data into a single buffer to speed the
	 * transmission up at least on the initial stage.
	 */
	// cmd and data have been gathered in a buffer
	// ret = dw_spi_init_mem_buf(dws, op);
	// if (ret)
	// 	return ret;

	/*
	 * DW SPI EEPROM-read mode is required only for the SPI memory Data-IN
	 * operation. Transmit-only mode is suitable for the rest of them.
	 */
	int tmode, ndf;
	if (dir == SPI_MEM_DATA_IN) {
		tmode = DW_SPI_CTRLR0_TMOD_EPROMREAD;
		ndf = dws.rx_len - 1;
	} else {
		tmode = DW_SPI_CTRLR0_TMOD_TO;
	}

	dw_spi_enable(0);

	dw_spi_update_config(tmode, ndf);

	dw_spi_mask_intr(0xff);

	dw_spi_enable(1);

	/*
	 * DW APB SSI controller has very nasty peculiarities. First originally
	 * (without any vendor-specific modifications) it doesn't provide a
	 * direct way to set and clear the native chip-select signal. Instead
	 * the controller asserts the CS lane if Tx FIFO isn't empty and a
	 * transmission is going on, and automatically de-asserts it back to
	 * the high level if the Tx FIFO doesn't have anything to be pushed
	 * out. Due to that a multi-tasking or heavy IRQs activity might be
	 * fatal, since the transfer procedure preemption may cause the Tx FIFO
	 * getting empty and sudden CS de-assertion, which in the middle of the
	 * transfer will most likely cause the data loss. Secondly the
	 * EEPROM-read or Read-only DW SPI transfer modes imply the incoming
	 * data being automatically pulled in into the Rx FIFO. So if the
	 * driver software is late in fetching the data from the FIFO before
	 * it's overflown, new incoming data will be lost. In order to make
	 * sure the executed memory operations are CS-atomic and to prevent the
	 * Rx FIFO overflow we have to disable the local interrupts so to block
	 * any preemption during the subsequent IO operations.
	 *
	 * Note. At some circumstances disabling IRQs may not help to prevent
	 * the problems described above. The CS de-assertion and Rx FIFO
	 * overflow may still happen due to the relatively slow system bus or
	 * CPU not working fast enough, so the write-then-read algo implemented
	 * here just won't keep up with the SPI bus data transfer. Such
	 * situation is highly platform specific and is supposed to be fixed by
	 * manually restricting the SPI bus frequency using the
	 * dws->max_mem_freq parameter.
	 */

	ret = dw_spi_write_then_read();

	/*
	 * Wait for the operation being finished and check the controller
	 * status only if there hasn't been any run-time error detected. In the
	 * former case it's just pointless. In the later one to prevent an
	 * additional error message printing since any hw error flag being set
	 * would be due to an error detected on the data transfer.
	 */
	if (!ret) {
		ret = dw_spi_wait_mem_op_done();
		if (!ret)
			ret = dw_spi_check_status(true);
		// uartlog("check statue\n");
	}

	dw_spi_stop_mem_op();

	return ret;
}

u8 spi_non_data_tran(u64 spi_base, u8* cmd_buf, u32 with_cmd, u32 addr_bytes)
{
	if (addr_bytes > 3) {
		uartlog("non-data: addr bytes should be less than 3 (%d)\n", addr_bytes);
		return -1;
	}

	dws.tx_len = addr_bytes + 1;
	dws.tx = cmd_buf;
	dws.rx_len = 0;

	dw_spi_exec_mem_op(SPI_MEM_NO_DATA);

	return 0;
}

u8 spi_data_out_tran(u64 spi_base, u8* src_buf, u8* cmd_buf, u32 with_cmd, u32 addr_bytes, u32 data_bytes)
{
	u32 cmd_bytes = addr_bytes + ((with_cmd) ? 1 : 0);

	if (data_bytes > 65535) {
		uartlog("data out overflow, should be less than 65535 bytes(%d)\n", data_bytes);
		return -1;
	}

	u8 buf[SPI_PAGE_SIZE + 10];

	memcpy(buf, cmd_buf, addr_bytes + cmd_bytes);
	memcpy(buf+cmd_bytes, src_buf, data_bytes);

	dws.tx_len = data_bytes + cmd_bytes;
	dws.tx = buf;
	dws.rx_len = 0;
	dw_spi_exec_mem_op(SPI_MEM_DATA_OUT);

	return 0;
}

u8 spi_data_in_tran(u64 spi_base, u8* dst_buf, u8* cmd_buf, u32 with_cmd, u32 addr_bytes, u32 data_bytes)
{
	u32 cmd_bytes = addr_bytes + ((with_cmd) ? 1 : 0);

	if (data_bytes > 65535) {
		uartlog("data in overflow, should be less than 65535 bytes(%d)\n", data_bytes);
		return -1;
	}

	dws.tx_len = cmd_bytes;
	dws.tx = cmd_buf;
		// dws.rx_len = 0;
		// dw_spi_exec_mem_op(SPI_MEM_NO_DATA);

	dws.rx = dst_buf;
	dws.rx_len = data_bytes;
	// dws.tx_len = 0;
	dw_spi_exec_mem_op(SPI_MEM_DATA_IN);

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
	// if (send_bytes != get_bytes) {
	// 	uartlog("data in&out: get_bytes should be the same as send_bytes\n");
	// 	return -1;
	// }

	dws.tx_len = addr_bytes + send_bytes +((with_cmd) ? 1 : 0);
	dws.tx = src_buf;
	// dw_spi_exec_mem_op(SPI_MEM_NO_DATA);
	// uartlog("cmd sent down\n");

	// dws.tx_len = 0;
	dws.rx_len = get_bytes;
	dws.rx = dst_buf;
	dw_spi_exec_mem_op(SPI_MEM_DATA_IN);

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

	// spi_in_out_tran(spi_base, data_buf, cmd_buf, 1, 0, 3, 3);
	spi_in_out_tran(spi_base, data_buf, cmd_buf, 1, 0, 0, 3);
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
	spi_in_out_tran(spi_base, data_buf, cmd_buf, 1, 0, 0, 3);

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
		uartlog("wel statue %x\n", spi_status);
		if ((spi_status & SPI_STATUS_WEL) == 0) {
			uartlog("write en failed, get status: 0x%02x i=%d\n", spi_status,i);
			return -1;
		}
		uartlog("write en\n");
		u32 offset = i * SPI_SECTOR_SIZE;
		spi_flash_sector_erase(spi_base, sector_addr + offset);
		u32 wait = 0;
		while(1) {
			spi_status = spi_read_status(spi_base);
			uartlog("statue %d\n", spi_status);
			if (((spi_status & SPI_STATUS_WIP) == 0 ) || (wait > 100000)) {
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
		uartlog("read one page done\n");
	}

	u32 remainder = size % SPI_PAGE_SIZE;
	if (remainder) {
		offset = page_num * SPI_PAGE_SIZE;
		spi_data_read(spi_base, data + offset, fa + offset, remainder);
		uartlog("read remaind  %d bytes done\n", remainder);
	}
	#ifdef DEBUG
	//dump_hex((char *)"cmp_buf", (void *)data, size);
	#endif
}

u64 spi_flash_map_enable(u8 enable)
{
	u64 spi_base = SPI_BASE;

	return spi_base;
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
		// dump_hex((char *)"fw_buf", (void *)(fw_buf + off), 16);
		// dump_hex((char *)"cmp_buf", (void *)cmp_buf, 32);
		return ret;
		}
		uartlog("page read compare ok @%d\n", off);
		off += xfer_size;
	}

	// uartlog("--%s done!\n", __func__);

	return 0;
}

void spi_init(u8 n_bytes)
{
	u32 ctrl0 = 0;
    //debug("%s\n", __func__);
	/* Disable i2c */
#ifdef CONFIG_TEST_WITH_ADAU1372
	writel(SPI_CS_TOG_EN, 0x0000000F);
#else
	//writel(SPI_CS_TOG_EN, 0x00000000);
#endif

	dw_spi_enable(false);
	ctrl0 = (SPI_READ(DW_SPI_CTRL0) & ~(SPI_TMOD_MASK
											| SPI_SCPOL_MASK
											| SPI_SCPH_MASK
											| SPI_FRF_MASK
											| SPI_DFS_MASK));
	if (n_bytes == 1) /* 8 bits data*/
		SPI_WRITE(DW_SPI_CTRL0, ctrl0| SPI_EERPOMREAD | SPI_SCPOL_LOW | SPI_SCPH_MIDDLE | SPI_FRF_SPI | SPI_DFS_8BIT); /* Set to SPI frame format */
	else if (n_bytes == 2)
		SPI_WRITE(DW_SPI_CTRL0, ctrl0| SPI_EERPOMREAD | SPI_SCPOL_LOW | SPI_SCPH_MIDDLE | SPI_FRF_SPI | SPI_DFS_16BIT);
	SPI_WRITE(DW_SPI_BAUDR, SPI_BAUDR_DIV);
	SPI_WRITE(DW_SPI_TXFLTR, 8);
	SPI_WRITE(DW_SPI_RXFLTR, 8);
	// SPI_WRITE(DW_SPI_SER, 0x1); /* enable slave 1 device*/

	dws.reg_io_width = 1;
	dws.fifo_len = 8;

	request_irq(DW_SPI_INTR, spi_irq_handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_MASK, "spi int", (void *)SPI_BASE);


	//dw_spi_enable(true);

#ifdef CONFIG_TEST_WITH_ADAU1372
	SPI_WRITE(DW_SPI_DR, 0x5A);
	udelay(1);
	SPI_WRITE(DW_SPI_DR, 0x5A);
	udelay(1);
	SPI_WRITE(DW_SPI_DR, 0x5A);
	udelay(1);
	dw_spi_enable(false);
	//writel(SPI_CS_TOG_EN, 0x00000000);
#endif
#ifdef CONFIG_USE_SPI_DMA
	printf("USE DMA, DMA=0x%x\n", SPI_READ(DW_SPI_DMACR));
	SPI_WRITE(DW_SPI_DMACR, SPI_DMA_TDMAE_ON | SPI_DMA_RDMAE_ON);
	SPI_WRITE(DW_SPI_DMATDLR, 4);
	SPI_WRITE(DW_SPI_DMARDLR, 4);
	printf("USE DMA, DMA=0x%x\n", SPI_READ(DW_SPI_DMACR));
#endif
	dw_spi_enable(true);

}

/* To initial channel assignment for I2C/I2S */
#ifdef CONFIG_USE_SPI_DMA
static void dma_channel_init(void)
{
	mmio_clrsetbits_32(TOP_DMA_CH_REMAP0, 0x3f << DMA_REMAP_CH2_OFFSET, DMA_RX_REQ_SPI0 << DMA_REMAP_CH2_OFFSET);
	mmio_clrsetbits_32(TOP_DMA_CH_REMAP0, 0x3f << DMA_REMAP_CH3_OFFSET, DMA_TX_REQ_SPI0 << DMA_REMAP_CH3_OFFSET);
	mmio_clrsetbits_32(TOP_DMA_CH_REMAP0, 0x1<<31, DMA_REMAP_UPDATE << DMA_REMAP_UPDATE_OFFSET);
}
#endif

#if 0
static int irq_handler(int irqn, void *priv)
{

	printf("%s irqn=%d\n", __func__, irqn);
	disable_irq(irqn);

    return 0;
}
#endif

