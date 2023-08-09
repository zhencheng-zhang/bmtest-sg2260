#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <bm_spi_nor.h>
#include "mmio.h"
#include "timer.h"
#include "system_common.h"

static unsigned long ctrl_base;
static uint8_t spi_non_data_tran(unsigned long spi_base, uint8_t *cmd_buf,
	uint32_t with_cmd, uint32_t addr_bytes);
static uint8_t spi_data_out_tran(unsigned long spi_base, uint8_t *src_buf, uint8_t *cmd_buf,
	uint32_t with_cmd, uint32_t addr_bytes, uint32_t data_bytes);
static uint8_t spi_in_out_tran(unsigned long spi_base, uint8_t *dst_buf, uint8_t *src_buf,
	uint32_t with_cmd, uint32_t addr_bytes, uint32_t send_bytes, uint32_t get_bytes);

struct dmmr_reg_setting_t
{
	uint32_t cmd;
	uint32_t value;
	uint32_t dummy;
};

const struct dmmr_reg_setting_t dmmr_reg_setting[] = {
	{0x03, 0x003b81,  0}, // 0, RD
	{0x0b, 0x003b89,  8}, // 1, FRD
	{0x3b, 0x003b91,  8}, // 2, FRD_DO
	{0xbb, 0x003b99,  4}, // 3, FRD_DIO
	{0x6b, 0x003ba1,  8}, // 4, FRD_QO
	{0xeb, 0x003ba9,  6}, // 5, FRD_QIO
	// {0xeb, 0x003ba9,  10}, // 5, FRD_QIO, MT25Q256 (spinor1) needs dummy cycle 10
	{0x13, 0x303c81,  0}, // 6 , RD4B
	{0x0c, 0x303c89,  8}, // 7 , FRD4B
	{0x3c, 0x303c91,  8}, // 8 , FRD4B_DO
	{0xbc, 0x303c99,  4}, // 9 , FRD4B_DIO
	{0x6c, 0x303ca1,  8}, // 10, FRD4B_QO
	{0xec, 0x303ca9,  6}  // 11, FRD4B_QIO
	// {0xec, 0x303ca9,  10}, // 11, FRD4B_QIO, MT25Q256 (spinor1) needs dummy cycle 10
};

static int g_int_flag = 0;

int bm_read_int_flag()
{
	return g_int_flag;
}

void bm_write_int_flag(int val)
{
	g_int_flag = val;
}

static uint32_t match_read_cmd(uint8_t read_cmd)
{
	int i;
	uint32_t val;

	for (i = 0; i < sizeof(dmmr_reg_setting); i++) {
		if (read_cmd == dmmr_reg_setting[i].cmd) {
			val = dmmr_reg_setting[i].value;
			val &= ~(0xf << 16);
			val |= ((dmmr_reg_setting[i].dummy & 0xf) << 16);
			return val;
		}
	}
	return 0;
}

static void dump_hex(char *desc, void *addr, int len)
{
	int i;
	unsigned char buff[17];
	unsigned char *pc = (unsigned char *)addr;

	/* Output description if given. */
	if (desc != NULL)
		printf("%s:\n", desc);

	/* Process every byte in the data. */
	for (i = 0; i < len; i++) {
		/* Multiple of 16 means new line (with line offset). */
		if ((i % 16) == 0) {
			/* Just don't print ASCII for the zeroth line. */
			if (i != 0)
				printf("  %s\n", buff);

			/* Output the offset. */
			printf("  %x ", i);
		}

		/* Now the hex code for the specific character. */
		printf(" %x", pc[i]);

		/* And store a printable ASCII character for later. */
		if ((pc[i] < 0x20) || (pc[i] > 0x7e))
			buff[i % 16] = '.';
		else
			buff[i % 16] = pc[i];
		buff[(i % 16) + 1] = '\0';
	}

	/* Pad out last line if not exactly 16 characters. */
	while ((i % 16) != 0) {
		printf("   ");
		i++;
	}

	/* And print the final ASCII bit. */
	printf("  %s\n", buff);
}

void bm_spi_nor_enter_quad(unsigned long spi_base, uint32_t enter)
{
	uint8_t cmd_buf[4];
	uint8_t data_buf[20];

	printf("%s\n", __func__);

	memset(cmd_buf, 0, sizeof(cmd_buf));
	memset(data_buf, 0, sizeof(data_buf));

	if (enter) {
		cmd_buf[0] = SPINOR_CMD_ENQD;
	}
	else {
		cmd_buf[0] = SPINOR_CMD_EXQD;
	}

	spi_in_out_tran(spi_base, data_buf, cmd_buf, 1, 0, 3, 3);

	return;
}

