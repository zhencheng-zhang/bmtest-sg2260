/*
 * Copyright (c) 2016-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "plat_def.h"
#include "debug.h"
#include "mmio.h"
#include "bm_sdhci.h"
#include "mmc.h"
#include "cache.h"

static void bm_mmc_hw_init(struct mmc_host *host);
static int bm_mmc_send_cmd(struct mmc_host *host, mmc_cmd_t *cmd);
static int bm_mmc_read(int lba, uintptr_t buf, size_t size);
static int bm_mmc_write(int lba, uintptr_t buf, size_t size);
static int bm_mmc_start_signal_voltage_switch(struct mmc_host *mmc, struct mmc_ios *ios);
static int bm_mmc_card_busy(struct mmc_host *mmc);
static int bm_mmc_set_ios(struct mmc_host *host, struct mmc_ios *ios);
static int bm_mmc_select_drive_strength(struct mmc_card *card,
				       unsigned int max_dtr, int host_drv,
				       int card_drv, int *drv_type);
#if MMC_TUNING_SOFT
static int bm_mmc_execute_software_tuning(struct mmc_host *host, uint32_t opcode);
#else
static int bm_mmc_execute_tuning(struct mmc_host *host, uint32_t opcode);
#endif

struct mmc_ops common_ops = {
	.init           = bm_mmc_hw_init,
	.send_cmd       = bm_mmc_send_cmd,
	.set_ios        = bm_mmc_set_ios,
	.read           = bm_mmc_read,
	.write          = bm_mmc_write,
#if MMC_TUNING_SOFT
	.execute_tuning = bm_mmc_execute_software_tuning,
#else
	.execute_tuning = bm_mmc_execute_tuning,
#endif
	.start_signal_voltage_switch = bm_mmc_start_signal_voltage_switch,
	.select_drive_strength = bm_mmc_select_drive_strength,
	.card_busy		= bm_mmc_card_busy,
};

static void bm_sdhci_adma_write_cmd_desc(struct mmc_host *host, void *desc,
				uint32_t reg, unsigned int attr)
{
	struct sdhci_adma3_cmd_desc *cmd_desc = desc;

	cmd_desc->attr = attr;
	cmd_desc->reg = reg;
	cmd_desc->reserved = 0;
	cmd_desc->reserved2 = 0;
}

static void bm_sdhci_desc_reg_set(struct mmc_host *host, int reg, uint32_t value)
{
	struct sdhci_adma3_cmd_desc *cmd_desc = host->adma_table;

	if (SDHCI_DMA_ADDRESS == reg) {
		cmd_desc[0].reg = value;
	} else if (SDHCI_BLOCK_SIZE == reg) {
		cmd_desc[1].reg |= value & 0xffff;
	} else if (SDHCI_BLOCK_COUNT == reg) {
		cmd_desc[1].reg |= (value & 0xffff) << 16;
	} else if (SDHCI_ARGUMENT == reg) {
		cmd_desc[2].reg = value;
	} else if (SDHCI_TRANSFER_MODE == reg) {
		cmd_desc[3].reg |=  value & 0xffff;
	} else if (SDHCI_COMMAND == reg) {
		cmd_desc[3].reg |= (value & 0xffff) << 16;
	}
}

static void bm_sdhci_adma_write_desc(struct mmc_host *host, void *desc,
				uint64_t addr, unsigned int len, unsigned int attr)
{
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	struct sdhci_adma2_64_desc *adma_desc = desc;

	adma_desc->attr = attr;
	if (mmio_read_16(base + SDHCI_HOST_CONTROL2) & SDHCI_HOST_ADMA2_LEN_MODE) {
		adma_desc->len_10 = len;
		adma_desc->len_16 = len >> 10;
	} else {
		adma_desc->len_16 = len;
	}

	adma_desc->addr = addr;
}

static void bm_sdhci_cmd_desc_init(struct mmc_host *host)
{
	void *desc = host->adma_table;

	bm_sdhci_adma_write_cmd_desc(host, desc, 0, ADMA3_CMD_VALID);
	desc += 8;
	bm_sdhci_adma_write_cmd_desc(host, desc, 0, ADMA3_CMD_VALID);
	desc += 8;
	bm_sdhci_adma_write_cmd_desc(host, desc, 0, ADMA3_CMD_VALID);
	desc += 8;
	bm_sdhci_adma_write_cmd_desc(host, desc, 0, ADMA3_CMD_VALID);
}

#define min(x, y) ((x) > (y) ? (y) : (x))
static void bm_sdhci_adma_table_pre(struct mmc_host *host, struct mmc_data *data)
{
	uint32_t remain_size, buf_size;
	void *desc;
	uint64_t addr;

	remain_size = data->blksz * data->blocks;
	addr = (uint64_t)data->buf;
	desc = host->adma_table;
	if (host->flags & SDHCI_USE_ADMA3)
		desc += 32;

	while (remain_size) {
		buf_size = min(host->adma_buf_size, remain_size);
		bm_sdhci_adma_write_desc(host, desc, addr, buf_size, ADMA2_TRAN_VALID);

		addr += buf_size;
		desc += host->desc_sz;
		remain_size -= buf_size;

		if ((desc - host->adma_table) >= host->adma_table_sz) {
			printf("Exceed adma table capacity!!!\n");
			ASSERT(0);
		}
	}

	bm_sdhci_adma_write_desc(host, desc, 0, 0, ADMA2_NOP_END_VALID);
}

static int bm_mmc_prepare_data(struct mmc_host *host, mmc_cmd_t *cmd)
{
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	struct mmc_data *data = cmd->data;
	uint64_t load_addr;
	uint32_t block_cnt, blksz;
	uint8_t tmp;

	ASSERT(((uint64_t)data->buf & SD_BLOCK_MASK) == 0);

	blksz = data->blksz;
	block_cnt = data->blocks;

	if (host->flags & (SDHCI_USE_ADMA2 | SDHCI_USE_ADMA3)) {
		bm_sdhci_adma_table_pre(host, data);

		if (host->flags & SDHCI_USE_ADMA3) {
			bm_sdhci_cmd_desc_init(host);
			bm_sdhci_desc_reg_set(host, SDHCI_DMA_ADDRESS, block_cnt);
			bm_sdhci_desc_reg_set(host, SDHCI_BLOCK_COUNT, 0);
			bm_sdhci_desc_reg_set(host, SDHCI_BLOCK_SIZE, SDHCI_MAKE_BLKSZ(7, blksz));

			tmp = mmio_read_8(base + SDHCI_HOST_CONTROL);
			tmp &= ~SDHCI_CTRL_DMA_MASK;
			tmp |= SDHCI_CTRL_ADMA3;
			mmio_write_8(base + SDHCI_HOST_CONTROL, tmp);
			flush_dcache_range((unsigned long)host->adma_table, (unsigned long)host->adma_table + host->adma_table_sz);

			return 0;
		}

		load_addr = (uint64_t)host->adma_table;
		flush_dcache_range((unsigned long)host->adma_table, (unsigned long)host->adma_table + host->adma_table_sz);
	} else {
		load_addr = (uint64_t)data->buf;
	}

	if (mmio_read_16(base + SDHCI_HOST_CONTROL2) & SDHCI_HOST_VER4_ENABLE) {
		if (host->flags & SDHCI_REQ_USE_DMA) {
			mmio_write_32(base + SDHCI_ADMA_SA_LOW, load_addr);
			mmio_write_32(base + SDHCI_ADMA_SA_HIGH, (load_addr >> 32));
		}
		mmio_write_32(base + SDHCI_DMA_ADDRESS, block_cnt);
		mmio_write_16(base + SDHCI_BLOCK_COUNT, 0);
	} else {
		ASSERT((load_addr >> 32) == 0);

		if (host->flags & SDHCI_REQ_USE_DMA) {
			mmio_write_32(base + SDHCI_DMA_ADDRESS, load_addr);
		}
		mmio_write_16(base + SDHCI_BLOCK_COUNT, block_cnt);
	}

	mmio_write_16(base + SDHCI_BLOCK_SIZE, SDHCI_MAKE_BLKSZ(7, blksz));

	// select SDMA
	tmp = mmio_read_8(base + SDHCI_HOST_CONTROL);
	tmp &= ~SDHCI_CTRL_DMA_MASK;

	if (host->flags & SDHCI_REQ_USE_DMA) {
		if (host->flags & SDHCI_USE_ADMA2)
			tmp |= SDHCI_CTRL_ADMA2;
		else
			tmp |= SDHCI_CTRL_SDMA;
	}
	mmio_write_8(base + SDHCI_HOST_CONTROL, tmp);

	return 0;
}

static void bm_sdhci_read_block_pio(struct mmc_host *host)
{
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	size_t blksize, chunk;
	uint32_t scratch;
	uint8_t *buf;
	uint32_t block_cnt;

	pr_debug("PIO reading\n");

	blksize = host->data->blksz;
	block_cnt = host->data->blocks;

	buf = (uint8_t *)((uint64_t)host->data->buf + ((block_cnt - 1) * blksize));
	chunk = 0;
	scratch = 0;

	while (blksize) {
		if (chunk == 0) {
			scratch = mmio_read_32(base + SDHCI_BUFFER);
			chunk = 4;
		}

		*buf = scratch & 0xFF;

		buf++;
		scratch >>= 8;
		chunk--;
		blksize--;
	}
}

static void bm_sdhci_write_block_pio(struct mmc_host *host)
{
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	size_t blksize, chunk;
	uint32_t scratch;
	uint8_t *buf;
	uint32_t block_cnt;

	pr_debug("PIO writing\n");

	blksize = host->data->blksz;
	block_cnt = host->data->blocks;

	buf = (uint8_t *)((uint64_t)host->data->buf + ((block_cnt - 1) * blksize));
	chunk = 0;
	scratch = 0;

	while (blksize) {
		scratch |= (uint32_t)*buf << (chunk * 8);

		buf++;
		chunk++;
		blksize--;

		if ((chunk == 4) || (blksize == 0)) {
			mmio_write_32(base + SDHCI_BUFFER, scratch);
			chunk = 0;
			scratch = 0;
		}
	}
}

static void bm_sdhci_transfer_pio(struct mmc_host *host)
{
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	uint32_t mask;

	if (host->data->blocks == 0)
		return;

	if (host->data->flags & MMC_DATA_READ)
		mask = SDHCI_DATA_AVAILABLE;
	else
		mask = SDHCI_SPACE_AVAILABLE;

	while (mmio_read_32(base + SDHCI_PRESENT_STATE) & mask) {
		if (host->data->flags & MMC_DATA_READ)
			bm_sdhci_read_block_pio(host);
		else
			bm_sdhci_write_block_pio(host);

		host->data->blocks--;
		if (host->data->blocks == 0)
			break;
	}

	pr_debug("PIO transfer complete.\n");
}

static int bm_mmc_check_cmd_stats(struct mmc_host *host)
{
	int i;
	volatile mmc_cmd_t *cmd = host->cmd;
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	uint32_t stat;

	// check cmd complete if necessary
	if ((mmio_read_16(base + SDHCI_TRANSFER_MODE)
				& SDHCI_TRNS_RESP_INT) == 0) {
		while (1) {
			stat = mmio_read_16(base + SDHCI_INT_STATUS);
			if (stat & SDHCI_INT_ERROR)
			{
				printf("status reg 0x30 0x%x\n", mmio_read_16(base + SDHCI_INT_STATUS));
				printf("Error status 0x32 0x%x\n", mmio_read_16(base + SDHCI_ERR_INT_STATUS));
				return -EIO;
			}
			if (stat & SDHCI_INT_RESPONSE) {
				mmio_write_16(base + SDHCI_INT_STATUS, SDHCI_INT_RESPONSE);
				break;
			}
		}

		if (cmd->resp_type & MMC_RSP_PRESENT) {
			if (cmd->resp_type & MMC_RSP_136) {
				/* CRC is stripped so we need to do some shifting. */
				for (i = 0; i < 4; i++) {
					cmd->resp_data[i] = mmio_read_32(base + SDHCI_RESPONSE_01 + (3-i)*4) << 8;
					if (i != 3)
						cmd->resp_data[i] |= mmio_read_8(base + SDHCI_RESPONSE_01 + (3-i)*4-1);
				}
			} else {
				cmd->resp_data[0] = mmio_read_32(base + SDHCI_RESPONSE_01);
			}
		}
	}

	return 0;
}

