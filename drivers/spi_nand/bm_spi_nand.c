#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <bm_spi_nand.h>
#include "mmio.h"
#include "timer.h"
#include "system_common.h"
#include "utils_def.h"
#include <sysdma.h>

static unsigned long spi_nand_ctrl_base;

#define MAX_BLK_ID		(spinand_info->block_cnt)
#define NR_PG_PER_NATIVE_BLK	(spinand_info->pages_per_block)
#define TEST_BLK_ID		1023
#define DUMP_REG 0

#define SPI_NAND_PLANE_BIT_OFFSET	BIT(12)

static spi_nand_info_t *spinand_info;

static spi_nand_info_t spi_nand_support_list[] =
{
	// spec : DS-00499-GD5F1GQ4xExxG-Rev2.4.pdf
	{0xd1c8, 2048, 128, 64, 1024, 6, FLAGS_SET_QE_BIT | FLAGS_ENABLE_X4_BIT | FLAGS_OW_SETTING_BIT}, // DS-GD5F1GQ4U, 1G, 3.3v (FPGA)
	{0xc1c8, 2048, 128, 64, 1024, 6, 0}, // DS-GD5F1GQ4R, 1G, 1.8V

	{0x41c8, 2048, 128, 64, 2048, 6, FLAGS_SET_QE_BIT | FLAGS_SET_PLANE_BIT},  // ESMT F50D2G41XA(2B) 2G, 1.8V
	{0x11c8, 2048, 128, 64, 1024, 6, 0}, // ESMT F50D1G41LB(2M) 1G, 1.8V (FPGA)
	{0x242c, 2048, 128, 64, 2048, 6, FLAGS_SET_PLANE_BIT},  // ESMT F50L2G41XA(2B) 2G, 3.3V (FPGA)
	{0x252c, 2048, 128, 64, 1024, 6, FLAGS_SET_PLANE_BIT},  // ESMT F50D2G41XA(2B) 2G, 1.8V


	{0x12c2, 2048, 64, 64, 1024, 6, FLAGS_SET_QE_BIT}, // MX35LF1GE4AB-Z4I, 1Gb, 3V
	{0x22c2, 2048, 64, 64, 2048, 6, FLAGS_SET_PLANE_BIT},  // MX35LF2GE4AB-MI, 2Gb 3V (PXP)
	{0x22c8, 2048, 64, 64, 2048, 6, FLAGS_SET_QE_BIT},//GD5F2GQ5RExxH 1.8V
	{0x55c8, 2048, 64, 128, 4096, 6, 0},//GD5F4GQ6xExxG 1.8V
	{0x52c8, 2048, 128, 64, 2048, 6, FLAGS_SET_QE_BIT | FLAGS_ENABLE_X4_BIT},//GD5F4GQ6xExxG 1.8V
	{0xaaef, 2048, 128, 64, 4096, 6, FLAGS_ENABLE_X4_BIT},//GD5F4GQ6xExxG 1.8V
	/* pg sz 2048, pg cnt 64, spare sz 64, block count 1024*/
	{0x0000, 2048, 64, 64, 1024, 6, 0}, // general setting
};


#define ROUND_UP_DIV(a, b) (((a) + (b)-1) / (b))

static uint8_t spi_nand_get_feature(uint32_t fe);
static int spi_nand_set_feature(uint8_t fe, uint8_t val);
static void spi_nand_write_en(void);
static void spi_nand_write_dis(void);
static int spi_nand_prog_exec(uint32_t row_addr);
static int spi_nand_read_page(uint32_t blk_id, uint32_t page_nr, void *buf, uint32_t len, uint32_t mode);
static uint8_t spi_nand_device_reset(void);
static void bm_spi_nand_enable_ecc(void);
static void spi_nand_polling_oip(void);
static uint32_t spi_nand_parsing_ecc_info(void);

void dump_hex(char *desc, void *addr, int len)
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
			printf("  %04x ", i);
		}

		/* Now the hex code for the specific character. */
		printf(" %02x", pc[i]);

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

static void spi_nand_polling_oip(void)
{
	uint32_t status = 0xff;

	while (status & SPI_NAND_STATUS0_OIP) {
		status = spi_nand_get_feature(SPI_NAND_FEATURE_STATUS0);
//		printf("spi_nand_polling_oip 0x%x\n", status);

	}

	return ;
}

static void spi_nand_get_features_all(void)
{
	uint32_t val = 0;
	val = spi_nand_get_feature(SPI_NAND_FEATURE_PROTECTION);
	printf("SPI_NAND_FEATURE_PROTECTION 0x%x\n", val);

	val = spi_nand_get_feature(SPI_NAND_FEATURE_FEATURE0);
	printf("SPI_NAND_FEATURE_FEATURE0 0x%x\n", val);

	val = spi_nand_get_feature(SPI_NAND_FEATURE_STATUS0);
	printf("SPI_NAND_FEATURE_STATUS0 0x%x\n", val);

//	val = spi_nand_get_feature(SPI_NAND_FEATURE_FEATURE1);
//	printf("SPI_NAND_FEATURE_FEATURE1 0x%x\n", val);
//
//	val = spi_nand_get_feature(SPI_NAND_FEATURE_STATUS1);
//	printf("SPI_NAND_FEATURE_STATUS1 0x%x\n", val);
}