static int spi_data_in_tran(unsigned long spi_base, uint8_t *dst_buf, uint8_t *cmd_buf,
	int with_cmd, int addr_bytes, int data_bytes)
{
	uint32_t *p_data = (uint32_t *)cmd_buf;
	uint32_t tran_csr = 0;
	int cmd_bytes = addr_bytes + ((with_cmd) ? 1 : 0);
	int i, xfer_size, off;

	if (data_bytes > 65535) {
		printf("SPI data in overflow, should be less than 65535 bytes(%d)\n", data_bytes);
		return -1;
	}

	// disable DMMR (direct memory mapping read)
	mmio_write_32(ctrl_base + REG_SPINOR_DMMR, 0);

	/* init tran_csr */
	tran_csr = mmio_read_32(spi_base + REG_SPINOR_TRAN_CSR);
	// printf("before: tran_csr = 0x%x\n", tran_csr);

	tran_csr &= ~(BIT_SPINOR_TRAN_CSR_TRAN_MODE_MASK
			| BIT_SPINOR_TRAN_CSR_ADDR_BYTES_MASK
			| BIT_SPINOR_TRAN_CSR_FIFO_TRG_LVL_MASK
			| BIT_SPINOR_TRAN_CSR_WITH_CMD
			| BIT_SPINOR_TRAN_CSR_FAST_MODE
			| BIT_SPINOR_TRAN_CSR_BUS_WIDTH_2_BIT
			| BIT_SPINOR_TRAN_CSR_BUS_WIDTH_4_BIT
			| BIT_SPINOR_TRAN_CSR_DUMMY_MASK);
	tran_csr |= (addr_bytes << SPINOR_TRAN_CSR_ADDR_BYTES_SHIFT);
	tran_csr |= (with_cmd) ? BIT_SPINOR_TRAN_CSR_WITH_CMD : 0;
	tran_csr |= BIT_SPINOR_TRAN_CSR_FIFO_TRG_LVL_8_BYTE;
	tran_csr |= BIT_SPINOR_TRAN_CSR_TRAN_MODE_RX;

	mmio_write_32(spi_base + REG_SPINOR_FIFO_PT, 0); // flush FIFO before filling fifo
	if (with_cmd) {
		for (i = 0; i < ((cmd_bytes - 1) / 4 + 1); i++) {
			// printf("p_data = 0x%08x\n", p_data[i]);
			mmio_write_32(spi_base + REG_SPINOR_FIFO_PORT, p_data[i]);
		}
	}

	/* issue tran */
	mmio_write_32(spi_base + REG_SPINOR_INT_STS, 0); // clear all int
	mmio_write_32(spi_base + REG_SPINOR_TRAN_NUM, data_bytes);
	tran_csr |= BIT_SPINOR_TRAN_CSR_GO_BUSY;
	// printf("after: tran_csr = 0x%x\n", tran_csr);
	mmio_write_32(spi_base + REG_SPINOR_TRAN_CSR, tran_csr);

	/* check rd int to make sure data out done and in data started */
	while((mmio_read_32(spi_base + REG_SPINOR_INT_STS) & BIT_SPINOR_INT_RD_FIFO) == 0)
		;

	/* get data */
	p_data = (uint32_t *)dst_buf;
	off = 0;
	while (off < data_bytes) {
		if (data_bytes - off >= SPINOR_MAX_FIFO_DEPTH)
			xfer_size = SPINOR_MAX_FIFO_DEPTH;
		else
			xfer_size = data_bytes - off;

		while ((mmio_read_32(spi_base + REG_SPINOR_FIFO_PT) & 0xF) != xfer_size)
			;
		for (i = 0; i < ((xfer_size - 1) / 4 + 1); i++) {
			p_data[off / 4 + i] = mmio_read_32(spi_base + REG_SPINOR_FIFO_PORT);
			// printf("data[%d] = 0x%X\n", off / 4 + i, p_data[off / 4 + i]);
		}
		off += xfer_size;
	}

	/* wait tran done */
	while((mmio_read_32(spi_base + REG_SPINOR_INT_STS) & BIT_SPINOR_INT_TRAN_DONE) == 0)
		;

	mmio_write_32(spi_base + REG_SPINOR_FIFO_PT, 0);    //should flush FIFO after tran

	// disable DMMR (direct memory mapping read)
	mmio_write_32(ctrl_base + REG_SPINOR_DMMR, 1);

	return 0;
}

static int spi_data_read(unsigned long spi_base, uint8_t *dst_buf, uint32_t addr, uint32_t size, uint32_t addr_mode)
{
	uint8_t cmd_buf[1+addr_mode];
	uint32_t shift = 0;

	memset(cmd_buf, 0, sizeof(cmd_buf));

	if (addr_mode == ADDR_4B)
		cmd_buf[0] = SPINOR_CMD_READ_4B;
	else
		cmd_buf[0] = SPINOR_CMD_READ;

	for (uint32_t i = 0; i < addr_mode; i++) {
		shift =  (addr_mode - i - 1) * 8;
		cmd_buf[1+i] = ((addr) >> shift) & 0xFF;
	}

	spi_data_in_tran(spi_base, dst_buf, cmd_buf, 1, addr_mode, size);

	return 0;
}