static int bm_mmc_check_data_stats(struct mmc_host *host)
{
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	uint32_t stat, dma_addr;

	if (host->flags & SDHCI_REQ_USE_DMA) {
		// check dma/transfer complete
		while (1) {
			stat = mmio_read_16(base + SDHCI_INT_STATUS);
			if (stat & SDHCI_INT_ERROR)
			{
				printf("status reg 0x30 0x%x\n", mmio_read_16(base + SDHCI_INT_STATUS));
				printf("Error status 0x32 0x%x\n", mmio_read_16(base + SDHCI_ERR_INT_STATUS));
				return -EIO;
			}

			if (stat & SDHCI_INT_DATA_END) {
				mmio_write_16(base + SDHCI_INT_STATUS, stat);
				break;
			}

			if ((host->flags & SDHCI_USE_SDMA) && (stat & SDHCI_INT_DMA_END)) {
				mmio_write_16(base + SDHCI_INT_STATUS, stat);
				if (mmio_read_16(base + SDHCI_HOST_CONTROL2) & SDHCI_HOST_VER4_ENABLE) {
					dma_addr = mmio_read_32(base + SDHCI_ADMA_SA_LOW);
					mmio_write_32(base + SDHCI_ADMA_SA_LOW, dma_addr);
					mmio_write_32(base + SDHCI_ADMA_SA_HIGH, 0);
				} else {
					dma_addr = mmio_read_32(base + SDHCI_DMA_ADDRESS);
					mmio_write_32(base + SDHCI_DMA_ADDRESS, dma_addr);
				}
			}
		}
	} else {
		while (1) {
			stat = mmio_read_16(base + SDHCI_INT_STATUS);
			if (stat & (SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL)) {
				mmio_write_16(base + SDHCI_INT_STATUS, SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL);
				bm_sdhci_transfer_pio(host);
			}

			if (stat & SDHCI_INT_ERROR)
			{
				printf("status reg 0x30 0x%x\n", mmio_read_16(base + SDHCI_INT_STATUS));
				printf("Error status 0x32 0x%x\n", mmio_read_16(base + SDHCI_ERR_INT_STATUS));
				return -EIO;
			}

			if (stat & SDHCI_INT_DATA_END) {
				mmio_write_16(base + SDHCI_INT_STATUS, stat);
				break;
			}

		}
	}


	return 0;
}

static void bm_sdhci_host_init_for_irq(struct mmc_host *host)
{
	host->cmd_completion = 0;
	host->data_completion = 0;
	host->cmd->error = 0;

	if (host->data) {
		host->data->error = 0;
	}
}

static void bm_sdhci_wait_cmd_complete(struct mmc_host *host)
{
	while (!host->cmd_completion) {
		pr_debug("%s\n", __func__);
	}
	host->cmd_completion = 0;
}

static void bm_sdhci_wait_data_complete(struct mmc_host *host)
{
	while (!host->data_completion) {
		pr_debug("%s\n", __func__);
	}
	host->data_completion = 0;
}