static void bm_spi_nand_enable_ecc(void)
{
	__attribute__((unused)) uint8_t status = 0;
//	printf("%s\n", __func__);

	status = spi_nand_get_feature(SPI_NAND_FEATURE_FEATURE0);

	spi_nand_set_feature(SPI_NAND_FEATURE_FEATURE0, status | SPI_NAND_FEATURE0_ECC_EN); // set ECC_EN

	return;
}

static void bm_spi_nand_disable_ecc(void)
{
	__attribute__((unused)) uint8_t status = 0;
	printf("%s\n", __func__);

	status = spi_nand_get_feature(SPI_NAND_FEATURE_FEATURE0);

	spi_nand_set_feature(SPI_NAND_FEATURE_FEATURE0, status & ~(SPI_NAND_FEATURE0_ECC_EN)); // clear ECC_EN

	return;
}

static void spi_nand_setup_intr(void)
{
	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_EN, 3);
	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_CLR, BITS_SPI_NAND_INT_CLR_ALL);
	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_MASK, 0x00000f00);
}

static int spi_nand_set_feature(uint8_t fe, uint8_t val)
{
    uint32_t fe_set = fe | (val << 8);

//    printf("%s\n", __func__);

    mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL2, 2 << TRX_CMD_CONT_SIZE_SHIFT);
    mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3, BIT_REG_TRX_RW);
    mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0, (fe_set << TRX_CMD_CONT0_SHIFT) | SPI_NAND_CMD_SET_FEATURE);
//    printf("set fe reg 0x30 0x%08x\n", (fe_set << TRX_CMD_CONT0_SHIFT) | SPI_NAND_CMD_SET_FEATURE);

    mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL0, BIT_REG_TRX_START);

    while((mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT) & BIT_REG_TRX_DONE_INT) == 0);

    mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_CLR, BIT_REG_TRX_DONE_INT); // clr trx done intr

    return 0;
}

#if 0
struct spinand_cmd {
	u8		cmd;
	uint32_t		n_addr;		/* Number of address */
	u8		addr[3];	/* Reg Offset */
	uint32_t		n_dummy;	/* Dummy use */
	uint32_t		n_tx;		/* Number of tx bytes */
	u8		*tx_buf;	/* Tx buf */
	uint32_t		n_rx;		/* Number of rx bytes */
	u8		*rx_buf;	/* Rx buf */
	uint32_t		flags;		/* Flags */
};
#endif

__attribute__((unused)) static void spi_nand_send_cmd(struct spinand_cmd *scmd)
{

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL2, 1 << scmd->n_rx | scmd->n_cmd_cont << TRX_CMD_CONT_SIZE_SHIFT);

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3, scmd->flags);

	uint32_t cont = scmd->content[2] << 16 |scmd->content[1] << 8|scmd->content[0] << 0;
	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0, (cont << 8) | scmd->cmd);

	spi_nand_setup_intr();

	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL0, BIT_REG_TRX_START);

	while((mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT) & BIT_REG_TRX_DONE_INT) == 0);

	if (scmd->n_rx) {
		uint32_t n_dw = ROUND_UP_DIV(scmd->n_rx , 4); //ROUND_UP_DIV(a, b)
		for (uint32_t i = 0; i < n_dw; i++)
			scmd->rx_buf[i] = mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_RX_DATA);
	}

	uint32_t intr = mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT);
	//	printf("intr = 0x%x\n", intr);
	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_CLR, intr);

}


static uint8_t spi_nand_get_feature(uint32_t fe)
{
#if 1
	uint32_t val = 0;
//	printf("%s 1 fe 0x%x\n", __func__, fe);

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL2, 1 << TRX_DATA_SIZE_SHIFT | 1 << TRX_CMD_CONT_SIZE_SHIFT);
//	printf("%s 11 fe 0x%x\n", __func__, fe);

	//    mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3, 0x00080000);
	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3, BIT_REG_RSP_CHK_EN);
//	printf("%s 12 fe 0x%x\n", __func__, fe);

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0, fe << 8 | SPI_NAND_CMD_GET_FEATURE);
//	printf("%s 13 fe 0x%x\n", __func__, fe);

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_RSP_POLLING, 0xff00ff);
//	printf("%s 2\n", __func__);

//	spi_nand_setup_intr();

	// Trigger communication start
	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL0, BIT_REG_TRX_START);

	while((mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT) & BIT_REG_TRX_DONE_INT) == 0);