static uint8_t spi_page_program(unsigned long spi_base, uint8_t *src_buf, uint32_t addr, uint32_t size, uint32_t addr_mode)
{
	uint8_t cmd_buf[1+addr_mode];
	uint32_t shift = 0;

	memset(cmd_buf, 0, sizeof(cmd_buf));

	if (addr_mode == ADDR_4B)
		cmd_buf[0] = SPINOR_CMD_PP_4B;
	else
		cmd_buf[0] = SPINOR_CMD_PP;

	for (uint32_t i = 0; i < addr_mode; i++) {
		shift =  (addr_mode - i - 1) * 8;
		cmd_buf[1+i] = ((addr) >> shift) & 0xFF;
	}

	spi_data_out_tran(spi_base, src_buf, cmd_buf, 1, addr_mode, size);
	return 0;
}


void bm_spi_flash_read_sector(unsigned long spi_base, uint32_t addr, uint32_t addr_mode, uint8_t *buf)
{
	spi_data_read(spi_base, buf, (int)addr, SPI_SECTOR_SIZE, addr_mode);
}

__attribute((unused)) static int bm_spi_nor_irq_handler(int irqn, void *priv)
{
	__attribute((unused)) u32 int_sts = mmio_read_32(ctrl_base + REG_SPINOR_INT_STS);
	printf("\tenter spi nor irq handler\n");
	debug("  -- INT_STS 0x%x\n", mmio_read_32(ctrl_base + REG_SPINOR_INT_STS));

	mmio_clrbits_32(ctrl_base + REG_SPINOR_INT_STS, BIT_SPINOR_INT_TRAN_DONE);
	debug("  -- After clearing int: INT_STS 0x%x\n", 
		mmio_read_32(ctrl_base + REG_SPINOR_INT_STS));

	// set int_flag
	bm_write_int_flag(1);

	debug("  -- exit bm_spi_nor_irq_handler()\n");

	return 0;
}

void bm_spi_init(uint32_t base, uint32_t clk_div, uint32_t cpha, uint32_t cpol, uint32_t edge, uint32_t int_en)
{
	uint32_t tran_csr = 0;
	uint8_t read_cmd;

	ctrl_base = base;

	// disable DMMR (direct memory mapping read)
	mmio_write_32(ctrl_base + REG_SPINOR_DMMR, 0);

	// soft reset
	mmio_write_32(ctrl_base + REG_SPINOR_CTRL, \
		(mmio_read_32(ctrl_base + REG_SPINOR_CTRL) | BIT_SPINOR_CTRL_SRST));
	while(mmio_read_32(ctrl_base + REG_SPINOR_CTRL) & BIT_SPINOR_CTRL_SRST);

	mmio_write_32(ctrl_base + REG_SPINOR_CTRL, \
		(mmio_read_32(ctrl_base + REG_SPINOR_CTRL) & 0xFFFFC000) | (0x8<<16) | clk_div | cpol<<13 | cpha<<12);

	mmio_write_32(ctrl_base + REG_SPINOR_CE_CTRL, 0x0);

	read_cmd = 0xEB;
	tran_csr = match_read_cmd(read_cmd);
	mmio_write_32(ctrl_base + REG_SPINOR_TRAN_CSR, tran_csr);

	if (edge == 0)	// positive edge sample
		mmio_write_32(ctrl_base + REG_SPINOR_DLY_CTRL, 0x300);
	else	// negative edge sample
		mmio_write_32(ctrl_base + REG_SPINOR_DLY_CTRL, 0x4300);

	// mmio_write_32(ctrl_base + REG_SPINOR_OPT, 0x3);

	debug("%s base 0x%lx: CTRL 0x=%x, DLY_CTRL=0x%x, TRAN_CSR=0x%x\n",
		__func__,
		ctrl_base,
		mmio_read_32(ctrl_base + REG_SPINOR_CTRL),
		mmio_read_32(ctrl_base + REG_SPINOR_DLY_CTRL),
		mmio_read_32(ctrl_base + REG_SPINOR_TRAN_CSR));

	if (int_en) {
		// clear all int status
		mmio_write_32(ctrl_base + REG_SPINOR_INT_STS, 0);
		mmio_write_32(ctrl_base + REG_SPINOR_INT_EN, BIT_SPINOR_INT_TRAN_DONE);

		printf("\tregister spinor interrupt ID: %d\n", SPINOR_INT);
		request_irq(SPINOR_INT, bm_spi_nor_irq_handler, 0, "spif int", NULL);
	}
}

/* here are APIs for SPI flash programming */