static int bm_mmc_send_data_cmd(struct mmc_host *host, mmc_cmd_t *cmd)
{
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	struct mmc_data *data = cmd->data;
	uint32_t mode = 0, flags = 0;

	//make sure cmd line is clear
	while (1) {
		if (!(mmio_read_32(base + SDHCI_PRESENT_STATE) & SDHCI_CMD_INHIBIT))
			break;
	}

	bm_mmc_prepare_data(host, cmd);

	if (host->flags & SDHCI_REQ_USE_DMA)
		mode = SDHCI_TRNS_DMA;

	if (mmc_op_multi(cmd->cmd_idx) || data->blocks > 1) {
		mode |= SDHCI_TRNS_MULTI | SDHCI_TRNS_BLK_CNT_EN;
	}

	if (data->flags & MMC_DATA_READ) {
		mode |= SDHCI_TRNS_READ;
    invalidate_dcache_range((unsigned long)data->buf,((unsigned long)data->buf) + data->blksz * data->blocks);
	} else {
		mode &= ~SDHCI_TRNS_READ;
	}

	// set cmd flags
	if (cmd->cmd_idx == 0)
		flags |= SDHCI_CMD_RESP_NONE;
	else if (cmd->resp_type == MMC_RSP_R2)
		flags |= SDHCI_CMD_RESP_LONG;
	else
		flags |= SDHCI_CMD_RESP_SHORT;
	flags |= SDHCI_CMD_CRC;
	flags |= SDHCI_CMD_INDEX;
	flags |= SDHCI_CMD_DATA;

	if (host->flags & SDHCI_SDIO_IRQ_ENABLED) {
		bm_sdhci_host_init_for_irq(host);
	}

	if (host->flags & SDHCI_USE_ADMA3) {
		bm_sdhci_desc_reg_set(host, SDHCI_TRANSFER_MODE, mode);
		bm_sdhci_desc_reg_set(host, SDHCI_ARGUMENT, cmd->cmd_arg);
		bm_sdhci_desc_reg_set(host, SDHCI_COMMAND, SDHCI_MAKE_CMD(cmd->cmd_idx, flags));
		mmio_write_64(base + SDHCI_ADMA_ID_LOW_R, (uint64_t)host->intgr_table);

		flush_dcache_range((unsigned long)host->adma_table, (unsigned long)host->adma_table + host->adma_table_sz);
	} else {
		mmio_write_16(base + SDHCI_TRANSFER_MODE, mode);
		mmio_write_32(base + SDHCI_ARGUMENT, cmd->cmd_arg);
		// issue the cmd
		mmio_write_16(base + SDHCI_COMMAND,
				SDHCI_MAKE_CMD(cmd->cmd_idx, flags));
	}

	if (host->flags & SDHCI_SDIO_IRQ_ENABLED) {
		bm_sdhci_wait_cmd_complete(host);
		bm_sdhci_wait_data_complete(host);

		if (cmd->error || cmd->data->error) {
			printf("%s command transfer failed\n", __func__);
			return -1;
		}
	} else {
		if (bm_mmc_check_cmd_stats(host)) {
			printf("%s cmd stat err\n", __func__);
			return -1;
		}

		if (bm_mmc_check_data_stats(host)) {
			printf("%s data stat err\n", __func__);
			return -1;
		}
	}

	return 0;
}

static int bm_mmc_send_nondata_cmd(struct mmc_host *host, mmc_cmd_t *cmd)
{
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	uint32_t flags = 0x0;
#if PLD_MODULE_QUIRK_WAIT_DAT0

	printf("%s wait dat up\n", __func__);
	while (1) {
		if ((mmio_read_32(base + SDHCI_PRESENT_STATE) & SDHCI_DATA0_LVL_MASK))
			break;
	}
	printf("%s dat is up\n", __func__);
	
#endif
	//make sure cmd line is clear
	while (1) {
		if (!(mmio_read_32(base + SDHCI_PRESENT_STATE) & SDHCI_CMD_INHIBIT))
			break;
	}

	// set cmd flags
	if (cmd->cmd_idx == MMC_CMD0)
		flags |= SDHCI_CMD_RESP_NONE;
	else if (cmd->cmd_idx == MMC_CMD1)
		flags |= SDHCI_CMD_RESP_SHORT;
	else if (cmd->cmd_idx == SD_ACMD41)
		flags |= SDHCI_CMD_RESP_SHORT;
	else if (cmd->resp_type == MMC_RSP_R2) {
		flags |= SDHCI_CMD_RESP_LONG;
		flags |= SDHCI_CMD_CRC;
	} else {
		flags |= SDHCI_CMD_RESP_SHORT;
		flags |= SDHCI_CMD_CRC;
		flags |= SDHCI_CMD_INDEX;
	}

	// make sure dat line is clear if necessary
	if (flags & SDHCI_CMD_RESP_SHORT_BUSY) {
		while (1) {
			if (!(mmio_read_32(base + SDHCI_PRESENT_STATE) & SDHCI_CMD_INHIBIT_DAT))
				break;
		}
	}

	if (cmd->cmd_idx == MMC_CMD19 || cmd->cmd_idx == MMC_CMD21)
		flags |= SDHCI_CMD_DATA;

	if (host->flags & SDHCI_SDIO_IRQ_ENABLED) {
		bm_sdhci_host_init_for_irq(host);
	}

	// issue the cmd
	mmio_write_32(base + SDHCI_ARGUMENT, cmd->cmd_arg);
	mmio_write_16(base + SDHCI_COMMAND,
			SDHCI_MAKE_CMD(cmd->cmd_idx, flags));

	if (cmd->cmd_idx == MMC_CMD19 || cmd->cmd_idx == MMC_CMD21)
		return 0;

	if (host->flags & SDHCI_SDIO_IRQ_ENABLED) {
		bm_sdhci_wait_cmd_complete(host);

		if (cmd->error) {
			printf("%s command transfer failed\n", __func__);
			return -1;
		}
	} else {
		if (bm_mmc_check_cmd_stats(host)) {
			printf("%s cmd stat err\n", __func__);
			return -1;
		}
	}

	return 0;
}

static int bm_mmc_send_cmd(struct mmc_host *host, mmc_cmd_t *cmd)
{
#if 1
	printf("send cmd, cmd_idx=%d, cmd_arg=0x%x, resp_type=0x%x\n",
			cmd->cmd_idx,
			cmd->cmd_arg,
			cmd->resp_type);
#endif

	host->cmd = cmd;
	if (cmd->data) {
		host->data_cmd = cmd;
		host->data = cmd->data;
		return bm_mmc_send_data_cmd(host, cmd);
	} else {
		host->data_cmd = NULL;
		host->data = NULL;
		return bm_mmc_send_nondata_cmd(host, cmd);
	}
}

static void bm_mmc_set_clk(struct mmc_host *host, int clk)
{
	int i, div;
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;

	ASSERT(clk > 0);

	if (param->f_max <= clk) {
		div = 0;
	} else {
		for (div=0x1; div<0xFF; div++) {
			if (param->f_max / (2 * div) <= clk)
				break;
		}
		if (div == 0xFF) {
			pr_debug("Warning: Can't set the freq to %d, divider is filled!!!\n", clk);
		}
	}
	ASSERT(div <= 0xFF);

	if (mmio_read_16(base + SDHCI_HOST_CONTROL2) & 1<<15) {
		printf("Use SDCLK Preset Value\n");
	} else {
		VERBOSE("Set SDCLK by driver. div=0x%x(%d)\n", div, div);
		mmio_write_16(base + SDHCI_CLK_CTRL,
				mmio_read_16(base + SDHCI_CLK_CTRL) & ~0x9); // disable INTERNAL_CLK_EN and PLL_ENABLE
		mmio_write_16(base + SDHCI_CLK_CTRL,
				(mmio_read_16(base + SDHCI_CLK_CTRL) & 0xDF) | div << 8); // set clk div
		mmio_write_16(base + SDHCI_CLK_CTRL,
				mmio_read_16(base + SDHCI_CLK_CTRL) | 0x1); // set INTERNAL_CLK_EN

		for (i = 0; i <= 150000; i += 100) {
			if (mmio_read_16(base + SDHCI_CLK_CTRL) & 0x2) {
				break;
			}
			udelay(100);
		}

		if (i > 150000) {
			ERROR("SD INTERNAL_CLK_EN seting FAILED!\n");
			ASSERT(0);
		}

		mmio_write_16(base + SDHCI_CLK_CTRL,
				mmio_read_16(base + SDHCI_CLK_CTRL) | 0x8); // set PLL_ENABLE

		for (i = 0; i <= 150000; i += 100) {
			if (mmio_read_16(base + SDHCI_CLK_CTRL) & 0x2) {
				return;
			}
			udelay(100);
		}
	}

	ERROR("SD PLL seting FAILED!\n");
	return;
}