//	printf("%s 3\n", __func__);

	val = mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_RX_DATA);
//	printf("%s fe 0x%x\n", __func__, fe);
//	printf("Get feature value = 0x%x\n", val);

	uint32_t intr = mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT);
//	printf("intr = 0x%x\n", intr);
	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_CLR, intr);

	return (val & 0xff);
#else

#if 0
	struct spinand_cmd {
		u8		cmd;
		uint32_t 	n_cmd_cont; 	/* Number of command content */
		u8		content[3]; /* Command content */
		uint32_t 	n_dummy;	/* Dummy use */
		uint32_t 	n_tx;		/* Number of tx bytes */
		void	*tx_buf;	/* Tx buf */
		uint32_t 	n_rx;		/* Number of rx bytes */
		void	*rx_buf;	/* Rx buf */
		uint32_t 	flags;		/* Flags */
	};

#endif

	printf("%s 1 fe 0x%x\n", __func__, fe);

	uint32_t val = 0;
	struct spinand_cmd scmd = {0};

//	memset(&scmd, 0, sizeof(struct spinand_cmd));

	scmd.cmd = 0;
	scmd.n_cmd_cont = 1;
	scmd.content[2] = 0;
	scmd.content[1] = 0;
	scmd.content[0] = fe;
	scmd.n_dummy = 0;
	scmd.n_tx = 0; // align to 4 bytes
	scmd.tx_buf = 0;
	scmd.n_rx = 4; // align to 4 bytes
	scmd.rx_buf = (void *) &val;
	scmd.flags = 0;

	spi_nand_send_cmd(&scmd);

	printf("%s n fe 0x%x\n", __func__, fe);

	return (val & 0xff);

#endif
}

static void spi_nand_write_dis(void)
{
//    printf("%s\n", __func__);

    mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL2, 0x0);
    mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3, 0x0);
    mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0, SPI_NAND_CMD_WRDI); // command

//    spi_nand_setup_intr();

    mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL0, BIT_REG_TRX_START);

    while((mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT) & BIT_REG_TRX_DONE_INT) == 0); // should we wait?

    mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_CLR, BIT_REG_TRX_DONE_INT); // clr trx done intr

    return;
}

static void spi_nand_write_en(void)
{
//	printf("%s\n", __func__);

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL2, 0x0);
	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3, 0x0);
	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0, SPI_NAND_CMD_WREN); // command

//	spi_nand_setup_intr();

	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL0, BIT_REG_TRX_START);

	while((mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT) & BIT_REG_TRX_DONE_INT) == 0); // should we wait?

	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_CLR, BIT_REG_TRX_DONE_INT); // clr trx done intr

	return;
}

static uint8_t spi_nand_block_erase(uint32_t row_addr)
{
//	printf("%s row_addr 0x%x\n", __func__, row_addr);
	uint32_t r_row_addr = ((row_addr & 0xff0000) >> 16) | (row_addr & 0xff00) | ((row_addr & 0xff) << 16);
//	printf("%s r_row_addr 0x%x\n", __func__, r_row_addr);

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL2, 3 << TRX_CMD_CONT_SIZE_SHIFT); // 3 bytes for 24-bit row address
	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3, 0x0);
	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0, (r_row_addr << TRX_CMD_CONT0_SHIFT) | SPI_NAND_CMD_BLOCK_ERASE);
//	printf("%s REG_SPI_NAND_TRX_CMD0 0x%x\n", __func__, mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0));

//	spi_nand_setup_intr();

	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL0, BIT_REG_TRX_START);

	while((mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT) & BIT_REG_TRX_DONE_INT) == 0); // should we wait?

	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_CLR, BIT_REG_TRX_DONE_INT); // clr trx done intr

	return 0;
}

static uint8_t spi_nand_device_reset(void)
{
	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL2, 0); // 3 bytes for 24-bit row address
	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3, 0x0);
	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0, SPI_NAND_CMD_RESET);

	spi_nand_setup_intr();

	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL0, BIT_REG_TRX_START);

//	while((mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT) & BIT_REG_TRX_DONE_INT) == 0); // should we wait?
//
//	uint32_t intr = mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT);
//
//	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_CLR, intr); // clr intr

//	mdelay(1); //The OIP status can be read from 300ns after the reset command is sent

	return 0;
}