static uint8_t spi_non_data_tran(unsigned long spi_base, uint8_t *cmd_buf,
	uint32_t with_cmd, uint32_t addr_bytes)
{
//	uint32_t *p_data = (uint32_t*)cmd_buf;
	uint32_t tran_csr = 0;

	// disable DMMR (direct memory mapping read)
	mmio_write_32(spi_base + REG_SPINOR_DMMR, 0);

	/* init tran_csr */
	tran_csr = mmio_read_32(spi_base + REG_SPINOR_TRAN_CSR);
	// printf("%s: -tran_csr=0x%x\n", __func__, tran_csr);
	tran_csr &= ~(BIT_SPINOR_TRAN_CSR_TRAN_MODE_MASK
			  | BIT_SPINOR_TRAN_CSR_BUSWIDTH_MASK
	          | BIT_SPINOR_TRAN_CSR_ADDR_BYTES_MASK
	          | BIT_SPINOR_TRAN_CSR_FIFO_TRG_LVL_MASK
	          | BIT_SPINOR_TRAN_CSR_WITH_CMD
			  | BIT_SPINOR_TRAN_CSR_FAST_MODE
			  | BIT_SPINOR_TRAN_CSR_DUMMY_MASK
			  | BIT_SPINOR_TRAN_CSR_4BADDR_MASK);
	tran_csr |= (addr_bytes << SPINOR_TRAN_CSR_ADDR_BYTES_SHIFT);
	tran_csr |= BIT_SPINOR_TRAN_CSR_FIFO_TRG_LVL_1_BYTE;
	tran_csr |= (with_cmd ? BIT_SPINOR_TRAN_CSR_WITH_CMD : 0);

	// printf("%s: +tran_csr=0x%x\n", __func__, tran_csr);

	mmio_write_32(spi_base + REG_SPINOR_FIFO_PT, 0); //do flush FIFO before filling fifo

#if 0
	mmio_write_32(spi_base + REG_SPINOR_FIFO_PORT, p_data[0]);
#else
	if (with_cmd) {
		for (uint32_t i = 0; i < 1 + addr_bytes; i++) {
			mmio_write_8(spi_base + REG_SPINOR_FIFO_PORT, cmd_buf[i]);
		}
	}
#endif

	/* issue tran */
	mmio_write_32(spi_base + REG_SPINOR_INT_STS, 0); //clear all int
	tran_csr |= BIT_SPINOR_TRAN_CSR_GO_BUSY;
	mmio_write_32(spi_base + REG_SPINOR_TRAN_CSR, tran_csr);

	/* wait tran done */
	// while((mmio_read_32(spi_base + REG_SPINOR_INT_STS) & BIT_SPINOR_INT_TRAN_DONE) == 0)
	// 	;

	while(1) {
		if (bm_read_int_flag())
			break;
		else if (mmio_read_32(spi_base + REG_SPINOR_INT_STS) & BIT_SPINOR_INT_TRAN_DONE)
			break;
	}
	bm_write_int_flag(0);

	mmio_write_32(spi_base + REG_SPINOR_FIFO_PT, 0); //should flush FIFO after tran

	// enable DMMR (direct memory mapping read)
	mmio_write_32(spi_base + REG_SPINOR_DMMR, 1);

	return 0;
}