static void bm_mmc_change_clk(struct mmc_host *host, int clk)
{
	int div;
	int i;
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;

	if (clk <= 0) {
		mmio_write_16(base + SDHCI_CLK_CTRL,
				mmio_read_16(base + SDHCI_CLK_CTRL) & ~(0x1<<2)); // stop SD clock
		return;
	}

	if (param->f_max <= clk) {
		div = 0;
	} else {
		for (div=0x1; div<0xFF; div++) {
			if (param->f_max / (2 * div) <= clk)
				break;
		}
		if (div == 0xFF) {
			pr_debug("Warning: Can't set the freq to %d, divider is filled!!!\n", clk);
		}
	}
	ASSERT(div <= 0xFF);

	mmio_write_16(base + SDHCI_CLK_CTRL,
			mmio_read_16(base + SDHCI_CLK_CTRL) & ~(0x1<<2)); // stop SD clock

	mmio_write_16(base + SDHCI_CLK_CTRL,
			mmio_read_16(base + SDHCI_CLK_CTRL) & ~0x8); // disable  PLL_ENABLE

	if (mmio_read_16(base + SDHCI_HOST_CONTROL2) & (1 << 15)) {
		printf("Use SDCLK Preset Value\n");
		// 4 need recheck?
		mmio_write_16(base + SDHCI_HOST_CONTROL2,
				mmio_read_16(base + SDHCI_HOST_CONTROL2) & ~0x7); // clr UHS_MODE_SEL
	} else {
		VERBOSE("Set SDCLK by driver. div=0x%x(%d)\n", div, div);
		// 5 need recheck?

		// 6
		mmio_write_16(base + SDHCI_CLK_CTRL,
				(mmio_read_16(base + SDHCI_CLK_CTRL) & 0xDF) | div << 8); // set clk div
		mmio_write_16(base + SDHCI_CLK_CTRL,
				mmio_read_16(base + SDHCI_CLK_CTRL) & ~(0x1 << 5)); // CLK_GEN_SELECT
	}
	mmio_write_16(base + SDHCI_CLK_CTRL,
			mmio_read_16(base + SDHCI_CLK_CTRL) | 0xc); // enable  PLL_ENABLE

	for (i = 0; i <= 150000; i += 100) {
		if (mmio_read_16(base + SDHCI_CLK_CTRL) & 0x2) {
			return;
		}
		udelay(100);
	}

	ERROR("SD PLL seting FAILED!\n");
	return;
}

#if MMC_TUNING_SOFT
static int bm_mmc_execute_software_tuning(struct mmc_host *host, uint32_t opcode)
{
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	uint32_t ctl2;
	mmc_cmd_t cmd;
	uint32_t norm_stat, err_stat;
	uint32_t norm_stat_en_b, err_stat_en_b;
	uint32_t norm_signal_en_b;
	uintptr_t vendor_base;
	int min, max;

	vendor_base = base + (mmio_read_16(base + P_VENDOR_SPECIFIC_AREA) & ((1<<12)-1));

	mmio_write_32(vendor_base + 0x8, 0x0000);
	norm_stat_en_b = mmio_read_16(base + SDHCI_INT_STATUS_EN);
	err_stat_en_b = mmio_read_16(base + SDHCI_ERR_INT_STATUS_EN);
	norm_signal_en_b = mmio_read_32(base + SDHCI_SIGNAL_ENABLE);

	mmio_write_32(base + SDHCI_SIGNAL_ENABLE, 0);
	mmio_write_16(base + SDHCI_INT_STATUS_EN, SDHCI_INT_BUF_RD_READY);
	mmio_write_16(base + SDHCI_ERR_INT_STATUS_EN, 0x7f);

	mmio_write_16(base + SDHCI_INT_STATUS, 0xff);
	mmio_write_16(base + SDHCI_ERR_INT_STATUS, 0xff);

	MMC_PACK_CMD(cmd, opcode, 0, MMC_RSP_R1);

	ctl2 = mmio_read_16(base + SDHCI_HOST_CONTROL2);
	ctl2 |= SDHCI_CTRL_EXEC_TUNING;
	mmio_write_16(base + SDHCI_HOST_CONTROL2, ctl2);

	mmio_write_32(vendor_base + 0x40, 0x0018);
	min = 0;
	while (min < 128) {
		mmio_write_32(vendor_base + 0x44, min);

		mmio_write_16(base + SDHCI_BLOCK_SIZE, SDHCI_MAKE_BLKSZ(7, 64));

		mmio_write_16(base + SDHCI_TRANSFER_MODE, SDHCI_TRNS_READ);

		host->ops->send_cmd(host, &cmd);

		while (1) {
			norm_stat = mmio_read_16(base + SDHCI_INT_STATUS);
			err_stat = mmio_read_16(base + SDHCI_ERR_INT_STATUS);
			if (norm_stat & SDHCI_INT_BUF_RD_READY)
				break;
			if (err_stat & 0x7f)
				break;
		}
		mmio_write_16(base + SDHCI_INT_STATUS, 0xff);
		mmio_write_16(base + SDHCI_ERR_INT_STATUS, 0xff);

		mmio_write_8(base + SDHCI_SOFTWARE_RESET, 0x6);
		while (mmio_read_8(base + SDHCI_SOFTWARE_RESET))
			;

		if (err_stat == 0) {
			break;
		}
		min++;
	}


	if (min >= 128) {
		printf("%s tuning failed\n", __func__);
		return -1;
	} else {
		pr_debug("\t\b min:%d\n", min);

		max = min + 1;
		while (max < 128) {
			pr_debug("\t\b max:%d\n", max);
			mmio_write_32(vendor_base + 0x44, max);

			mmio_write_16(base + SDHCI_BLOCK_SIZE, SDHCI_MAKE_BLKSZ(7, 64));

			mmio_write_16(base + SDHCI_TRANSFER_MODE, SDHCI_TRNS_READ);

			host->ops->send_cmd(host, &cmd);

			while (1) {
				norm_stat = mmio_read_16(base + SDHCI_INT_STATUS);
				err_stat = mmio_read_16(base + SDHCI_ERR_INT_STATUS);
				if (norm_stat & SDHCI_INT_BUF_RD_READY)
					break;
				if (err_stat & 0x7f)
					break;
			}
			mmio_write_16(base + SDHCI_INT_STATUS, 0xff);
			mmio_write_16(base + SDHCI_ERR_INT_STATUS, 0xff);

			mmio_write_8(base + SDHCI_SOFTWARE_RESET, 0x6);
			while (mmio_read_8(base + SDHCI_SOFTWARE_RESET))
				;

			if (err_stat != 0) {
				break;
			}
			max++;
		}

		pr_debug("\t\b max:%d\n", max);

		mmio_write_32(vendor_base + 0x44, min + ((max - min) * 3 / 4));

		mmio_write_8(base + SDHCI_SOFTWARE_RESET, 0x6);
		while (mmio_read_8(base + SDHCI_SOFTWARE_RESET))
			;

		ctl2 = mmio_read_16(base + SDHCI_HOST_CONTROL2);
		ctl2 &= ~SDHCI_CTRL_EXEC_TUNING;
		mmio_write_16(base + SDHCI_HOST_CONTROL2, ctl2);
	}

	mmio_write_16(base + SDHCI_INT_STATUS_EN, norm_stat_en_b);
	mmio_write_32(base + SDHCI_SIGNAL_ENABLE, norm_signal_en_b);
	mmio_write_16(base + SDHCI_ERR_INT_STATUS_EN, err_stat_en_b);

	printf("%s finished tuning, code:%d\n", __func__, min + ((max - min) * 3 / 4));
	return 0;
}
#else
int bm_mmc_execute_tuning(struct mmc_host *host, uint32_t opcode)
{
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	int tuning_loop_counter = 1024;
	uint32_t ctl2;
	mmc_cmd_t cmd;
	uint32_t stat;
	uint32_t vendor_base;

	vendor_base = base + (mmio_read_16(base + P_VENDOR_SPECIFIC_AREA) & ((1<<12)-1));
	mmio_write_32(vendor_base + 0x8, 0x0000);
	mmio_write_32(vendor_base + 0x40, 0x1F1B0001);

	mmio_write_8(base + SDHCI_SOFTWARE_RESET, 0x6);
	while (mmio_read_8(base + SDHCI_SOFTWARE_RESET))
		;

	printf("%s\n", __func__);
	ctl2 = mmio_read_16(base + SDHCI_HOST_CONTROL2);
	ctl2 |= SDHCI_CTRL_EXEC_TUNING;
	ctl2 &= ~SDHCI_CTRL_TUNED_CLK;
	mmio_write_16(base + SDHCI_HOST_CONTROL2, ctl2);

	//we enable all interrupt status here for testing
	mmio_write_16(base + SDHCI_INT_STATUS_EN, mmio_read_16(base + SDHCI_INT_STATUS_EN) | SDHCI_INT_BUF_RD_READY);

	do {
		if (tuning_loop_counter-- == 0)
			break;

		MMC_PACK_CMD(cmd, opcode, 0, MMC_RSP_R1);

		mmio_write_16(base + SDHCI_BLOCK_SIZE, SDHCI_MAKE_BLKSZ(7, 64));

		mmio_write_16(base + SDHCI_TRANSFER_MODE, SDHCI_TRNS_READ);

		host->ops->send_cmd(host, &cmd);

		while (1) {
			stat = mmio_read_16(base + SDHCI_INT_STATUS);
			if (stat & SDHCI_INT_BUF_RD_READY) {
				mmio_write_16(base + SDHCI_INT_STATUS, stat | SDHCI_INT_BUF_RD_READY);
				break;
			}
		}

		mdelay(1);

		ctl2 = mmio_read_16(base + SDHCI_HOST_CONTROL2);
	} while (ctl2 & SDHCI_CTRL_EXEC_TUNING);

	if (ctl2 & SDHCI_CTRL_EXEC_TUNING)
	{
		printf("Tune procedure out!\n");
	}
	else
	{
		printf("Tune procedure finish\n");
		if (ctl2 & SDHCI_CTRL_TUNED_CLK )
		{
			printf("Tune procedure success!\n");
		}
		else
		{
			printf("Tune procedure fail!\n");
		}
		
	}
	

	mmio_write_8(base + SDHCI_SOFTWARE_RESET, 0x6);
	while (mmio_read_8(base + SDHCI_SOFTWARE_RESET))
		;

	if (tuning_loop_counter < 0) {
		ctl2 &= ~SDHCI_CTRL_TUNED_CLK;
		mmio_write_16(base + SDHCI_HOST_CONTROL2, ctl2);
		printf("%s exceed loop counter\n", __func__);
	}

	if (!(ctl2 & SDHCI_CTRL_TUNED_CLK)) {
		printf("Tune procedure failed, falling back to fixed sampling clock!\n");
		return 0;
	}


	return 0;
}
#endif
static int bm_mmc_select_drive_strength(struct mmc_card *card,
				       unsigned int max_dtr, int host_drv,
				       int card_drv, int *drv_type)
{
	struct bm_mmc_params *param = card->host->priv;
	uintptr_t base = param->reg_base;
	int driver_type = MMC_SET_DRIVER_TYPE_B;
	uint32_t value;

	pr_debug("%s\n", __func__);

	*drv_type = SD_DRIVER_TYPE_B;
	if (max_dtr == UHS_SDR104_BUS_SPEED || max_dtr == MMC_HS200_MAX_DTR) {
		driver_type = MMC_SET_DRIVER_TYPE_A;
		*drv_type = SD_DRIVER_TYPE_A;

		value = (1 << PHY_CNFG_PHY_PWRGOOD) | (0xe << PHY_CNFG_PAD_SP) | (0xe << PHY_CNFG_PAD_SN) | (1 << PHY_CNFG_PHY_RSTN);

	}
	else if (max_dtr == UHS_DDR50_BUS_SPEED) {
		driver_type = MMC_SET_DRIVER_TYPE_B;
		*drv_type = SD_DRIVER_TYPE_B;
		value = (1 << PHY_CNFG_PHY_PWRGOOD) | (0x8 << PHY_CNFG_PAD_SP) | (0x8 << PHY_CNFG_PAD_SN) | (1 << PHY_CNFG_PHY_RSTN);
	}
	else if (max_dtr == UHS_SDR50_BUS_SPEED) {
		driver_type = MMC_SET_DRIVER_TYPE_B;
		*drv_type = SD_DRIVER_TYPE_B;
		value = (1 << PHY_CNFG_PHY_PWRGOOD) | (0xc << PHY_CNFG_PAD_SP) | (0xc << PHY_CNFG_PAD_SN) | (1 << PHY_CNFG_PHY_RSTN);
	}
	else {
		return 0;
	}

	// Set PAD_SN PAD_SP
	mmio_write_32(base + SDHCI_P_PHY_CNFG, value);

	return driver_type;
}