/*
 * spi_nand user guide v0.3.docx
 * 5.4 : Write Command with DMA Data
 *
 * DW_axi_dmac_databook（sysdma）.pdf
 *
 *
 * rw : 0 for read, 1 for write
*/
static void spi_nand_rw_dma_setup(void *buf, uint32_t len, uint32_t rw)
{
	uint32_t ch = 0;
//	printf("%s: buf %p, len %d\n", __func__, buf, len);

	mmio_write_32(DMAC_BASE + 0x010, 0x00000003);
	mmio_write_32(DMAC_BASE + 0x018, 0x00000f00);

	mmio_write_32(DMAC_BASE + 0x110 + ch * (0x100), (len / 4) - 1);
	mmio_write_32(DMAC_BASE + 0x11C + ch * (0x100), 0x000f0792);

	if (rw) {
		// for dma write
		mmio_write_64(DMAC_BASE + 0x100 + ch * (0x100), (uint64_t) buf);
		mmio_write_64(DMAC_BASE + 0x108 + ch * (0x100), spi_nand_ctrl_base + 0x800);
		mmio_write_32(DMAC_BASE + 0x118 + ch * (0x100), 0x00045441);
		mmio_write_32(DMAC_BASE + 0x124 + ch * (0x100), 0x00000001); // [0:2] = 1 : MEM_TO_PER_DMAC, PER dst = 0
		mmio_clrsetbits_32(TOP_DMA_CH_REMAP0, 0x3f << DMA_REMAP_CH0_OFFSET, DMA_SPI_NAND << DMA_REMAP_CH0_OFFSET);
		mmio_clrsetbits_32(TOP_DMA_CH_REMAP0, 0x1 << DMA_REMAP_UPDATE_OFFSET, 1 << DMA_REMAP_UPDATE_OFFSET);
	}
	else {
		mmio_write_64(DMAC_BASE + 0x100 + ch * (0x100), spi_nand_ctrl_base + 0xC00);
		mmio_write_64(DMAC_BASE + 0x108 + ch * (0x100), (uint64_t)buf);
		mmio_write_32(DMAC_BASE + 0x118 + ch * (0x100), 0x00046214);
		mmio_write_32(DMAC_BASE + 0x124 + ch * (0x100), 0x00000002); // [0:2] = 2 : PER_TO_MEM_DMAC, PER src = 0
		mmio_clrsetbits_32(TOP_DMA_CH_REMAP0, 0x3f << DMA_REMAP_CH0_OFFSET, DMA_SPI_NAND << DMA_REMAP_CH0_OFFSET);
		mmio_clrsetbits_32(TOP_DMA_CH_REMAP0, 0x1 << DMA_REMAP_UPDATE_OFFSET, 1 << DMA_REMAP_UPDATE_OFFSET);
	}

	mmio_write_32(DMAC_BASE + 0x120 + ch * (0x100), 0x0);
	mmio_write_32(DMAC_BASE + 0x018, 0x00000f0f);
}

void spi_nand_set_qe(uint32_t enable)
{
	if (enable) {
		uint32_t val = 0;
		val = spi_nand_get_feature(SPI_NAND_FEATURE_FEATURE0);
//		printf("before enable qe %x\n", val);

 		val |= SPI_NAND_FEATURE0_QE;
		spi_nand_set_feature(SPI_NAND_FEATURE_FEATURE0, val);
		val = spi_nand_get_feature(SPI_NAND_FEATURE_FEATURE0);
//		printf("enable qe %x\n", val);
	}
	else {
		uint32_t val = 0;
		val = spi_nand_get_feature(SPI_NAND_FEATURE_FEATURE0);
		val &= ~SPI_NAND_FEATURE0_QE;
		spi_nand_set_feature(SPI_NAND_FEATURE_FEATURE0, val);
		val = spi_nand_get_feature(SPI_NAND_FEATURE_FEATURE0);
//		printf("disable qe %x\n", val);
	}
}

/*
    Load x 1 : qe = 0
    Load x 4 : qe = 1
*/
static uint8_t spi_nand_prog_load(void *buf, size_t size, uint32_t col_addr, uint32_t qe)
{
	uint8_t cmd = qe ? SPI_NAND_CMD_PROGRAM_LOADX4 : SPI_NAND_CMD_PROGRAM_LOAD;
	uint32_t r_col_addr = ((col_addr & 0xff00) >> 8) | ((col_addr & 0xff) << 8);
	uint32_t ctrl3 = 0;
//	printf("%s size %d, col_addr 0x%x, r_col_addr 0x%x,  qe %d\n", __func__, size, col_addr, r_col_addr, qe);

	spi_nand_rw_dma_setup(buf, size, 1);

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL2, (size << TRX_DATA_SIZE_SHIFT) | 2 << TRX_CMD_CONT_SIZE_SHIFT);

	ctrl3 = qe ? (BIT_REG_TRX_RW | BIT_REG_TRX_DMA_EN | SPI_NAND_CTRL3_IO_TYPE_X4_MODE) : (BIT_REG_TRX_RW | BIT_REG_TRX_DMA_EN);
	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3, ctrl3);
//	printf("REG_SPI_NAND_TRX_CTRL3 0x%x\n", mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3));

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0, cmd | (r_col_addr << TRX_CMD_CONT0_SHIFT));
//	printf("REG_SPI_NAND_TRX_CMD0 0x%x\n", mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0));

	spi_nand_setup_intr();

	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL0, BIT_REG_TRX_START);