static uint8_t spi_data_out_tran(unsigned long spi_base, uint8_t *src_buf, uint8_t *cmd_buf,
	uint32_t with_cmd, uint32_t addr_bytes, uint32_t data_bytes)
{
	uint32_t *p_data = (uint32_t *)cmd_buf;
	uint32_t tran_csr = 0;
	uint32_t cmd_bytes = addr_bytes + (with_cmd ? 1 : 0);
	uint32_t xfer_size, off;
	uint32_t wait = 0;
	int i;

	if (data_bytes > 65535) {
		printf("data out overflow, should be less than 65535 bytes(%d)\n", data_bytes);
		return -1;
	}

	// disable DMMR (direct memory mapping read)
	mmio_write_32(spi_base + REG_SPINOR_DMMR, 0);

	/* init tran_csr */
	tran_csr = mmio_read_32(spi_base + REG_SPINOR_TRAN_CSR);
	// printf("%s: -tran_csr=0x%x\n", __func__, tran_csr);
	tran_csr &= ~(BIT_SPINOR_TRAN_CSR_TRAN_MODE_MASK
			  | BIT_SPINOR_TRAN_CSR_BUSWIDTH_MASK
	          | BIT_SPINOR_TRAN_CSR_ADDR_BYTES_MASK
	          | BIT_SPINOR_TRAN_CSR_FIFO_TRG_LVL_MASK
	          | BIT_SPINOR_TRAN_CSR_WITH_CMD
			  | BIT_SPINOR_TRAN_CSR_FAST_MODE
			  | BIT_SPINOR_TRAN_CSR_DUMMY_MASK
			  | BIT_SPINOR_TRAN_CSR_4BADDR_MASK);
	tran_csr |= (addr_bytes << SPINOR_TRAN_CSR_ADDR_BYTES_SHIFT);
	tran_csr |= (with_cmd ? BIT_SPINOR_TRAN_CSR_WITH_CMD : 0);
	tran_csr |= BIT_SPINOR_TRAN_CSR_FIFO_TRG_LVL_8_BYTE;
	tran_csr |= BIT_SPINOR_TRAN_CSR_TRAN_MODE_TX;

	// printf("%s: +tran_csr=0x%x\n", __func__, tran_csr);

	mmio_write_32(spi_base + REG_SPINOR_FIFO_PT, 0); //do flush FIFO before filling fifo

#if 0
	if (with_cmd) {
		for (i = 0; i < ((cmd_bytes - 1) / 4 + 1); i++) {
			printf("data out 0x%x\n", p_data[i]);
			mmio_write_32(spi_base + REG_SPINOR_FIFO_PORT, p_data[i]);
		}
	}
#else
	if (with_cmd) {
		for (i = 0; i < cmd_bytes; i++) {
			mmio_write_8(spi_base + REG_SPINOR_FIFO_PORT, cmd_buf[i]);
		}
	}
#endif
	/* issue tran */
	mmio_write_32(spi_base + REG_SPINOR_INT_STS, 0); //clear all int
	mmio_write_32(spi_base + REG_SPINOR_TRAN_NUM, data_bytes);
	tran_csr |= BIT_SPINOR_TRAN_CSR_GO_BUSY;
	mmio_write_32(spi_base + REG_SPINOR_TRAN_CSR, tran_csr);
	while ((mmio_read_32(spi_base + REG_SPINOR_FIFO_PT) & 0xF) != 0)
		; //wait for cmd issued

	mmio_write_32(spi_base + REG_SPINOR_FIFO_PT, 0); //do flush FIFO before filling fifo

	/* fill data */
	p_data = (uint32_t *)src_buf;
	off = 0;
	while (off < data_bytes) {
		if (data_bytes - off >= SPINOR_MAX_FIFO_DEPTH)
			xfer_size = SPINOR_MAX_FIFO_DEPTH;
		else
			xfer_size = data_bytes - off;

		wait = 0;
		while ((mmio_read_32(spi_base + REG_SPINOR_FIFO_PT) & 0xF) != 0) {
			wait++;
			udelay(10);
			if (wait > 30000) { // 300ms
				printf("wait to write FIFO timeout\n");
				return -1;
			}
		}

		for (i = 0; i < ((xfer_size - 1) / 4 + 1); i++)
			mmio_write_32(spi_base + REG_SPINOR_FIFO_PORT, p_data[off / 4 + i]);

		off += xfer_size;
	}

	/* wait tran done */
	while((mmio_read_32(spi_base + REG_SPINOR_INT_STS) & BIT_SPINOR_INT_TRAN_DONE) == 0)
		;
	mmio_write_32(spi_base + REG_SPINOR_FIFO_PT, 0); //should flush FIFO after tran

	// enable DMMR (direct memory mapping read)
	mmio_write_32(spi_base + REG_SPINOR_DMMR, 1);

	return 0;
}

/*
 * spi_in_out_tran is a workaround fucntion for current 32-bit access to spic fifo:
 * AHB bus could only do 32-bit access to spic fifo, so cmd without 3-bytes addr will leave 3-byte
 * data in fifo, so set tx to mark that these 3-bytes data would be sent out.
 * So send_bytes should be 3 (wirte 1 dw into fifo) or 7(write 2 dw), get_bytes sould be the same value.
 * software would mask out unuseful data in get_bytes.
 */
