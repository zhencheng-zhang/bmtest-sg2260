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


struct dw_spi {
	/* Current message transfer state info */
	u16			len;
	void		*tx;
	void		*tx_end;
	void		*rx;
	void		*rx_end;
	u8			n_bytes;
};

bool loop_back = true;

static void dw_spi_enable(bool enable)
{
	if (enable == true)
		SPI_WRITE(DW_SPI_SSIENR, 0x1);
	else
		SPI_WRITE(DW_SPI_SSIENR, 0x0);

	udelay(5);

	printf("%sabling SPI\n", enable ? "En" : "Dis");
}


static int spi_rx(void *rx_buf, u16 len, u8 n_bytes)
{
	u32 max = len;
	u16 rxw;

	while (max--) {
		rxw = SPI_READ(DW_SPI_DR);
		/* Care rx only if the transfer's original "rx" is not null */
		if (n_bytes == 1)
			*(u8 *)(rx_buf) = rxw;
		else if (n_bytes == 2)
			*(u16 *)(rx_buf) = rxw;
		else {
			printf("No support of this format\n");
			return 1;
		}
		rx_buf += n_bytes;
	}
	return 0;
}


static int spi_tx(void *tx_buf, u16 len, u8 n_bytes)
{
	u32 max = len;
	u16 txw = 0;

	timer_meter_start();

	while (max > 0) {
		/* Set the tx word if the transfer's original "tx" is not null */
		if ((SPI_READ(DW_SPI_RISR) & SPI_RISR_TXOIR) != SPI_RISR_TXOIR) {
			timer_meter_start();

			if (n_bytes == 1)
				txw = *(u8 *)(tx_buf);
			else
				txw = *(u16 *)(tx_buf);

			SPI_WRITE(DW_SPI_DR, txw);
			tx_buf += n_bytes;
			max--;
		} else if (timer_meter_get_ms() < 100) {
			udelay(10); /* wait FIFO exit overflow status */
		} else {
			printf("SPI TX timeout\n");
			return 1;
		}
	}
	return 0;
}

static int spi_loopback(struct dw_spi *dws)
{
#ifdef CONFIG_USE_SPI_DMA
	dma_mem2dev_setting(dws->tx, dws->len * sizeof(u32), (unsigned int *)(SPI_BASE+DW_SPI_DR), DMA_SPI0);
	dma_start_transfer(DMA_SPI0);

	dma_dev2mem_setting(dws->rx, dws->len * sizeof(u32), (unsigned int *)(SPI_BASE+DW_SPI_DR), DMA_SPI0);
	dma_start_receive(DMA_SPI0);
#else
#ifndef CONFIG_TEST_WITH_ADAU1372
	u32 max = dws->len;
	u16 txw = 0;
	u16 rxw = 0;

	timer_meter_start();
	while (max > 0) {
		/* Set the tx word if the transfer's original "tx" is not null */
		if ((SPI_READ(DW_SPI_RISR) & SPI_RISR_TXOIR) != SPI_RISR_TXOIR) {
			timer_meter_start();

			if (dws->n_bytes == 1)
				txw = *(u8 *)(dws->tx);
			else
				txw = *(u16 *)(dws->tx);
			SPI_WRITE(DW_SPI_DR, txw);
			dws->tx += dws->n_bytes;
			udelay(1);

			rxw = SPI_READ(DW_SPI_DR);

			if (dws->n_bytes == 1)
				*(u8 *)(dws->rx) = rxw;
			else
				*(u16 *)(dws->rx) = rxw;

			dws->rx += dws->n_bytes;
			max--;
		} else if (timer_meter_get_ms() < 100) {
			udelay(10); /* wait FIFO exit overflow status */
		} else {
			printf("SPI TX timeout\n");
			return 1;
		}
	}
#else
	SPI_WRITE(DW_SPI_DR, 0x01);
	SPI_WRITE(DW_SPI_DR, 0x00);
	SPI_WRITE(DW_SPI_DR, 0x40);
	SPI_WRITE(DW_SPI_DR, 0xFF);
	SPI_WRITE(DW_SPI_DR, 0xFF);
	SPI_WRITE(DW_SPI_DR, 0xFF);
#endif
#endif
	return 0;
}

/*
 * The legacy I2C functions. These need to get removed once
 * all users of this driver are converted to DM.
 */
//static struct spi_regs *spi_get_base(void)
//{
//	return (struct spi_regs *)SPI_BASE;
//}

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
		SPI_WRITE(DW_SPI_CTRL0, ctrl0| SPI_TMOD_TR | SPI_SCPOL_LOW | SPI_SCPH_MIDDLE | SPI_FRF_SPI | SPI_DFS_8BIT); /* Set to SPI frame format */
	else if (n_bytes == 2)
		SPI_WRITE(DW_SPI_CTRL0, ctrl0| SPI_TMOD_TR |SPI_SCPOL_LOW | SPI_SCPH_MIDDLE | SPI_FRF_SPI | SPI_DFS_16BIT);
	SPI_WRITE(DW_SPI_BAUDR, SPI_BAUDR_DIV);
	SPI_WRITE(DW_SPI_TXFLTR, 4);
	SPI_WRITE(DW_SPI_RXFLTR, 4);
	SPI_WRITE(DW_SPI_SER, 0x1); /* enable slave 1 device*/

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
int testcase_spi(void)
{
	struct dw_spi dws;
	u8 idx = 0;


	dws.tx = (void *)default_tx;
	dws.len = ARRAY_SIZE(default_tx);
	dws.tx_end = dws.tx + dws.len;
	dws.rx = (void *)default_rx;
	dws.rx_end = dws.rx + dws.len;

#ifdef CONFIG_USE_SPI_DMA
    dma_channel_init();
#endif

	printf("\n------------ SPI test start ------------\n\n");

	dws.n_bytes = 1; /* test 8 bits data */
	spi_init(dws.n_bytes);

	printf("TX data:");
	for(idx=0; idx < dws.len; idx++)
		printf("%X ", default_tx[idx]);
	printf("\n\n");
	if (loop_back == true)
		spi_loopback(&dws);
	else {
		spi_tx(dws.tx, dws.len, dws.n_bytes);
		spi_rx(dws.rx, dws.len, dws.n_bytes);
	}
	printf("RX data:");
	for(idx=0; idx < dws.len; idx++)
		printf("%X ", default_rx[idx]);
	printf("\n\n");

	printf("\n------------ SPI test done ------------\n");

	return 0;
}