void bm_mmc_hw_reset(struct mmc_host *host)
{
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;

	// EMMC
	mmio_write_32(REG_TOP_SOFT_RST0,
			mmio_read_32(REG_TOP_SOFT_RST0) & (~BIT_TOP_SOFT_RST0_EMMC));
	udelay(10);
	mmio_write_32(REG_TOP_SOFT_RST0,
			mmio_read_32(REG_TOP_SOFT_RST0) | BIT_TOP_SOFT_RST0_EMMC);
	udelay(10);

	// SD
	mmio_write_32(REG_TOP_SOFT_RST0,
			mmio_read_32(REG_TOP_SOFT_RST0) & (~BIT_TOP_SOFT_RST0_SD));
	udelay(10);
	mmio_write_32(REG_TOP_SOFT_RST0,
			mmio_read_32(REG_TOP_SOFT_RST0) | BIT_TOP_SOFT_RST0_SD);
	udelay(10);

	// reset hw
	mmio_write_8(base + SDHCI_SOFTWARE_RESET, 0x7);
	while (mmio_read_8(base + SDHCI_SOFTWARE_RESET))
		;
}

void bm_mmc_phy_init(struct mmc_host *host)
{
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;

	// Wait for the PHY power on ready
	while (!(mmio_read_32(base + SDHCI_P_PHY_CNFG) & (1 << PHY_CNFG_PHY_PWRGOOD))) {
		;
	}

	// Asset reset of phy
	mmio_write_32(base + SDHCI_P_PHY_CNFG, mmio_read_32(base + SDHCI_P_PHY_CNFG) & ~(1 << PHY_CNFG_PHY_RSTN));

	// Set PAD_SN PAD_SP
	mmio_write_32(base + SDHCI_P_PHY_CNFG, (1 << PHY_CNFG_PHY_PWRGOOD) | (0x9 << PHY_CNFG_PAD_SP) | (0x8 << PHY_CNFG_PAD_SN));

	// Set CMDPAD
	mmio_write_16(base + SDHCI_P_CMDPAD_CNFG,
			(0x2 << PAD_CNFG_RXSEL) | (1 << PAD_CNFG_WEAKPULL_EN) |
			(0x3 << PAD_CNFG_TXSLEW_CTRL_P) | (0x2 << PAD_CNFG_TXSLEW_CTRL_N));

	// Set DATAPAD
	mmio_write_16(base + SDHCI_P_DATPAD_CNFG,
			(0x2 << PAD_CNFG_RXSEL) | (1 << PAD_CNFG_WEAKPULL_EN) |
			(0x3 << PAD_CNFG_TXSLEW_CTRL_P) | (0x2 << PAD_CNFG_TXSLEW_CTRL_N));

	// Set CLKPAD
	mmio_write_16(base + SDHCI_P_CLKPAD_CNFG,
			(0x2 << PAD_CNFG_RXSEL) | (0x3 << PAD_CNFG_TXSLEW_CTRL_P) | (0x2 << PAD_CNFG_TXSLEW_CTRL_N));

	// Set STB_PAD
	mmio_write_16(base + SDHCI_P_STBPAD_CNFG,
			(0x2 << PAD_CNFG_RXSEL) | (0x2 << PAD_CNFG_WEAKPULL_EN) |
			(0x3 << PAD_CNFG_TXSLEW_CTRL_P) | (0x2 << PAD_CNFG_TXSLEW_CTRL_N));

	// Set RSTPAD
	mmio_write_16(base + SDHCI_P_RSTNPAD_CNFG,
			(0x2 << PAD_CNFG_RXSEL) | (1 << PAD_CNFG_WEAKPULL_EN) |
			(0x3 << PAD_CNFG_TXSLEW_CTRL_P) | (0x2 << PAD_CNFG_TXSLEW_CTRL_N));

	// Set SDCLKDL_CNFG, EXTDLY_EN = 1, fix delay
	mmio_write_8(base + SDHCI_P_SDCLKDL_CNFG, (1 << SDCLKDL_CNFG_EXTDLY_EN));

	// Add 70 * 10 ps = 0.7ns
	mmio_write_8(base + SDHCI_P_SDCLKDL_DC, 10);

	// Set SMPLDL_CNFG, Bypass
	mmio_write_8(base + SDHCI_P_SMPLDL_CNFG, (1 << SMPLDL_CNFG_BYPASS_EN));
#if 0
	mmio_write_8(base + SDHCI_P_SMPLDL_CNFG, (2 << SMPLDL_CNFG_INPSEL_CNFG));
#endif


	// Set ATDL_CNFG, tuning clk not use for init
	mmio_write_8(base + SDHCI_P_ATDL_CNFG, (2 << ATDL_CNFG_INPSEL_CNFG));

	// deasset reset of phy
	mmio_write_32(base + SDHCI_P_PHY_CNFG, mmio_read_32(base + SDHCI_P_PHY_CNFG) | (1 << PHY_CNFG_PHY_RSTN));
}