static uint8_t spi_in_out_tran(unsigned long spi_base, uint8_t *dst_buf, uint8_t *src_buf,
	uint32_t with_cmd, uint32_t addr_bytes, uint32_t send_bytes, uint32_t get_bytes)
{
	uint32_t *p_data = (uint32_t *)src_buf;
	uint32_t total_out_bytes;
	uint32_t tran_csr = 0;
	int i;

	if ((send_bytes > SPINOR_MAX_FIFO_DEPTH) || (get_bytes > SPINOR_MAX_FIFO_DEPTH)) {
		printf("data in&out: FIFO will overflow\n");
		return -1;
	}

	// disable DMMR (direct memory mapping read)
	mmio_write_32(spi_base + REG_SPINOR_DMMR, 0);

	/* init tran_csr */
	tran_csr = mmio_read_32(spi_base + REG_SPINOR_TRAN_CSR);
	// printf("%s: -tran_csr=0x%x\n", __func__, tran_csr);
	tran_csr &= ~(BIT_SPINOR_TRAN_CSR_TRAN_MODE_MASK
			  | BIT_SPINOR_TRAN_CSR_BUSWIDTH_MASK
	          | BIT_SPINOR_TRAN_CSR_ADDR_BYTES_MASK
	          | BIT_SPINOR_TRAN_CSR_FIFO_TRG_LVL_MASK
	          | BIT_SPINOR_TRAN_CSR_WITH_CMD
			  | BIT_SPINOR_TRAN_CSR_FAST_MODE
			  | BIT_SPINOR_TRAN_CSR_DUMMY_MASK
			  | BIT_SPINOR_TRAN_CSR_4BADDR_MASK);
	tran_csr |=	BIT_SPINOR_TRAN_CSR_BUS_WIDTH_1_BIT;
	tran_csr |= (addr_bytes << SPINOR_TRAN_CSR_ADDR_BYTES_SHIFT);
	tran_csr |= BIT_SPINOR_TRAN_CSR_FIFO_TRG_LVL_1_BYTE;
	tran_csr |= BIT_SPINOR_TRAN_CSR_WITH_CMD;
	tran_csr |= BIT_SPINOR_TRAN_CSR_TRAN_MODE_TX;
	tran_csr |= BIT_SPINOR_TRAN_CSR_TRAN_MODE_RX;

	// printf("%s: +tran_csr=0x%x\n", __func__, tran_csr);

	mmio_write_32(spi_base + REG_SPINOR_FIFO_PT, 0); //do flush FIFO before filling fifo
	total_out_bytes = addr_bytes + send_bytes + (with_cmd ? 1 : 0);
	for (i = 0; i < ((total_out_bytes - 1) / 4 + 1); i++) {
		// printf("send data 0x%x\n", p_data[i]);
		mmio_write_32(spi_base + REG_SPINOR_FIFO_PORT, p_data[i]);
	}
	/* issue tran */
	mmio_write_32(spi_base + REG_SPINOR_INT_STS, 0); //clear all int
	mmio_write_32(spi_base + REG_SPINOR_TRAN_NUM, get_bytes);
	tran_csr |= BIT_SPINOR_TRAN_CSR_GO_BUSY;
	mmio_write_32(spi_base + REG_SPINOR_TRAN_CSR, tran_csr);

	/* wait tran done and get data */
	// while((mmio_read_32(spi_base + REG_SPINOR_INT_STS) & BIT_SPINOR_INT_TRAN_DONE) == 0)
	// 	;

	while(1) {
		if (bm_read_int_flag())
			break;
		else if (mmio_read_32(spi_base + REG_SPINOR_INT_STS) & BIT_SPINOR_INT_TRAN_DONE)
			break;
	}
	bm_write_int_flag(0);

	// ** should flush FIFO after tran
	mmio_write_32(spi_base + REG_SPINOR_FIFO_PT, 0);

	p_data = (uint32_t*)dst_buf;
	for (i = 0; i < ((get_bytes - 1) / 4 + 1); i++)
		p_data[i] = mmio_read_32(spi_base + REG_SPINOR_FIFO_PORT);

	mmio_write_32(spi_base + REG_SPINOR_FIFO_PT, 0); //should flush FIFO after tran

	// enable DMMR (direct memory mapping read)
	mmio_write_32(spi_base + REG_SPINOR_DMMR, 1);

	return 0;
}

static uint8_t spi_write_en(unsigned long spi_base)
{
	uint8_t cmd_buf[4];
	memset(cmd_buf, 0, sizeof(cmd_buf));

	debug("spi_write_en()\n");

	cmd_buf[0] = SPINOR_CMD_WREN;
	spi_non_data_tran(spi_base, cmd_buf, 1, 0);
	return 0;
}

uint8_t spi_read_status(unsigned long spi_base)
{
	uint8_t cmd_buf[4];
	uint8_t data_buf[4];
	memset(cmd_buf, 0, sizeof(cmd_buf));
	memset(data_buf, 0, sizeof(data_buf));

	cmd_buf[0] = SPINOR_CMD_RDSR;
	spi_in_out_tran(spi_base, data_buf, cmd_buf, 1, 0, 3, 3);

	debug("got status %x %x %x %x\n", data_buf[3], data_buf[2], data_buf[1], data_buf[0]);
	return data_buf[0];
}

void bm_spi_byte4en(unsigned long spi_base, uint8_t read_cmd)
{
	uint32_t tran_csr;

	mmio_write_32(spi_base + REG_SPINOR_DMMR, 0);

	tran_csr = match_read_cmd(read_cmd);

	mmio_write_32(spi_base + REG_SPINOR_TRAN_CSR, tran_csr);

	mmio_write_32(spi_base + REG_SPINOR_DMMR, 1);

	return;
}

uint32_t bm_spi_reset_nor(unsigned long spi_base)
{
	uint8_t cmd_buf[4];
	memset(cmd_buf, 0, sizeof(cmd_buf));

	cmd_buf[0] = 0x66;
	spi_non_data_tran(spi_base, cmd_buf, 1, 0);
	cmd_buf[0] = 0x99;
	spi_non_data_tran(spi_base, cmd_buf, 1, 0);
	return 0;
}