//	bm_spi_nand_dump_reg();

	while((mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT) & (BIT_REG_TRX_DONE_INT | BIT_REG_DMA_DONE_INT)) == 0); // should we wait?

	uint32_t intr = mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT);
	//	printf("intr = 0x%x\n", intr);
	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_CLR, intr);

	return 0;
}

static int spi_nand_prog_exec(uint32_t row_addr)
{
//	printf("%s row_addr 0x%x\n", __func__, row_addr);
	uint32_t r_row_addr = ((row_addr & 0xff0000) >> 16) | (row_addr & 0xff00) | ((row_addr & 0xff) << 16);
//	printf("%s r_row_addr 0x%x\n", __func__, r_row_addr);

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL2, 3 << TRX_CMD_CONT_SIZE_SHIFT);
	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3, 3);
	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0, SPI_NAND_CMD_PROGRAM_EXECUTE | (r_row_addr << TRX_CMD_CONT0_SHIFT)); // command
//	printf("%s REG_SPI_NAND_TRX_CMD0 0x%x\n", __func__, mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0));
	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_RSP_POLLING, 0xff00ff);

	spi_nand_setup_intr();

	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL0, BIT_REG_TRX_START);

//	bm_spi_nand_dump_reg();

	while((mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT) & BIT_REG_TRX_DONE_INT) == 0); // should we wait?

	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_CLR, BIT_REG_TRX_DONE_INT); // clr trx done intr

//	printf("%s done\n", __func__);

	return 0;
}

static void spi_nand_set_read_from_cache_mode(uint32_t mode, uint32_t r_col_addr)
{
//	printf("%s, mode %d\n", __func__, mode);
	switch (mode) {
	case SPI_NAND_READ_FROM_CACHE_MODE_X1:
//		printf("%s, 1\n", __func__);

		mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3, BIT_REG_TRX_DMA_EN | BIT_REG_TRX_DUMMY_HIZ);
		mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0, r_col_addr << TRX_CMD_CONT0_SHIFT | SPI_NAND_CMD_READ_FROM_CACHE);
		break;
	case SPI_NAND_READ_FROM_CACHE_MODE_X2:
//		printf("%s, 2\n", __func__);
		mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3, BIT_REG_TRX_DMA_EN | SPI_NAND_CTRL3_IO_TYPE_X2_MODE | BIT_REG_TRX_DUMMY_HIZ);
		mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0, r_col_addr << TRX_CMD_CONT0_SHIFT | SPI_NAND_CMD_READ_FROM_CACHEX2);
		break;
	case SPI_NAND_READ_FROM_CACHE_MODE_X4:

//		printf("%s, 3\n", __func__);

		mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3, BIT_REG_TRX_DMA_EN | SPI_NAND_CTRL3_IO_TYPE_X4_MODE);
		mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0, r_col_addr << TRX_CMD_CONT0_SHIFT | SPI_NAND_CMD_READ_FROM_CACHEX4);

//		printf("%s, 4\n", __func__);

		break;
	default:
		printf("unsupport mode!\n");
		assert(0);
		break;
	}

//	printf("%s REG_SPI_NAND_TRX_CTRL3 0x%x\n", __func__, mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3));
//	printf("%s REG_SPI_NAND_TRX_CMD0 0x%x\n", __func__, mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0));

	return;
}

static int spi_nand_read_page(uint32_t blk_id, uint32_t page_nr, void *buf, uint32_t len, uint32_t mode)
{
//	uint32_t status = 0xff;
	uint32_t row_addr = blk_id * NR_PG_PER_NATIVE_BLK + page_nr;
//	uint32_t m_len = 0 ? len / 4 : len;
//	uint32_t col_addr = 0;

//	printf("%s row_addr 0x%x\n", __func__, row_addr);
	uint32_t r_row_addr = ((row_addr & 0xff0000) >> 16) | (row_addr & 0xff00) | ((row_addr & 0xff) << 16);
//	printf("%s r_row_addr 0x%x\n", __func__, r_row_addr);

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL2, 0 << TRX_DATA_SIZE_SHIFT | 3 << TRX_CMD_CONT_SIZE_SHIFT); // 0x8

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3, 0);
	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0, r_row_addr << TRX_CMD_CONT0_SHIFT | SPI_NAND_CMD_PAGE_READ_TO_CACHE);
//	printf("%s REG_SPI_NAND_TRX_CMD0 0x%x\n", __func__, mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0));

	spi_nand_setup_intr();

	// Trigger communication start
	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL0, BIT_REG_TRX_START);
	//    bm_spi_nand_dump_reg();
	//    spi_nand_get_feature_all();

	bm_spi_nand_dump_reg();

	while((mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT) & BIT_REG_TRX_DONE_INT) == 0);

	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_CLR, BIT_REG_TRX_DONE_INT);

	spi_nand_polling_oip();

	return 0;
}