static void bm_sdhci_finish_command(struct mmc_host *host)
{
	int i;
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	volatile mmc_cmd_t *cmd = host->cmd;

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136) {
			/* CRC is stripped so we need to do some shifting. */
			for (i = 0; i < 4; i++) {
				cmd->resp_data[i] = mmio_read_32(base + SDHCI_RESPONSE_01 + (3-i)*4) << 8;
				if (i != 3)
					cmd->resp_data[i] |= mmio_read_8(base + SDHCI_RESPONSE_01 + (3-i)*4-1);
			}
		} else {
			cmd->resp_data[0] = mmio_read_32(base + SDHCI_RESPONSE_01);
		}
	}
}

static void bm_sdhci_cmd_irq(struct mmc_host *host, uint32_t intmask)
{
	if (!host->cmd) {
		pr_debug("%s Got command interrupt 0x%08x even through no command operation was in progress.\n", __func__, intmask);

		return;
	}

	if (intmask & (SDHCI_INT_TIMEOUT | SDHCI_INT_CRC |
		       SDHCI_INT_END_BIT | SDHCI_INT_INDEX)) {
		if (intmask & SDHCI_INT_TIMEOUT)
			host->cmd->error = -ETIMEDOUT;
		else
			host->cmd->error = -EILSEQ;

		host->cmd = NULL;
		return;
	}

	if (intmask & SDHCI_INT_RESPONSE)
		bm_sdhci_finish_command(host);
}

static void bm_sdhci_data_irq(struct mmc_host *host, uint32_t intmask)
{
	volatile mmc_cmd_t *data_cmd = host->data_cmd;
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	uint32_t command;
	uint32_t dma_addr;

	/* CMD19 generates _only_ Buffer Read Ready interrupt */
	if (intmask & SDHCI_INT_DATA_AVAIL) {
		command = SDHCI_GET_CMD(mmio_read_16(base + SDHCI_COMMAND));
		if (command == MMC_CMD19 ||
		    command == MMC_CMD19) {
			//host->tuning_done = 1;
			return;
		}
	}

	if (data_cmd && (data_cmd->data->flags & MMC_RSP_BUSY)) {
		if (intmask & SDHCI_INT_DATA_TIMEOUT) {
			host->data_cmd = NULL;
			data_cmd->error = -ETIMEDOUT;
			return;
		}
		if (intmask & SDHCI_INT_DATA_END) {
			host->data_cmd = NULL;
			return;
		}
	}

	if (intmask & SDHCI_INT_DATA_TIMEOUT)
		host->data->error = -ETIMEDOUT;
	else if (intmask & SDHCI_INT_DATA_END_BIT)
		host->data->error = -EILSEQ;
	else if (intmask & SDHCI_INT_DATA_CRC) {
		host->data->error = -EILSEQ;
	}
	else if (intmask & SDHCI_INT_ADMA_ERROR) {
		host->data->error = -EIO;
	}

	if (host->data->error) {
		host->data = NULL;
		host->data_cmd = NULL;
	} else {
		if (intmask & (SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL))
			bm_sdhci_transfer_pio(host);

		if (intmask & SDHCI_INT_DATA_END) {
			host->data = NULL;
			host->data_cmd = NULL;
			return;
		}

		if ((host->flags & SDHCI_USE_SDMA) && (intmask & SDHCI_INT_DMA_END)) {
			if (mmio_read_16(base + SDHCI_HOST_CONTROL2) & SDHCI_HOST_VER4_ENABLE) {
				dma_addr = mmio_read_32(base + SDHCI_ADMA_SA_LOW);
				mmio_write_32(base + SDHCI_ADMA_SA_LOW, dma_addr);
				mmio_write_32(base + SDHCI_ADMA_SA_HIGH, 0);
			} else {
				dma_addr = mmio_read_32(base + SDHCI_DMA_ADDRESS);
				mmio_write_32(base + SDHCI_DMA_ADDRESS, dma_addr);
			}
		}
	}

	return;
}

static int bm_sdhci_irq_handler(int irqn, void *priv)
{
	struct mmc_host *host = priv;
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	uint32_t intmask, mask, unexpected;
	int max_loop = 16;

	intmask = mmio_read_32(base + SDHCI_INT_STATUS);

	if (!intmask || intmask == 0xffffffff) {
		printf("%s never be here!\n", __func__);
		return -1;
	}

	pr_debug("%s got interrupt: 0x%08x\n", __func__, intmask);

	do {
		mask = intmask & (SDHCI_INT_CMD_MASK | SDHCI_INT_DATA_MASK |
				SDHCI_INT_BUS_POWER);

		mmio_write_32(base + SDHCI_INT_STATUS, mask);

		if (intmask & SDHCI_INT_CMD_MASK) {
			bm_sdhci_cmd_irq(host, intmask & SDHCI_INT_CMD_MASK);
			host->cmd_completion = 1;
		}

		if (intmask & SDHCI_INT_DATA_MASK) {
			bm_sdhci_data_irq(host, intmask & SDHCI_INT_DATA_MASK);
			host->data_completion = 1;
		}

		intmask &= ~(SDHCI_INT_CARD_INSERT | SDHCI_INT_CARD_REMOVE |
				SDHCI_INT_CMD_MASK | SDHCI_INT_DATA_MASK |
				SDHCI_INT_ERROR | SDHCI_INT_BUS_POWER |
				SDHCI_INT_RETUNE | SDHCI_INT_CARD_INT);

		if (intmask) {
			unexpected = intmask;
			mmio_write_32(base + SDHCI_INT_STATUS, intmask);
			printf("%s unexpected interrupt: 0x%08x\n", __func__, unexpected);
		}

		intmask = mmio_read_32(base + SDHCI_INT_STATUS);
	} while (intmask && --max_loop);

	return 0;
}