uint32_t bm_spi_read_id(unsigned long spi_base)
{
	uint8_t cmd_buf[4];
	uint8_t data_buf[4];
	uint32_t read_id = 0;

	memset(cmd_buf, 0, sizeof(cmd_buf));
	memset(data_buf, 0, sizeof(data_buf));

	cmd_buf[0] = SPINOR_CMD_RDID;
	spi_in_out_tran(spi_base, data_buf, cmd_buf, 1, 0, 3, 3);
	read_id = (data_buf[0] << 16) | (data_buf[1] << 8) | (data_buf[2]);

	return read_id;
}

uint32_t bm_spi_nor_sfdp(unsigned long spi_base)
{
	uint8_t cmd_buf[5];
	uint8_t data_buf[20];
	uint32_t read_id = 0;

	printf("%s\n", __func__);

	memset(cmd_buf, 0, sizeof(cmd_buf));
	memset(data_buf, 0, sizeof(data_buf));

	cmd_buf[0] = SPINOR_CMD_SFDP;

	spi_data_in_tran(spi_base, data_buf, cmd_buf, 1, 3, 20);

	read_id = (data_buf[2] << 16) | (data_buf[1] << 8) | (data_buf[0]);

	return read_id;
}

uint32_t bm_spi_nor_enter_4byte(unsigned long spi_base, uint32_t enter)
{
	uint8_t cmd_buf[5];
	uint8_t data_buf[20];
	uint32_t read_id = 0;

	printf("%s\n", __func__);

	memset(cmd_buf, 0, sizeof(cmd_buf));
	memset(data_buf, 0, sizeof(data_buf));

	if (enter) {
		cmd_buf[0] = SPINOR_CMD_EN4B;
	}
	else {
		cmd_buf[0] = SPINOR_CMD_EX4B;
	}

	spi_in_out_tran(spi_base, data_buf, cmd_buf, 1, 0, 3, 3);

	for (uint32_t i = 0; i < 20; i++) {
		printf("%d: 0x%x\n", i, data_buf[i]);
	}

	read_id = (data_buf[2] << 16) | (data_buf[1] << 8) | (data_buf[0]);

	return read_id;
}

static void spi_sector_erase(unsigned long spi_base, uint32_t addr, uint32_t addr_mode, uint32_t sector_size)
{
	uint32_t i, shift;
	uint8_t cmd_buf[1+addr_mode];
	memset(cmd_buf, 0, sizeof(cmd_buf));

	if ((sector_size == SIZE_4KB) && (addr_mode == ADDR_4B))
		cmd_buf[0] = SPINOR_CMD_SE_4B;	//21h
	else if ((sector_size == SIZE_4KB) && (addr_mode == ADDR_3B))
		cmd_buf[0] = SPINOR_CMD_SE;	//20h
	else if ((sector_size == SIZE_64KB) && (addr_mode == ADDR_4B))
		cmd_buf[0] = SPINOR_CMD_BE_64K_4B;	//DCh
	else if ((sector_size == SIZE_64KB) && (addr_mode == ADDR_3B))
		cmd_buf[0] = SPINOR_CMD_BE_64K;	//D8h
	else {
		printf("%s Wrong arguments!!!\n", __func__);
		return;
	}

	for (i = 0; i < addr_mode; i++) {
		shift =  (addr_mode - i - 1) * 8;
		cmd_buf[1+i] = ((addr) >> shift) & 0xFF;
	}
	spi_non_data_tran(spi_base, cmd_buf, 1, addr_mode);
	return;
}

int bm_spi_flash_erase_sector(unsigned long spi_base, uint32_t addr, uint32_t addr_mode, uint32_t sector_size)
{
	uint32_t spi_status, wait = 0;

	// check address alignment
	if ((sector_size == SIZE_4KB) && (addr % SIZE_4KB) != 0) {
		printf("%s erased address is not 4KB-aligned\n", __func__);
		return -1;
	} else if ((sector_size == SIZE_64KB) && (addr % SIZE_64KB) != 0) {
		printf("%s erased address is not 64KB-aligned\n", __func__);
		return -1;
	}

	spi_write_en(spi_base);	//06h

	spi_status = spi_read_status(spi_base);	//05h
	if ((spi_status & SPINOR_STATUS_WEL) == 0) {
		printf("write en failed, get status: 0x%x\n", spi_status);
		return -1;
	}

	if (addr_mode == ADDR_4B)
		spi_sector_erase(spi_base, addr, ADDR_4B, sector_size);
	else
		spi_sector_erase(spi_base, addr, ADDR_3B, sector_size);


	while(1) {
		mdelay(200);
		spi_status = spi_read_status(spi_base);	//05h
		if (((spi_status & SPINOR_STATUS_WIP) == 0 ) || (wait > 30)) { // 3s, spec 0.15~1s
			debug("sector erase done, get status: 0x%x, wait: %d\n", spi_status, wait);
			break;
		}
		wait++;
		debug("device busy, get status: 0x%x\n", spi_status);
	}

	return 0;
}