static int spi_nand_read_from_cache(uint32_t blk_id, uint32_t page_nr, uint32_t col_addr, uint32_t len, uint32_t mode, void *buf)
{
//	uint32_t status = 0xff;
	uint32_t r_col_addr = ((col_addr & 0xff00) >> 8) | ((col_addr & 0xff) << 8);

//	printf("%s col_addr 0x%x, r_col_addr 0x%x\n", __func__, col_addr, r_col_addr);

	spi_nand_rw_dma_setup(buf, len, 0);

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL2, len << TRX_DATA_SIZE_SHIFT | 3 << TRX_CMD_CONT_SIZE_SHIFT);

	spi_nand_set_read_from_cache_mode(mode, r_col_addr);

//	spi_nand_setup_intr();

	bm_spi_nand_dump_reg();

	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL0, BIT_REG_TRX_START);

	while((mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT) & (BIT_REG_TRX_DONE_INT | BIT_REG_DMA_DONE_INT)) == 0); // should we wait?

	uint32_t intr = mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT);
	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_CLR, intr);

	return 0;
}

//static uint8_t spi_nand_read_status(void)
//{
////	uint32_t status = 0;
//
////	printf("%s\n", __func__);
//
//	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL2, 0x00010001);
//	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3, 0x00080000);
//	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0, 0xc00f);
//	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_RSP_POLLING, 0x0100ff);
//	//    mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_RSP_POLLING, 0x01fe);
//
//	spi_nand_setup_intr();
//
//	// Trigger communication start
//	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL0, BIT_REG_TRX_START);
//
//	while((mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT) & BIT_REG_TRX_DONE_INT) == 0);
//
////	status = mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_RX_DATA);
//
////    printf("status0x%x\n", status);
//	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_CLR, BIT_REG_TRX_DONE_INT);
//
//	return 0;
//}

uint32_t bm_spi_nand_erase_block(uint32_t blk_id)
{
	assert(blk_id < MAX_BLK_ID);
	uint32_t row_addr = blk_id * NR_PG_PER_NATIVE_BLK;

	printf("%s erase blk id %d, row_addr 0x%08x, spinand_info->flags %d\n", __func__, blk_id, row_addr, spinand_info->flags);

//	bm_spi_nand_enable_ecc();
	spi_nand_set_feature(SPI_NAND_FEATURE_PROTECTION, FEATURE_PROTECTION_NONE);

	spi_nand_write_en();

	spi_nand_block_erase(row_addr);

	spi_nand_polling_oip();

	spi_nand_write_dis();

	return 0;
}

uint16_t bm_spi_nand_read_id(void)
{
	uint32_t read_id = 0;

//	printf("%s\n", __func__);

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL2, 0x00020001);
	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3, 0);
	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0, SPI_NAND_CMD_READ_ID);

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_BOOT_CTRL, BIT_REG_BOOT_PRD);

	spi_nand_setup_intr();

	// Trigger communication start

//	bm_spi_nand_dump_reg();

	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL0, BIT_REG_TRX_START);

	while((mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT) & BIT_REG_TRX_DONE_INT) == 0);

	read_id = mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_RX_DATA);

	printf("MID = 0x%x, DID = 0x%x\n", read_id & 0xff, (read_id >> 8) & 0xff);

	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_CLR, BIT_REG_TRX_DONE_INT);

	return read_id;
}

uint32_t bm_spi_nand_read_page_non_ecc(uint32_t blk_id, uint32_t pg_nr, void *buf, uint32_t len)
{
	__attribute__((unused)) uint8_t status = 0;
	printf("%s\n", __func__);

	status = spi_nand_get_feature(SPI_NAND_FEATURE_FEATURE0); //

	bm_spi_nand_disable_ecc();

	spi_nand_read_page(blk_id, pg_nr, buf, len, 0); // blk_id, page_nr

	//(uint32_t blk_id, uint32_t page_nr, uint32_t col_addr, uint32_t len, uint32_t mode, void *buf)
	spi_nand_read_from_cache(blk_id, pg_nr, 0, len, 0, buf);

	status = spi_nand_get_feature(SPI_NAND_FEATURE_FEATURE0); //

	bm_spi_nand_enable_ecc();

	return 0;
}

void bm_spi_nand_dump_reg(void)
{
#if DUMP_REG
	for (uint32_t i = 0; i < 0x80; i+= 4) {
		uint32_t reg = mmio_read_32(spi_nand_ctrl_base + i);
		printf("0x%x: 0x%x\n", i, reg);
	}
#endif
	return;
}