static void bm_mmc_hw_init(struct mmc_host *host)
{
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	uint16_t ctrl;

	// Wait for phy power good

	// reset data & cmd
	mmio_write_8(base + SDHCI_SOFTWARE_RESET, 0x6);

	// init common parameters
	if (host->flags & SDHCI_TYPE_EMMC)
	{
		ctrl = mmio_read_16(base + SDHCI_HOST_CONTROL2);
		ctrl |= SDHCI_CTRL_VDD_180;
		mmio_write_16(base + SDHCI_HOST_CONTROL2, ctrl);
	}
	else
	{
		ctrl = mmio_read_16(base + SDHCI_HOST_CONTROL2);
		ctrl &= ~SDHCI_CTRL_VDD_180;
		mmio_write_16(base + SDHCI_HOST_CONTROL2, ctrl);
	}
	
	mmio_write_8(base + SDHCI_TOUT_CTRL, 0xe);  // for TMCLK 50Khz
	mmio_write_16(base + SDHCI_HOST_CONTROL2,
			mmio_read_16(base + SDHCI_HOST_CONTROL2) | 1<<11);  // set cmd23 support

	mmio_write_16(base + SDHCI_CLK_CTRL, mmio_read_16(base + SDHCI_CLK_CTRL) & ~(0x1 << 5));  // divided clock mode

	if (host->flags & SDHCI_USE_VERSION4) {
		// set host version 4 parameters
		mmio_write_16(base + SDHCI_HOST_CONTROL2,
				mmio_read_16(base + SDHCI_HOST_CONTROL2) | SDHCI_HOST_VER4_ENABLE); // set HOST_VER4_ENABLE
	}
	if (mmio_read_32(base + SDHCI_CAPABILITIES1) & (0x1 << 27)) {
		mmio_write_16(base + SDHCI_HOST_CONTROL2,
				mmio_read_16(base + SDHCI_HOST_CONTROL2) | 0x1<<13); // set 64bit addressing
	}

	// if support asynchronous int
	if (mmio_read_32(base + SDHCI_CAPABILITIES1) & (0x1<<29)) {
		mmio_write_16(base + SDHCI_HOST_CONTROL2, mmio_read_16(base + SDHCI_HOST_CONTROL2) | (0x1<<14)); // enable async int
	}

	// give some time to power down card
#ifndef PLATFORM_PALLADIUM
	mdelay(20);
#endif

	// 12
	mmio_write_16(base + SDHCI_HOST_CONTROL2,
			mmio_read_16(base + SDHCI_HOST_CONTROL2) & ~(0x1<<8)); // clr UHS2_IF_ENABLE
	mmio_write_8(base + SDHCI_PWR_CONTROL,
			mmio_read_8(base + SDHCI_PWR_CONTROL) | 0x1); // set SD_BUS_PWR_VDD1
	mmio_write_16(base + SDHCI_HOST_CONTROL2,
			mmio_read_16(base + SDHCI_HOST_CONTROL2) & ~0x7); // clr UHS_MODE_SEL

	// 13
	bm_mmc_set_clk(host, host->ios.clock);

#ifndef PLATFORM_PALLADIUM
	mdelay(50);
#endif

	// 15
	mmio_write_16(base + SDHCI_CLK_CTRL,
			mmio_read_16(base + SDHCI_CLK_CTRL) | (0x1<<2)); // supply SD clock

	// 16
	udelay(400); // wait for voltage ramp up time at least 74 cycle, 400us is 80 cycles for 200Khz

	mmio_write_16(base + SDHCI_INT_STATUS, mmio_read_16(base + SDHCI_INT_STATUS) | (0x1 << 6));

	//we enable all interrupt status here for testing
	mmio_write_16(base + SDHCI_INT_STATUS_EN, mmio_read_16(base + SDHCI_INT_STATUS_EN) | 0xFFFF);
	mmio_write_16(base + SDHCI_ERR_INT_STATUS_EN,mmio_read_16(base + SDHCI_ERR_INT_STATUS_EN) | 0xFFFF);

	if (host->ier) {
		if (host->flags & SDHCI_TYPE_EMMC)
			request_irq(EMMC_INTR, bm_sdhci_irq_handler, 0, "software int", host);
		else
			request_irq(SDIO_INTR, bm_sdhci_irq_handler, 0, "software int", host);

		mmio_write_32(base + SDHCI_SIGNAL_ENABLE, host->ier);
	} else {
		mmio_write_32(base + SDHCI_SIGNAL_ENABLE, 0);
		disable_irq(SDIO_INTR);
	}

	VERBOSE("SD init done\n");
}

static void bm_mmc_set_uhs_signaling(struct mmc_host *host, unsigned int uhs)
{
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	u16 ctrl_2;

	ctrl_2 = mmio_read_16(base + SDHCI_HOST_CONTROL2);
	/* Select Bus Speed Mode for host */
	ctrl_2 &= ~SDHCI_CTRL_UHS_MASK;
	switch (uhs) {
	case MMC_TIMING_SD_HS:
		ctrl_2 |= SDHCI_CTRL_UHS_SDR25;
		break;
	case MMC_TIMING_UHS_SDR12:
		ctrl_2 |= SDHCI_CTRL_UHS_SDR12;
		break;
	case MMC_TIMING_UHS_SDR25:
		ctrl_2 |= SDHCI_CTRL_UHS_SDR25;
		break;
	case MMC_TIMING_UHS_SDR50:
		ctrl_2 |= SDHCI_CTRL_UHS_SDR50;
		break;
	case MMC_TIMING_MMC_HS200:
	case MMC_TIMING_UHS_SDR104:
		ctrl_2 |= SDHCI_CTRL_UHS_SDR104;
		break;
	case MMC_TIMING_UHS_DDR50:
	case MMC_TIMING_MMC_DDR52:
		ctrl_2 |= SDHCI_CTRL_UHS_DDR50;
		break;
	}

	/*
	 * When clock frequency is less than 100MHz, the feedback clock must be
	 * provided and DLL must not be used so that tuning can be skipped. To
	 * provide feedback clock, the mode selection can be any value less
	 * than 3'b011 in bits [2:0] of HOST CONTROL2 register.
	 */
	if (host->clock <= 100000000 &&
	    (uhs == MMC_TIMING_MMC_HS400 ||
	     uhs == MMC_TIMING_MMC_HS200 ||
	     uhs == MMC_TIMING_UHS_SDR104))
		ctrl_2 &= ~SDHCI_CTRL_UHS_MASK;

	pr_debug("%s: clock=%u uhs=%u ctrl_2=0x%x\n",	__func__, host->clock, uhs, ctrl_2);
	mmio_write_16(base + SDHCI_HOST_CONTROL2, ctrl_2);
}

static int bm_mmc_set_ios(struct mmc_host *host, struct mmc_ios *ios)
{
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	uint32_t ctrl;

	pr_debug("%s ios->bus_width:%d, ios->clock:%d, ios->timing:0x%x\n", __func__, ios->bus_width, ios->clock, ios->timing);

	if (!ios->clock || ios->clock != host->clock) {
		bm_mmc_change_clk(host, ios->clock);
		host->clock = ios->clock;
	}

	ctrl = mmio_read_8(base + SDHCI_HOST_CONTROL);
	switch (ios->bus_width) {
		case MMC_BUS_WIDTH_1:
			ctrl &= ~SDHCI_DAT_XFER_WIDTH;
			break;
		case MMC_BUS_WIDTH_4:
			ctrl |= SDHCI_DAT_XFER_WIDTH;
			break;
		default:
			ASSERT(0);
	}

	if ((ios->timing == MMC_TIMING_MMC_HS200) ||
		(ios->timing == MMC_TIMING_MMC_DDR52) ||
		(ios->timing == MMC_TIMING_SD_HS) ||
		(ios->timing == MMC_TIMING_UHS_SDR50) ||
		(ios->timing == MMC_TIMING_UHS_SDR104) ||
		(ios->timing == MMC_TIMING_UHS_DDR50) ||
		(ios->timing == MMC_TIMING_UHS_SDR25)) {
		ctrl |= SDHCI_CTRL_HISPD;
		bm_mmc_set_uhs_signaling(host, ios->timing);
		host->timing = ios->timing;
	}
	else
		ctrl &= ~SDHCI_CTRL_HISPD;


	mmio_write_8(base + SDHCI_HOST_CONTROL, ctrl);

	return 0;
}

static int bm_mmc_start_signal_voltage_switch(struct mmc_host *host, struct mmc_ios *ios)
{
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	uint16_t ctrl;

	ctrl = mmio_read_16(base + SDHCI_HOST_CONTROL2);

	switch (ios->signal_voltage) {
	case MMC_SIGNAL_VOLTAGE_330:
		/* Set 1.8V Signal Enable in the Host Control2 register to 0 */
		ctrl &= ~SDHCI_CTRL_VDD_180;
		mmio_write_16(base + SDHCI_HOST_CONTROL2, ctrl);

		/* Wait for 5ms */
#ifndef PLATFORM_PALLADIUM
		mdelay(5);
#endif

		/* 3.3V regulator output should be stable within 5 ms */
		ctrl = mmio_read_16(base + SDHCI_HOST_CONTROL2);
		if (!(ctrl & SDHCI_CTRL_VDD_180))
			return 0;

		printf("%s: 3.3V regulator output did not became stable\n", __func__);

		return -EAGAIN;
	case MMC_SIGNAL_VOLTAGE_180:
		/*
		 * Enable 1.8V Signal Enable in the Host Control2
		 * register
		 */
		ctrl |= SDHCI_CTRL_VDD_180;
		mmio_write_16(base + SDHCI_HOST_CONTROL2, ctrl);

		/* 1.8V regulator output should be stable within 5 ms */
		ctrl = mmio_read_16(base + SDHCI_HOST_CONTROL2);
		if (ctrl & SDHCI_CTRL_VDD_180)
			return 0;

		printf("%s: 1.8V regulator output did not became stable\n", __func__);

		return -EAGAIN;
	case MMC_SIGNAL_VOLTAGE_120:
		printf("%s: 1.2v switch?\n", __func__);
		return -1;
	default:
		/* No signal voltage switch required */
		return 0;
	}
}