int bm_spi_flash_program_sector(unsigned long spi_base, uint32_t addr, uint32_t len, uint32_t addr_mode, uint8_t *buf)
{
	uint8_t spi_status;
	uint32_t wait = 0;

	spi_write_en(spi_base);	//06h

	spi_status = spi_read_status(spi_base);	//05h
	if (spi_status != SPINOR_STATUS_WEL) {
		printf("%s spi status check failed, get status: 0x%x\n", __func__, spi_status);
		return -1;
	}

	spi_page_program(spi_base, buf, addr, len, addr_mode);

	while(1) {
		udelay(100);
		spi_status = spi_read_status(spi_base);
		if (((spi_status & SPINOR_STATUS_WIP) == 0 ) || (wait > 600)) { // 60ms, spec 120~2800us
			debug("sector prog done, get status: 0x%x\n", spi_status);
			break;
		}
		wait++;
		debug("device busy, get status: 0x%x\n", spi_status);
	}
	return 0;
}

static int do_page_program(unsigned long spi_base, uint8_t *src_buf, uint32_t addr, uint32_t size)
{
	uint8_t spi_status;
	uint32_t wait = 0;

	printf("do page prog @ 0x%x, size: %d\n", addr, size);

	if (size > SPINOR_FLASH_BLOCK_SIZE) {
		printf("size larger than a page\n");
		return -1;
	}
	if ((addr % SPINOR_FLASH_BLOCK_SIZE) != 0) {
		printf("addr not alignned to page\n");
		return -1;
	}

	spi_write_en(spi_base);

	spi_status = spi_read_status(spi_base);
	if (spi_status != 0x02) {
		printf("spi status check failed, get status: 0x%x\n", spi_status);
		return -1;
	}

	spi_page_program(spi_base, src_buf, addr, size, ADDR_3B);

	while(1) {
		udelay(100);
		spi_status = spi_read_status(spi_base);
		if (((spi_status & SPINOR_STATUS_WIP) == 0 ) || (wait > 600)) { // 60ms, spec 120~2800us
			printf("page prog done, get status: 0x%x\n", spi_status);
			break;
		}
		wait++;
		printf("device busy, get status: 0x%x\n", spi_status);
	}
	return 0;
}

static void fw_sp_read_test(unsigned long spi_base, uint32_t addr)
{
	unsigned char cmp_buf[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	spi_data_read(spi_base, cmp_buf, addr, sizeof(cmp_buf), ADDR_3B);
	dump_hex((char *)"read flash", (void *)cmp_buf, sizeof(cmp_buf));
}

int bm_spi_flash_program(uint8_t *src_buf, uint32_t base, uint32_t size)
{
	uint32_t xfer_size, off, cmp_ret;
	uint8_t cmp_buf[SPINOR_FLASH_BLOCK_SIZE], erased_sectors, i;
	uint32_t id, sector_size;

	id = bm_spi_read_id(ctrl_base);
	if (id == SPINOR_ID_M25P128) {
		sector_size = 256 * 1024;
	} else {
		sector_size = 64 * 1024;
	}

	if ((base % sector_size) != 0) {
		printf("<flash offset addr> is not aligned erase sector size (0x%x)!\n", sector_size);
		return -1;
	}

	erased_sectors = (size + sector_size) / sector_size;
	printf("Start erasing %d sectors, each %d bytes...\n", erased_sectors, sector_size);

	for (i = 0; i < erased_sectors; i++)
		bm_spi_flash_erase_sector(ctrl_base, base + i * sector_size, ADDR_3B, sector_size);

	fw_sp_read_test(ctrl_base, base);
	printf("--program boot fw, page size %d\n", SPINOR_FLASH_BLOCK_SIZE);

	off = 0;
	i = 0;
	while (off < size) {
		if ((size - off) >= SPINOR_FLASH_BLOCK_SIZE)
			xfer_size = SPINOR_FLASH_BLOCK_SIZE;
		else
			xfer_size = size - off;

		if (do_page_program(ctrl_base, src_buf + off, base + off, xfer_size) != 0) {
			printf("page prog failed @ 0x%x\n", base + off);
			return -1;
		}

		spi_data_read(ctrl_base, cmp_buf, base + off, xfer_size, ADDR_3B);
		cmp_ret = memcmp(src_buf + off, cmp_buf, xfer_size);
		if (cmp_ret != 0) {
			printf("memcmp failed\n");
			dump_hex((char *)"src_buf", (void *)(src_buf + off), 16);
			dump_hex((char *)"cmp_buf", (void *)cmp_buf, 16);
			return cmp_ret;
		}
		printf("page read compare ok @ %d\n", off);
		off += xfer_size;

		printf(".");
		if (++i % 32 == 0) {
			printf("\n");
			i = 0;
		}
	}

	printf("--program boot fw success\n");
	fw_sp_read_test(ctrl_base, base);
	return 0;
}