uint32_t bm_spi_nand_prg_read_cache(uint32_t blk_id, uint32_t pg_nr, void *buf, uint32_t len, uint32_t mode)
{
//	printf("\n%s, blk_id %d, pg_nr %d, mode %d\n", __func__, blk_id, pg_nr, mode);
	bm_spi_nand_enable_ecc();
//	spi_nand_get_feature(SPI_NAND_FEATURE_STATUS0);

	spi_nand_prog_load(buf, len, 0, 0);
	memset(buf, 0, len);

	//static int spi_nand_read_from_cache(uint32_t blk_id, uint32_t page_nr, uint32_t col_addr, uint32_t len, uint32_t mode, void *buf)
	spi_nand_read_from_cache(blk_id, pg_nr, 0, len, mode, buf);

	uint32_t status0 = spi_nand_get_feature(SPI_NAND_FEATURE_STATUS0);
	uint32_t status1 = spi_nand_get_feature(SPI_NAND_FEATURE_STATUS1);

	if (status1 || status0) {
		printf("status0 0x%x\n", status0);
		printf("status1 0x%x\n", status1);
//		uint32_t fe = spi_nand_get_feature(SPI_NAND_FEATURE_FEATURE0);
//		printf("fe 0x%x\n", fe);
		spi_nand_get_features_all();
	}
	return (status0 & 0xff) | (status1 & 0xff) << 8;
}

/*
Register Addr. 7        6        5      4      3        2        1        0
Status C0H     Reserved Reserved ECCS1  ECCS0  P_FAIL   E_FAIL   WEL      OIP
Status F0H     Reserved Reserved ECCSE1 ECCSE0 Reserved Reserved Reserved Reserved

ECCS1 ECCS0 ECCSE1 ECCSE0 Description
  0     0     x      x     No bit errors were detected during the previous read algorithm.
  0     1     0      0     Bit errors(<4) were detected and corrected.
  0     1     0      1     Bit errors(=5) were detected and corrected.
  0     1     1      0     Bit errors(=6) were detected and corrected.
  0     1     1      1     Bit errors(=7) were detected and corrected.
  1     0     x      x     Bit errors greater than ECC capability(8 bits) and not corrected
  1     1     x      x     Bit errors reach ECC capability( 8 bits) and corrected
*/

#define ECC_NOERR  0
#define ECC_CORR   1
#define ECC_UNCORR 2

uint32_t spi_nand_parsing_ecc_info(void)
{
	uint32_t statusc0 = spi_nand_get_feature(SPI_NAND_FEATURE_STATUS0);
	u8 ecc_sts = (statusc0 & 0x30) >> 4;

	if (ecc_sts) {
		printf("statusc0 0x%x\n", statusc0);
		printf("ecc sts : %d!!\n", ecc_sts);
	}

	if (ecc_sts == 0) {
//		printf("ECC no error!!\n");
		return ECC_NOERR;
	}
	else if (ecc_sts == 1) {
		printf("ECC_CORR\n");
		return ECC_CORR;
	}
	else if (ecc_sts == 2){
		printf("ECC_UNCORR!!\n");
		return ECC_UNCORR;
	}
	else if (ecc_sts == 3) {
		printf("reserved\n");
		return ECC_NOERR;
	}

	return 0;
}

uint32_t bm_spi_nand_read_page(uint32_t blk_id, uint32_t pg_nr, void *buf, uint32_t len, uint32_t mode)
{
//	printf("%s, blk_id %d, pg_nr %d, mode %d\n", __func__, blk_id, pg_nr, mode);
	uint32_t col_addr = 0;

	assert(mode < 3);

	if ((mode == SPI_NAND_READ_FROM_CACHE_MODE_X4) && spinand_info->flags & FLAGS_SET_QE_BIT) {
		spi_nand_set_qe(1); // Enable QE
	}

	bm_spi_nand_enable_ecc();

	spi_nand_get_feature(SPI_NAND_FEATURE_STATUS0);
	spi_nand_read_page(blk_id, pg_nr, buf, len, mode);

	if ((spinand_info->flags & FLAGS_SET_PLANE_BIT) && (blk_id & 1)) {
		col_addr |= SPI_NAND_PLANE_BIT_OFFSET;
	}

	spi_nand_read_from_cache(blk_id, pg_nr, col_addr, len, mode, buf);

	if ((mode == SPI_NAND_READ_FROM_CACHE_MODE_X4) && (spinand_info->flags & FLAGS_SET_QE_BIT)) {
		spi_nand_set_qe(0); // Disable QE
	}

	return spi_nand_parsing_ecc_info();
}