static int bm_mmc_card_busy(struct mmc_host *host)
{
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	uint32_t present_state;

    present_state = mmio_read_32(base + SDHCI_PRESENT_STATE);

	return !(present_state & SDHCI_DATA_0_LVL_MASK);
}

static int bm_mmc_read(int lba, uintptr_t buf, size_t size)
{
    return 0;
}

static int bm_mmc_write(int lba, uintptr_t buf, size_t size)
{
    return 0;
}

int bm_sd_card_detect(struct mmc_host *host)
{
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	uint32_t reg;

	reg = mmio_read_32(base + SDHCI_PRESENT_STATE);

	if (reg & SDHCI_CARD_INSERTED)
		return 1;
	else
		return 0;
}

int bm_sd_card_lock(struct mmc_host *host)
{
	struct bm_mmc_params *param = host->priv;
	uintptr_t base = param->reg_base;
	uint32_t reg;

	reg = mmio_read_32(base + SDHCI_PRESENT_STATE);

	if (reg & SDHCI_WR_PROTECT_SW_LVL)
		return 1;
	else
		return 0;
}

static uint8_t g_adma_table[SDHCI_ADMA2_64_DESC_SZ * SDHCI_ADMA2_MAX_DESC_COUNT] __attribute__((aligned (SDHCI_ADMA_ALIGN)));
static void bm_sdhci_adma2_init(struct mmc_host *host)
{
	host->desc_sz = SDHCI_ADMA2_64_DESC_SZ;
	host->adma_table_sz = host->desc_sz * SDHCI_ADMA2_MAX_DESC_COUNT;
	//host->adma_table = malloc(host->adma_table_sz);
	host->adma_table = g_adma_table;
	host->adma_buf_size = 65536;

	if (((uint64_t)host->adma_table) & SDHCI_ADMA_MASK)
		ASSERT(0);

}

static uint8_t g_adma_intgr_tbl[SDHCI_ADMA2_64_DESC_SZ] __attribute__((aligned (SDHCI_ADMA_ALIGN)));
static void bm_sdhci_adma3_init(struct mmc_host *host)
{
	host->intgr_table = g_adma_intgr_tbl;
	bm_sdhci_adma2_init(host);
	bm_sdhci_adma_write_desc(host, host->intgr_table, (uint64_t)host->adma_table, 0, ADMA3_INTEGRATED_END_VALID);

	flush_dcache_range((unsigned long)host->intgr_table, (unsigned long)host->intgr_table + SDHCI_ADMA2_64_DESC_SZ);

	if (((uint64_t)host->adma_table) & SDHCI_ADMA_MASK)
		ASSERT(0);
}

int bm_mmc_setup_host(struct bm_mmc_params *param, struct mmc_host *host)
{
	uint32_t pio_irqs = SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL;
	uint32_t dma_irqs = SDHCI_INT_DMA_END | SDHCI_INT_ADMA_ERROR;

	host->priv = param;
	host->ocr_avail = MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;
	host->clock = (uint32_t)0xffffffff;
	host->timing = 0xff;
	host->card = NULL;

	host->ios.clock = param->f_min;
	/* Inititate to 1 line */
	host->ios.bus_width = MMC_BUS_WIDTH_1;

	host->ops = &common_ops;
	host->flags = SDHCI_USE_VERSION4;
	if (param->flags & MMC_FLAG_PIO_TRANS) {
		host->ops = &common_ops;
	} else if (param->flags & MMC_FLAG_SDMA_TRANS) {
		host->flags |= SDHCI_USE_SDMA | SDHCI_REQ_USE_DMA;
		host->ops = &common_ops;
	} else if (param->flags & MMC_FLAG_ADMA2_TRANS) {
		host->flags |= SDHCI_USE_ADMA2 | SDHCI_REQ_USE_DMA;
		host->ops = &common_ops;
		bm_sdhci_adma2_init(host);
	} else if (param->flags & MMC_FLAG_ADMA3_TRANS) {
		host->flags |= SDHCI_USE_ADMA3| SDHCI_USE_VERSION4 | SDHCI_REQ_USE_DMA;
		host->ops = &common_ops;
		bm_sdhci_adma3_init(host);
	}

	host->bus_mode = 0;
	host->force_1_8v = 0;
	host->force_bus_width_1 = 0;
	host->force_bus_clock = param->force_trans_freq;

	if (param->flags & MMC_FLAG_SDCARD) {
		host->flags |= SDHCI_TYPE_SDCARD;

		if (param->flags & MMC_FLAG_SD_DFLTSPEED) {
			host->bus_mode |= SD_MODE_DFLT_SPEED;
		}
		if (param->flags & MMC_FLAG_SD_HIGHSPEED) {
			host->bus_mode |= SD_MODE_HIGH_SPEED;
		}
		if (param->flags & MMC_FLAG_SD_SDR12) {
			host->bus_mode |= SD_MODE_UHS_SDR12;
			host->force_1_8v = 1;
		}
		if (param->flags & MMC_FLAG_SD_SDR25) {
			host->bus_mode |= SD_MODE_UHS_SDR25;
			host->force_1_8v = 1;
		}
		if (param->flags & MMC_FLAG_SD_SDR50) {
			host->bus_mode |= SD_MODE_UHS_SDR25;
			host->force_1_8v = 1;
		}
		if (param->flags & MMC_FLAG_SD_SDR104) {
			host->bus_mode |= SD_MODE_UHS_SDR104;
			host->force_1_8v = 1;
		}
		if (param->flags & MMC_FLAG_SD_DDR50) {
			host->bus_mode |= SD_MODE_UHS_DDR50;
			host->force_1_8v = 1;
		}

		if (!host->bus_mode) {
			host->bus_mode = SD_MODE_MASK;
		}
	}

	if (param->flags & MMC_FLAG_EMMC) {
		host->flags |= SDHCI_TYPE_EMMC;

		if (param->flags & MMC_FLAG_EMMC_HS200) {
			host->bus_mode |= EXT_CSD_CARD_TYPE_HS200_1_8V;
			host->force_1_8v = 1;
		}
		if (param->flags & MMC_FLAG_EMMC_DDR52) {
			host->bus_mode |= EXT_CSD_CARD_TYPE_DDR_52 | EXT_CSD_CARD_TYPE_HS_52;
			host->force_1_8v = 1;
		}
		if (param->flags & MMC_FLAG_EMMC_SDR52) {
			host->bus_mode |= EXT_CSD_CARD_TYPE_HS_52;
			host->force_1_8v = 1;
		}
		if (param->flags & MMC_FLAG_EMMC_SDR26) {
			host->bus_mode |= EXT_CSD_CARD_TYPE_HS_26;
			host->force_1_8v = 1;
		}

		if (!host->bus_mode) {
			host->bus_mode = EXT_CSD_CARD_TYPE_HS_26 | EXT_CSD_CARD_TYPE_HS_52
					| EXT_CSD_CARD_TYPE_DDR_52 | EXT_CSD_CARD_TYPE_HS200_1_8V;
		}
	}

	if (param->flags & MMC_FLAG_BUS_WIDTH_1) {
		host->force_bus_width_1 = 1;
	}

	if (param->flags & MMC_FLAG_SUPPORT_IRQ) {
		host->flags |= SDHCI_SDIO_IRQ_ENABLED;
		host->ier = SDHCI_INT_BUS_POWER | SDHCI_INT_DATA_END_BIT |
			SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_TIMEOUT |
			SDHCI_INT_INDEX | SDHCI_INT_END_BIT | SDHCI_INT_CRC |
			SDHCI_INT_TIMEOUT | SDHCI_INT_DATA_END | SDHCI_INT_RESPONSE;

		if (host->flags & SDHCI_REQ_USE_DMA)
			host->ier = (host->ier & ~pio_irqs) | dma_irqs;
		else
			host->ier = (host->ier & ~dma_irqs) | pio_irqs;

	} else {
		host->ier = 0;
	}
	return 0;
}