#if 0
static int irq_handler(int irqn, void *priv)
{

	printf("%s irqn=%d\n", __func__, irqn);
	disable_irq(irqn);

    return 0;
}
#endif

static int do_trx(int argc, char **argv)
{
	int ret;

	ret = testcase_spi();
	uartlog("testcase spi %s\n", ret?"failed":"passed");

	return ret;
}

static int do_inv_cs(int argc, char **argv)
{
	struct dw_spi dws;
	u8 idx = 0;
	uint8_t tmp_tx[] = {0x2};
	uint8_t tmp_rx[] = {0x0};

	dws.tx = (void *)tmp_tx;
	dws.len = ARRAY_SIZE(tmp_tx);
	dws.tx_end = dws.tx + dws.len;
	dws.rx = (void *)tmp_rx;
	dws.rx_end = dws.rx + dws.len;

	printf("\n------------ SPI CS invert test start ------------\n\n");

	dws.n_bytes = 1; /* test 8 bits data */
	spi_init(dws.n_bytes);

	dw_spi_enable(false);
	SPI_WRITE(DW_SPI_CTRL2, CVI_SS_INV_EN);
	dw_spi_enable(true);

	printf("TX data:");
	for(idx=0; idx < dws.len; idx++)
		printf("%X ", tmp_tx[idx]);
	printf("\n\n");
	if (loop_back == true)
		spi_loopback(&dws);
	else {
		spi_tx(dws.tx, dws.len, dws.n_bytes);
		spi_rx(dws.rx, dws.len, dws.n_bytes);
	}
	printf("RX data:");
	for(idx=0; idx < dws.len; idx++)
		printf("%X ", tmp_rx[idx]);
	printf("\n\n");

	printf("\n------------ SPI CS invert test done ------------\n");

	return 0;
}

static int do_lsb(int argc, char **argv)
{
	struct dw_spi dws;
	u8 idx = 0;
	uint8_t tmp_tx[] = {0x2};
	uint8_t tmp_rx[] = {0x0};

	dws.tx = (void *)tmp_tx;
	dws.len = ARRAY_SIZE(tmp_tx);
	dws.tx_end = dws.tx + dws.len;
	dws.rx = (void *)tmp_rx;
	dws.rx_end = dws.rx + dws.len;

	printf("\n------------ SPI CS LSB test start ------------\n\n");

	dws.n_bytes = 1; /* test 8 bits data */
	spi_init(dws.n_bytes);

	dw_spi_enable(false);
	SPI_WRITE(DW_SPI_CTRL2, CVI_LAB_FIRST);
	dw_spi_enable(true);

	printf("TX data with len %d: \n", dws.len);
	for(idx=0; idx < dws.len; idx++)
		printf("%X ", tmp_tx[idx]);
	printf("\n\n");
	if (loop_back == true)
		spi_loopback(&dws);
	else {
		spi_tx(dws.tx, dws.len, dws.n_bytes);
		spi_rx(dws.rx, dws.len, dws.n_bytes);
	}
	printf("RX data:");
	for(idx=0; idx < dws.len; idx++)
		printf("%X ", tmp_rx[idx]);
	printf("\n\n");

	printf("\n------------ SPI CS LSB test done ------------\n");

	return 0;
}

static struct cmd_entry test_cmd_list[] __attribute__ ((unused)) = {
	{"trx", do_trx, 0, "do spi loop test"},
	{"invert_cs", do_inv_cs, 0, "invert cs test (active high)"},
	{"lsb", do_lsb, 0, "TRX with LSB"},
	{NULL, NULL, 0, NULL}
};

int testcase_main(void)
{
  int i;
#if 0
  request_irq(SPI_0_SSI_INTR, irq_handler, 0, "SPI0 interrupt", NULL);
  request_irq(SPI_1_SSI_INTR, irq_handler, 0, "SPI1 interrupt", NULL);
  request_irq(SPI_2_SSI_INTR, irq_handler, 0, "SPI2 interrupt", NULL);
  request_irq(SPI_3_SSI_INTR, irq_handler, 0, "SPI3 interrupt", NULL);
#endif
//   pinmux_config(PINMUX_SPI0);

  printf("Commands:\n");
  printf("	trx - do normal TX/RX\n");
  printf("	invert_cs - do cs invert test\n");
  printf("	lsb - TX/RX by LSB first\n");
  for(i = 0;i < ARRAY_SIZE(test_cmd_list) - 1;i++) {
	command_add(&test_cmd_list[i]);
  }
  cli_simple_loop();

  return 0;
}

module_testcase("1", testcase_spi);