#if 1
/*
	size : from 512, 528, 2048 to 2172
*/
int bm_spi_nand_write_page(uint32_t blk_id, uint32_t page_nr, void *buf, size_t size, uint32_t qe)
{
	uint32_t row_addr = (blk_id << SPI_NAND_BLOCK_RA_SHIFT) | page_nr;
//	printf("%s : blk id %d, pg off %d, row_addr 0x%x\n", __func__, blk_id, page_nr, row_addr);

	bm_spi_nand_enable_ecc();
	spi_nand_set_feature(SPI_NAND_FEATURE_PROTECTION, FEATURE_PROTECTION_NONE);

	uint32_t col_addr = 0;

	if (qe && (spinand_info->flags & FLAGS_SET_QE_BIT)) {
		spi_nand_set_qe(1); // Enable QE
	}

	spi_nand_write_en();

	if ((spinand_info->flags & FLAGS_SET_PLANE_BIT) && (blk_id & 1)) {
		col_addr |= SPI_NAND_PLANE_BIT_OFFSET;
	}

	spi_nand_prog_load(buf, size, col_addr, qe);

	spi_nand_prog_exec(row_addr);

	spi_nand_polling_oip();

	spi_nand_write_dis();

	if (qe && (spinand_info->flags & FLAGS_SET_QE_BIT)) {
		spi_nand_set_qe(0); // disable QE
	}

	return 0;
}
#else
/*
	size : from 512, 528, 2048 to 2172
*/
int bm_spi_nand_write_page(uint32_t blk_id, uint32_t page_nr, void *buf, size_t size, uint32_t qe)
{
	uint32_t row_addr = (blk_id << SPI_NAND_BLOCK_RA_SHIFT) | page_nr;
//	printf("%s : blk id %d, pg off %d, row_addr 0x%x\n", __func__, blk_id, page_nr, row_addr);
//
//	bm_spi_nand_enable_ecc();
//	spi_nand_set_feature(SPI_NAND_FEATURE_PROTECTION, FEATURE_PROTECTION_NONE);
//
//	uint32_t col_addr = 0;
//
//	spi_nand_write_en();
//
//	spi_nand_prog_load(buf, size, col_addr, qe);

	spi_nand_prog_exec(0x123456);

//	spi_nand_polling_oip();
//
//	spi_nand_write_dis();

	return 0;
}

#endif

uint32_t bm_spi_nand_reset(void)
{
	printf("%s\n", __func__);

	spi_nand_device_reset();

	spi_nand_polling_oip();

	return 0;
}

static void lookup_spi_nand_list(uint16_t id)
{
	uint32_t i = 0;

	printf("%s id %x\n", __func__, id);

	for (i = 0; i < sizeof(spi_nand_support_list) / sizeof(struct spi_nand_info); i++) {
		printf("%s spi_nand_support_list[i].id %x\n", __func__, spi_nand_support_list[i].id);
		if (id == spi_nand_support_list[i].id) {
			printf("Got matched id %d\n", i);
			spinand_info = &spi_nand_support_list[i];
			return;
		}
	}

	if (i == sizeof(spi_nand_support_list) / sizeof(struct spi_nand_info)) {
		printf("Can't find matched device, use general setting!\n");
	}

	return;
}

/*
	This bit is set (OIP = 1 ) when a
	PROGRAM EXECUTE,
	PAGE READ,
	BLOCKERASE,
	or RESET
	command is executing, indicating the device is busy. When the bit
	is 0, the interface is in the ready state
*/

#define SPI_NAND_XTAL_SWITCH_REG 0x3002030
#define SPI_NAND_CLK_FROM_PLL	0xFFFFFEFF
#define SPI_NAND_CLK_FROM_XTAL	BIT(8)

uint32_t bm_spi_nand_init(uint32_t base, uint32_t clk_div, uint32_t cpha, uint32_t cpol)
{
	uint16_t id;
	uint32_t val = 0;

	printf("%s\n", __func__);

	spi_nand_ctrl_base = base;

	spi_nand_setup_intr();

	bm_spi_nand_reset();

	if ((cpha == 1) && (cpol == 1))
		mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL1, BIT_REG_IO_CPOL | BIT_REG_IO_CPHA);

	if (clk_div == 4)
		mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL1, BIT_REG_TRX_SCK_H | BIT_REG_TRX_SCK_L);

#if defined(BOARD_ASIC)
	val = mmio_read_32(SPI_NAND_XTAL_SWITCH_REG);
	printf("nand siwtch reg=0x%x\n", val);
	//mmio_setbits_32(SPI_NAND_XTAL_SWITCH_REG + REG_SPI_NAND_TRX_CTRL1);
	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL1, 0x3220900);
	printf("nand ctrl1 (0x4)=0x%x\n", mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL1));
	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_BOOT_CTRL, 0x100);
	printf("nand ctrl (0x24)=0x%x\n", mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_BOOT_CTRL));
#endif


	id = bm_spi_nand_read_id();

	lookup_spi_nand_list(id);

	if (spinand_info->id == 0) {
		printf("Not on support list\n");
		return -1;
	}

	val = spi_nand_get_feature(SPI_NAND_FEATURE_FEATURE0);
	printf("%s get feature3 %x\n", __func__, val);

	val &= ~SPI_NAND_FEATURE0_QE;
	spi_nand_set_feature(SPI_NAND_FEATURE_FEATURE0, val);
	val = spi_nand_get_feature(SPI_NAND_FEATURE_FEATURE0);
	printf("%s get feature4 %x\n", __func__, val);

	spi_nand_get_features_all();

	return 0;
}
