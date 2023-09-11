/*
 * Copyright (c) 2016-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Defines a simple and generic interface to access eMMC device.
 */

#include <errno.h>
#include <string.h>
#include "debug.h"
#include "cache.h"
#include "mmc.h"
#include "bm_sdhci.h"
#include "sizes.h"

static int is_cmd23_enabled(unsigned int flags)
{
	return flags & SDHCI_USE_CMD23;
}

static int sd_device_state(struct mmc_card *card)
{
	mmc_cmd_t cmd;
	int ret;

	do {
		zeromem(&cmd, sizeof(mmc_cmd_t));
		MMC_PACK_CMD(cmd, MMC_CMD13, card->rca << RCA_SHIFT_OFFSET, MMC_RSP_R1);
		ret = card->host->ops->send_cmd(card->host, &cmd);
		ASSERT(ret == 0);
		ASSERT((cmd.resp_data[0] & STATUS_SWITCH_ERROR) == 0);
		/* Ignore improbable errors in release builds */
		(void)ret;
	} while ((cmd.resp_data[0] & STATUS_READY_FOR_DATA) == 0);
	return SD_GET_STATE(cmd.resp_data[0]);
}

static int mmc_set_ios(struct mmc_host *host)
{
	int err;

	err = host->ops->set_ios(host, &host->ios);

	return err;
}

void mmc_go_idle(struct mmc_host *host)
{
	mmc_cmd_t cmd;
	mmc_ops_t *ops = host->ops;

	/* CMD0: reset to IDLE */
	MMC_PACK_CMD(cmd, MMC_CMD0, 0, 0);
	ops->send_cmd(host, &cmd);
}

int mmc_send_if_cond(struct mmc_host *host, uint32_t ocr)
{
	mmc_cmd_t cmd;
	mmc_ops_t *ops = host->ops;
	int ret;
	static const uint8_t test_pattern = 0xAA;
	uint8_t result_pattern;

	/* CMD8: VHS 0x1 2.7~3.6, CHECK PATTERN 0xAA */
	MMC_PACK_CMD(cmd, MMC_CMD8, ((ocr & 0xFF8000) != 0) << 8 | test_pattern, MMC_RSP_R7);
	ret = ops->send_cmd(host, &cmd);

	if (ret)
		return ret;

	result_pattern = cmd.resp_data[0] & 0xFF;

	if (result_pattern != test_pattern)
		return -1;

	return 0;
}

int mmc_send_app_op_cond(struct mmc_host *host, uint32_t ocr, uint32_t *rocr)
{
	mmc_cmd_t cmd;
	mmc_ops_t *ops = host->ops;
	int err;


	while (1) {
		MMC_PACK_CMD(cmd, MMC_CMD55, 0x0, MMC_RSP_R1);
		err = ops->send_cmd(host, &cmd);
		if (err)
			return err;

		MMC_PACK_CMD(cmd, SD_ACMD41, ocr, MMC_RSP_R3);
		err = ops->send_cmd(host, &cmd);
		if (err)
			return err;

		if (ocr == 0)
			break;

		if ((cmd.resp_data[0] & MMC_SD_CARD_BUSY) == MMC_SD_CARD_BUSY)
			break;

#ifndef PLATFORM_PALLADIUM
		mdelay(10);
#endif
	}

	*rocr = cmd.resp_data[0];

	return 0;
}

int mmc_select_voltage(struct mmc_host *host, uint32_t ocr)
{
	int bit = 0;

	if (ocr & 0x7f) {
		printf("%s support voltages below defined range\n", __func__);
		ocr &= ~0x7f;
	}

	ocr &= host->ocr_avail;
	if (!ocr) {
		printf("%s no support for card's volts\n", __func__);
		return 0;
	}

	bit = fls(ocr) - 1;
	ocr &= 3 << bit;

	return ocr;
}

int __mmc_set_signal_voltage(struct mmc_host *host, int signal_voltage)
{
	int err = 0;
	int old_signal_voltage = host->ios.signal_voltage;

	if (host->ops->start_signal_voltage_switch) {
		host->ios.signal_voltage = signal_voltage;
		err = host->ops->start_signal_voltage_switch(host, &host->ios);

		if (err) {
			printf("%s err\n", __func__);
			host->ios.signal_voltage = old_signal_voltage;
		}
	}

	return err;
}

#define R1_ERROR	(1 << 19)
int mmc_set_signal_voltage(struct mmc_host *host, int signal_voltage, uint32_t ocr)
{
	mmc_cmd_t cmd;
	mmc_ops_t *ops = host->ops;
	int err = 0;
	uint32_t clock;

	MMC_PACK_CMD(cmd, MMC_CMD11, 0, MMC_RSP_R1);
	err = ops->send_cmd(host, &cmd);
	if (err)
		return err;

	if(cmd.resp_data[0] & R1_ERROR)
		return -1;

	mdelay(1);
#ifndef PLATFORM_PALLADIUM
	if (!ops->card_busy(host)) {
		printf("%s card_busy dat0 low\n", __func__);
		return -1;
	}
#endif

	// disabled clk
	clock = host->ios.clock;
	host->ios.clock = 0;
	mmc_set_ios(host);

	__mmc_set_signal_voltage(host, signal_voltage);

#ifndef PLATFORM_PALLADIUM
	mdelay(10);
#endif
	// enabled clk
	host->ios.clock = clock;
	mmc_set_ios(host);

	mdelay(1);

#ifndef PLATFORM_PALLADIUM
	if (ops->card_busy(host)) {
		printf("%s card_busy dat0 high\n", __func__);
		err = -1;
	}
#endif

	return err;
}

int mmc_all_send_cid(struct mmc_host *host, uint32_t *cid)
{
	mmc_cmd_t cmd;
	int err;

	MMC_PACK_CMD(cmd, MMC_CMD2, 0, MMC_RSP_R2);
	err = host->ops->send_cmd(host, &cmd);

	memcpy(cid, &cmd.resp_data, sizeof(cmd.resp_data));

	return err;
}

#define SD_OCR_XPC      (1 << 28)    /* SDXC power control */
#define SD_OCR_CCS      (1 << 30)    /* Card Capacity Status */

int mmc_sd_get_cid(struct mmc_host *host, uint32_t ocr, uint32_t *cid, uint32_t *rocr)
{
	int err;

	mmc_go_idle(host);

	err = mmc_send_if_cond(host, ocr);
	if (!err)
		ocr |= SD_OCR_CCS;

	if (host->bus_mode & SD_MODE_UHS_MASK)
		ocr |= SD_OCR_S18R;
	else
		ocr &= ~SD_OCR_S18R;

	err = mmc_send_app_op_cond(host, ocr, rocr);
	if (err)
		return err;

	if ((*rocr & 0x41000000) == 0x41000000) {
		err = mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_180, ocr);
		if (err)
			return err;
	} else if (host->force_1_8v) {
		printf("Card can't response to 1.8v!!!\n");
#ifndef PLATFORM_PALLADIUM
		return -1;
#endif
	}

	err = mmc_all_send_cid(host, cid);

	return err;
}

int mmc_send_relative_addr(struct mmc_host *host, unsigned int *rca)
{
	mmc_cmd_t cmd;
	int err;

	/* CMD3: Set Relative Address */
	MMC_PACK_CMD(cmd, MMC_CMD3, 0, MMC_RSP_R6);
	err = host->ops->send_cmd(host, &cmd);

	if (err)
		return err;

	*rca = (cmd.resp_data[0] >> 16) & 0x0000ffff;

	return 0;
}

int mmc_select_card(struct mmc_card *card)
{
	mmc_cmd_t cmd;
	int err, state;

	/* CMD7: Select Card */
	MMC_PACK_CMD(cmd, MMC_CMD7, card->rca << RCA_SHIFT_OFFSET, MMC_RSP_R1);
	err = card->host->ops->send_cmd(card->host, &cmd);
	if (err)
		return err;

	/* wait to TRAN state */
	do {
		state = sd_device_state(card);
	} while (state != SD_STATE_TRAN);

	return 0;
}

int mmc_app_set_bus_width(struct mmc_card *card, int width)
{
	mmc_cmd_t cmd;
	int err;
	uint32_t arg = 0;
	mmc_ops_t *ops = card->host->ops;

	switch (width) {
	case MMC_BUS_WIDTH_1:
		arg = 0;
		break;
	case MMC_BUS_WIDTH_4:
		arg = 2;
		break;
	default:
		return -1;
	}

	MMC_PACK_CMD(cmd, MMC_CMD55, card->rca << RCA_SHIFT_OFFSET, MMC_RSP_R1);
    err = ops->send_cmd(card->host, &cmd);
	if (err)
		return err;

	MMC_PACK_CMD(cmd, SD_ACMD6, arg, MMC_RSP_R1);
    err = ops->send_cmd(card->host, &cmd);
	if (err)
		return err;

	return 0;
}

int mmc_sd_switch(struct mmc_card *card, int mode, int group, uint8_t value, uint8_t *resp)
{
	mmc_cmd_t cmd;
	int err, state;
	uint32_t arg;
	mmc_ops_t *ops = card->host->ops;
	struct mmc_data data;

	mode = !!mode;
	value &= 0xF;

	arg = mode << 31 | 0x00FFFFFF;
	arg &= ~(0xF << (group * 4));
	arg |= value << (group *4);
	MMC_PACK_CMD_DATA(cmd, MMC_CMD6, arg, MMC_RSP_R1, &data);

	data.buf = resp;
	data.blksz = 64;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	err = ops->send_cmd(card->host, &cmd);
	if (err)
		return err;

	/* wait buffer empty */
	do {
		state = sd_device_state(card);
	} while (state != SD_STATE_TRAN);

	return 0;
}

int sd_set_current_limit(struct mmc_card *card, uint8_t *status)
{
	int current_limit = SD_SET_CURRENT_NO_CHANGE;
	int err;

	if ((card->sd_bus_speed != UHS_SDR50_BUS_SPEED) &&
	    (card->sd_bus_speed != UHS_SDR104_BUS_SPEED) &&
	    (card->sd_bus_speed != UHS_DDR50_BUS_SPEED))
		return 0;

	if (card->sw_caps.sd3_curr_limit & SD_MAX_CURRENT_800)
		current_limit = SD_SET_CURRENT_LIMIT_800;
	else if (card->sw_caps.sd3_curr_limit & SD_MAX_CURRENT_600)
		current_limit = SD_SET_CURRENT_LIMIT_600;
	else if (card->sw_caps.sd3_curr_limit & SD_MAX_CURRENT_400)
		current_limit = SD_SET_CURRENT_LIMIT_400;
	else if (card->sw_caps.sd3_curr_limit & SD_MAX_CURRENT_200)
		current_limit = SD_SET_CURRENT_LIMIT_200;

	if (current_limit != SD_SET_CURRENT_NO_CHANGE) {
		err = mmc_sd_switch(card, 1, 3, current_limit, status);
		if (err)
			return err;

		if (((status[15] >> 4) & 0x0F) != current_limit)
			printf("%s: Problem setting current limit!\n", __func__);

	}

	return 0;
}

void mmc_set_timing(struct mmc_host *host, uint32_t timing)
{
	host->ios.timing = timing;
	mmc_set_ios(host);
}

void mmc_set_clock(struct mmc_host *host, uint32_t hz)
{
	host->ios.clock = hz;
	mmc_set_ios(host);
}

void mmc_set_driver_type(struct mmc_host *host, unsigned int drv_type)
{
	host->ios.drv_type = drv_type;
	mmc_set_ios(host);
}

int mmc_select_drive_strength(struct mmc_card *card, unsigned int max_dtr,
			      int card_drv_type, int *drv_type)
{
	struct mmc_host *host = card->host;
	int host_drv_type = SD_DRIVER_TYPE_B;

	*drv_type = 0;

	if (!host->ops->select_drive_strength)
		return 0;

	/*
	 * The drive strength that the hardware can support
	 * depends on the board design.  Pass the appropriate
	 * information and let the hardware specific code
	 * return what is possible given the options
	 */
	return host->ops->select_drive_strength(card, max_dtr,
						host_drv_type,
						card_drv_type,
						drv_type);
}

static int sd_select_driver_type(struct mmc_card *card, uint8_t *status)
{
	int card_drv_type, drive_strength, drv_type;
	int err;

	card->drive_strength = 0;

	card_drv_type = card->sw_caps.sd3_drv_type | SD_DRIVER_TYPE_B;

	drive_strength = mmc_select_drive_strength(card,
						   card->sd_bus_speed,
						   card_drv_type, &drv_type);

	if (drive_strength) {
		err = mmc_sd_switch(card, 1, 2, drive_strength, status);
		if (err)
			return err;
		if ((status[15] & 0xF) != drive_strength) {
			printf("%s: Problem setting drive strength!\n",	__func__);
			return 0;
		}
		card->drive_strength = drive_strength;
	}

	if (drv_type)
		mmc_set_driver_type(card->host, drv_type);

	return 0;
}

static int sd_set_bus_speed_mode(struct mmc_card *card, uint8_t *status)
{
	int err;
	unsigned int timing = 0;

	switch (card->sd_bus_speed) {
	case UHS_SDR104_BUS_SPEED:
		timing = MMC_TIMING_UHS_SDR104;
		card->sw_caps.uhs_max_dtr = UHS_SDR104_MAX_DTR;
		break;
	case UHS_DDR50_BUS_SPEED:
		timing = MMC_TIMING_UHS_DDR50;
		card->sw_caps.uhs_max_dtr = UHS_DDR50_MAX_DTR;
		break;
	case UHS_SDR50_BUS_SPEED:
		timing = MMC_TIMING_UHS_SDR50;
		card->sw_caps.uhs_max_dtr = UHS_SDR50_MAX_DTR;
		break;
	case UHS_SDR25_BUS_SPEED:
		timing = MMC_TIMING_UHS_SDR25;
		card->sw_caps.uhs_max_dtr = UHS_SDR25_MAX_DTR;
		break;
	case UHS_SDR12_BUS_SPEED:
		timing = MMC_TIMING_UHS_SDR12;
		card->sw_caps.uhs_max_dtr = UHS_SDR12_MAX_DTR;
		break;
	default:
		return 0;
	}

	err = mmc_sd_switch(card, 1, 0, card->sd_bus_speed, status);
	if (err)
		return err;

	if ((status[16] & 0xF) != card->sd_bus_speed)
		printf("%s: Problem setting bus speed mode!\n",	__func__);
	else {
		mmc_set_timing(card->host, timing);
		mmc_set_clock(card->host, card->sw_caps.uhs_max_dtr);
	}

	return 0;
}

int mmc_execute_tuning(struct mmc_card *card)
{
	struct mmc_host *host = card->host;
	uint32_t opcode;
	int err;

	if (!host->ops->execute_tuning)
		return 0;

	if (mmc_card_mmc(card))
		opcode = MMC_CMD21;
	else
		opcode = MMC_CMD19;

	err = host->ops->execute_tuning(host, opcode);

	if (err)
		return err;

	return err;
}

void mmc_set_bus_width(struct mmc_host *host, uint32_t width)
{
	host->ios.bus_width = width;
	mmc_set_ios(host);
}

static int sd_update_bus_speed_mode(struct mmc_card *card)
{
	struct mmc_host *host = card->host;
	uint32_t bus_mode;

	bus_mode = host->bus_mode & card->sw_caps.sd3_bus_mode;

	if (!bus_mode) {
		printf("Card not support the bus mode of your set.\n");
		return -1;
	}

	if (bus_mode & SD_MODE_UHS_SDR104) {
			card->sd_bus_speed = UHS_SDR104_BUS_SPEED;
	} else if (bus_mode & SD_MODE_UHS_DDR50) {
			card->sd_bus_speed = UHS_DDR50_BUS_SPEED;
	} else if (bus_mode & SD_MODE_UHS_SDR50) {
			card->sd_bus_speed = UHS_SDR50_BUS_SPEED;
	} else if (bus_mode & SD_MODE_UHS_SDR25) {
			card->sd_bus_speed = UHS_SDR25_BUS_SPEED;
	} else if (bus_mode & SD_MODE_UHS_SDR12) {
			card->sd_bus_speed = UHS_SDR12_BUS_SPEED;
	}

	return 0;
}


/*
 * UHS-I specific initialization procedure
 */
int mmc_sd_init_uhs_card(struct mmc_card *card)
{
	int err;
	uint8_t status[64]__attribute__((aligned (SD_BLOCK_SIZE)));

	if (!card->scr.sda_spec3)
		return 0;

	if (!(card->csd.cmdclass & CCC_SWITCH))
		return 0;

	/* Set bus width */
	err = mmc_app_set_bus_width(card, MMC_BUS_WIDTH_4);
	if (err)
		return err;

	mmc_set_bus_width(card->host, MMC_BUS_WIDTH_4);

	/*
	 * Select the bus speed mode depending on host
	 * and card capability.
	 */
	err = sd_update_bus_speed_mode(card);
	if (err)
		return err;

	/* Set the driver strength for the card */
	err = sd_select_driver_type(card, status);
	if (err)
		return err;

	/* Set current limit for the card */
	//err = sd_set_current_limit(card, status);
	if (err)
		return err;

	/* Set bus speed mode of the card */
	err = sd_set_bus_speed_mode(card, status);
	if (err)
		return err;

	if ((card->host->ios.timing == MMC_TIMING_UHS_SDR50 ||
		 card->host->ios.timing == MMC_TIMING_UHS_DDR50 ||
		 card->host->ios.timing == MMC_TIMING_UHS_SDR104)) {
		err = mmc_execute_tuning(card);

		/*
		 * As SD Specifications Part1 Physical Layer Specification
		 * Version 3.01 says, CMD19 tuning is available for unlocked
		 * cards in transfer state of 1.8V signaling mode. The small
		 * difference between v3.00 and 3.01 spec means that CMD19
		 * tuning is also available for DDR50 mode.
		 */
		if (err && card->host->ios.timing == MMC_TIMING_UHS_DDR50) {
			printf("%s: ddr50 tuning failed\n", __func__);
			err = 0;
		}
	}

	return err;
}

static struct mmc_card global_mmc_card;

static const unsigned int tran_exp[] = {
	10000,		100000,		1000000,	10000000,
	0,		0,		0,		0
};

static const unsigned char tran_mant[] = {
	0,	10,	12,	13,	15,	20,	25,	30,
	35,	40,	45,	50,	55,	60,	70,	80,
};

static const unsigned int tacc_exp[] = {
	1,	10,	100,	1000,	10000,	100000,	1000000, 10000000,
};

static const unsigned int tacc_mant[] = {
	0,	10,	12,	13,	15,	20,	25,	30,
	35,	40,	45,	50,	55,	60,	70,	80,
};

static const unsigned int sd_au_size[] = {
	0,		SZ_16K / 512,		SZ_32K / 512,	SZ_64K / 512,
	SZ_128K / 512,	SZ_256K / 512,		SZ_512K / 512,	SZ_1M / 512,
	SZ_2M / 512,	SZ_4M / 512,		SZ_8M / 512,	(SZ_8M + SZ_4M) / 512,
	SZ_16M / 512,	(SZ_16M + SZ_8M) / 512,	SZ_32M / 512,	SZ_64M / 512,
};

#define UNSTUFF_BITS(resp,start,size)					\
	({								\
		const int __size = size;				\
		const uint32_t __mask = (__size < 32 ? 1 << __size : 0) - 1;	\
		const int __off = 3 - ((start) / 32);			\
		const int __shft = (start) & 31;			\
		uint32_t __res;						\
									\
		__res = resp[__off] >> __shft;				\
		if (__size + __shft > 32)				\
			__res |= resp[__off-1] << ((32 - __shft) % 32);	\
		__res & __mask;						\
	})

/*
 * Given the decoded CSD structure, decode the raw CID to our CID structure.
 */
void mmc_sd_decode_cid(struct mmc_card *card)
{
	uint32_t *resp = card->raw_cid;

	/*
	 * SD doesn't currently have a version field so we will
	 * have to assume we can parse this.
	 */
	card->cid.manfid		= UNSTUFF_BITS(resp, 120, 8);
	card->cid.oemid			= UNSTUFF_BITS(resp, 104, 16);
	card->cid.prod_name[0]		= UNSTUFF_BITS(resp, 96, 8);
	card->cid.prod_name[1]		= UNSTUFF_BITS(resp, 88, 8);
	card->cid.prod_name[2]		= UNSTUFF_BITS(resp, 80, 8);
	card->cid.prod_name[3]		= UNSTUFF_BITS(resp, 72, 8);
	card->cid.prod_name[4]		= UNSTUFF_BITS(resp, 64, 8);
	card->cid.hwrev			= UNSTUFF_BITS(resp, 60, 4);
	card->cid.fwrev			= UNSTUFF_BITS(resp, 56, 4);
	card->cid.serial		= UNSTUFF_BITS(resp, 24, 32);
	card->cid.year			= UNSTUFF_BITS(resp, 12, 8);
	card->cid.month			= UNSTUFF_BITS(resp, 8, 4);

	card->cid.year += 2000; /* SD cards year offset */
}

/*
 * Given a 128-bit response, decode to our card CSD structure.
 */
static int mmc_sd_decode_csd(struct mmc_card *card)
{
	struct mmc_csd *csd = &card->csd;
	unsigned int e, m, csd_struct;
	uint32_t *resp = card->raw_csd;

	csd_struct = UNSTUFF_BITS(resp, 126, 2);

	switch (csd_struct) {
	case 0:
		m = UNSTUFF_BITS(resp, 115, 4);
		e = UNSTUFF_BITS(resp, 112, 3);
		csd->tacc_ns	 = (tacc_exp[e] * tacc_mant[m] + 9) / 10;
		csd->tacc_clks	 = UNSTUFF_BITS(resp, 104, 8) * 100;

		m = UNSTUFF_BITS(resp, 99, 4);
		e = UNSTUFF_BITS(resp, 96, 3);
		csd->max_dtr	  = tran_exp[e] * tran_mant[m];
		csd->cmdclass	  = UNSTUFF_BITS(resp, 84, 12);

		e = UNSTUFF_BITS(resp, 47, 3);
		m = UNSTUFF_BITS(resp, 62, 12);
		csd->capacity	  = (1 + m) << (e + 2);

		csd->read_blkbits = UNSTUFF_BITS(resp, 80, 4);
		csd->read_partial = UNSTUFF_BITS(resp, 79, 1);
		csd->write_misalign = UNSTUFF_BITS(resp, 78, 1);
		csd->read_misalign = UNSTUFF_BITS(resp, 77, 1);
		csd->dsr_imp = UNSTUFF_BITS(resp, 76, 1);
		csd->r2w_factor = UNSTUFF_BITS(resp, 26, 3);
		csd->write_blkbits = UNSTUFF_BITS(resp, 22, 4);
		csd->write_partial = UNSTUFF_BITS(resp, 21, 1);

		if (UNSTUFF_BITS(resp, 46, 1)) {
			csd->erase_size = 1;
		} else if (csd->write_blkbits >= 9) {
			csd->erase_size = UNSTUFF_BITS(resp, 39, 7) + 1;
			csd->erase_size <<= csd->write_blkbits - 9;
		}
		break;
	case 1:
		/*
		 * This is a block-addressed SDHC or SDXC card. Most
		 * interesting fields are unused and have fixed
		 * values. To avoid getting tripped by buggy cards,
		 * we assume those fixed values ourselves.
		 */

		csd->tacc_ns	 = 0; /* Unused */
		csd->tacc_clks	 = 0; /* Unused */

		m = UNSTUFF_BITS(resp, 99, 4);
		e = UNSTUFF_BITS(resp, 96, 3);
		csd->max_dtr	  = tran_exp[e] * tran_mant[m];
		csd->cmdclass	  = UNSTUFF_BITS(resp, 84, 12);
		csd->c_size	  = UNSTUFF_BITS(resp, 48, 22);

		m = UNSTUFF_BITS(resp, 48, 22);
		csd->capacity     = (1 + m) << 10;

		csd->read_blkbits = 9;
		csd->read_partial = 0;
		csd->write_misalign = 0;
		csd->read_misalign = 0;
		csd->r2w_factor = 4; /* Unused */
		csd->write_blkbits = 9;
		csd->write_partial = 0;
		csd->erase_size = 1;
		break;
	default:
		printf("%s: unrecognised CSD structure version %d\n",
			__func__, csd_struct);
		return -1;
	}

	card->erase_size = csd->erase_size;

	return 0;
}

/*
 * Given a 64-bit response, decode to our card SCR structure.
 */
static int mmc_decode_scr(struct mmc_card *card)
{
	struct sd_scr *scr = &card->scr;
	unsigned int scr_struct;
	uint32_t resp[4];

	resp[3] = card->raw_scr[1];
	resp[2] = card->raw_scr[0];

	scr_struct = UNSTUFF_BITS(resp, 60, 4);
	if (scr_struct != 0) {
		printf("%s: unrecognised SCR structure version %d\n",
			__func__, scr_struct);
		return -EINVAL;
	}

	scr->sda_vsn = UNSTUFF_BITS(resp, 56, 4);
	scr->bus_widths = UNSTUFF_BITS(resp, 48, 4);
	if (scr->sda_vsn == SCR_SPEC_VER_2)
		/* Check if Physical Layer Spec v3.0 is supported */
		scr->sda_spec3 = UNSTUFF_BITS(resp, 47, 1);

	if (UNSTUFF_BITS(resp, 55, 1))
		card->erased_byte = 0xFF;
	else
		card->erased_byte = 0x0;

	if (scr->sda_spec3)
		scr->cmds = UNSTUFF_BITS(resp, 32, 2);
	return 0;
}

int mmc_app_sd_status(struct mmc_card *card, uint32_t *ssr)
{
	mmc_cmd_t cmd;
	int err, state;
	mmc_ops_t *ops = card->host->ops;
	struct mmc_data data;

	MMC_PACK_CMD(cmd, MMC_CMD55, card->rca << RCA_SHIFT_OFFSET, MMC_RSP_R1);
	err = ops->send_cmd(card->host, &cmd);
	if (err)
		return err;

	data.buf = ssr;
	data.blksz = 64;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	MMC_PACK_CMD_DATA(cmd, SD_ACMD13, 0x0, MMC_RSP_R1, &data);
	err = ops->send_cmd(card->host, &cmd);
	if (err)
		return err;

	/* wait buffer empty */
	do {
		state = sd_device_state(card);
	} while (state != SD_STATE_TRAN);

	return 0;
}

#define be32_to_cpu(x)	__builtin_bswap32(x)
#define CCC_APP_SPEC        (1<<8)  /* (8) Application specific */
/*
 * Fetch and process SD Status register.
 */
static int mmc_read_ssr(struct mmc_card *card)
{
	unsigned int au, es, et, eo;
	uint32_t raw_ssr[sizeof(card->raw_ssr) / 4]__attribute__((aligned (SD_BLOCK_SIZE)));
	int i;

	if (!(card->csd.cmdclass & CCC_APP_SPEC)) {
		printf("%s: card lacks mandatory SD Status function\n",	__func__);
		return 0;
	}

	if (mmc_app_sd_status(card, raw_ssr)) {
		printf("%s: problem reading SD Status register\n", __func__);
		return 0;
	}

	for (i = 0; i < 16; i++)
		card->raw_ssr[i] = be32_to_cpu(raw_ssr[i]);

	/*
	 * UNSTUFF_BITS only works with four uint32_ts so we have to offset the
	 * bitfield positions accordingly.
	 */
	au = UNSTUFF_BITS(card->raw_ssr, 428 - 384, 4);
	if (au) {
		if (au <= 9 || card->scr.sda_spec3) {
			card->ssr.au = sd_au_size[au];
			es = UNSTUFF_BITS(card->raw_ssr, 408 - 384, 16);
			et = UNSTUFF_BITS(card->raw_ssr, 402 - 384, 6);
			if (es && et) {
				eo = UNSTUFF_BITS(card->raw_ssr, 400 - 384, 2);
				card->ssr.erase_timeout = (et * 1000) / es;
				card->ssr.erase_offset = eo * 1000;
			}
		} else {
			printf("%s: SD Status: Invalid Allocation Unit size\n", __func__);
		}
	}

	return 0;
}

int mmc_send_csd(struct mmc_card *card, uint32_t *csd)
{
	mmc_cmd_t cmd;
	int err;
	mmc_ops_t *ops = card->host->ops;

	MMC_PACK_CMD(cmd, MMC_CMD9, card->rca << 16, MMC_RSP_R2);
	err = ops->send_cmd(card->host, &cmd);
	if (err)
		return err;

	memcpy(csd, &cmd.resp_data, sizeof(uint32_t) * 4);

	return 0;
}

int mmc_sd_get_csd(struct mmc_host *host, struct mmc_card *card)
{
	int err;

	err = mmc_send_csd(card, card->raw_csd);
	if (err)
		return err;

	err = mmc_sd_decode_csd(card);
	if (err)
		return err;

	return 0;
}


int mmc_app_send_scr(struct mmc_card *card, uint32_t *scr)
{
	int state;
	mmc_cmd_t cmd;
	int err;
	uint32_t buf[sizeof(card->raw_scr)/4]__attribute__((aligned (SD_BLOCK_SIZE)));
	mmc_ops_t *ops = card->host->ops;
	struct mmc_data data;

	MMC_PACK_CMD(cmd, MMC_CMD55, card->rca << RCA_SHIFT_OFFSET, MMC_RSP_R1);
	err = ops->send_cmd(card->host, &cmd);
	if (err)
		return err;

	data.buf = buf;
	data.blksz = sizeof(buf);
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	MMC_PACK_CMD_DATA(cmd, SD_ACMD51, 0x0, MMC_RSP_R1, &data);
	err = ops->send_cmd(card->host, &cmd);

	/* wait buffer empty */
	do {
		state = sd_device_state(card);
	} while (state != SD_STATE_TRAN);

	scr[0] = be32_to_cpu(buf[0]);
	scr[1] = be32_to_cpu(buf[1]);

	pr_debug("[hq] scr[0]:0x%x, buf[0]:0x%x\n", scr[0], buf[0]);

	return 0;
}

int mmc_init_erase(struct mmc_card *card)
{
	return 0;
}

/*
 * Fetches and decodes switch information
 */
static int mmc_read_switch(struct mmc_card *card)
{
	int err;
	uint8_t status[64]__attribute__((aligned (SD_BLOCK_SIZE)));

	if (card->scr.sda_vsn < SCR_SPEC_VER_1)
		return 0;

	if (!(card->csd.cmdclass & CCC_SWITCH)) {
		printf("%s: card lacks mandatory switch function, performance might suffer\n", __func__);
		return 0;
	}

	err = -EIO;

	/*
	 * Find out the card's support bits with a mode 0 operation.
	 * The argument does not matter, as the support bits do not
	 * change with the arguments.
	 */
	err = mmc_sd_switch(card, 0, 0, 0, status);
	if (err) {
		return err;
	}

	if (status[13] & SD_MODE_HIGH_SPEED) {
		if (card->host->bus_mode & SD_MODE_HIGH_SPEED)
			card->sw_caps.hs_max_dtr = HIGH_SPEED_MAX_DTR;
		else {
			card->sw_caps.hs_max_dtr = 0;
			card->csd.max_dtr = UHS_SDR12_MAX_DTR;
		}
	}

	if (card->scr.sda_spec3) {
		card->sw_caps.sd3_bus_mode = status[13];
		/* Driver Strengths supported by the card */
		card->sw_caps.sd3_drv_type = status[9];
		card->sw_caps.sd3_curr_limit = status[7] | status[6] << 8;
	}

	return err;
}

int mmc_sd_setup_card(struct mmc_host *host, struct mmc_card *card,	int reinit)
{
	int err;

	if (!reinit) {
		/*
		 * Fetch SCR from card.
		 */
		err = mmc_app_send_scr(card, card->raw_scr);
		if (err)
			return err;

		err = mmc_decode_scr(card);
		if (err)
			return err;

		/*
		 * Fetch and process SD Status register.
		 */
		err = mmc_read_ssr(card);
		if (err)
			return err;

		/* Erase init depends on CSD and SSR */
		mmc_init_erase(card);

		/*
		 * Fetch switch information from card.
		 */
		err = mmc_read_switch(card);
		if (err)
			return err;
	}

	return 0;
}

/*
 * Test if the card supports high-speed mode and, if so, switch to it.
 */
int mmc_sd_switch_hs(struct mmc_card *card)
{
	int err;
	uint8_t status[64]__attribute__((aligned (SD_BLOCK_SIZE)));

	if (card->scr.sda_vsn < SCR_SPEC_VER_1)
		return 0;

	if (!(card->csd.cmdclass & CCC_SWITCH))
		return 0;

	if (card->sw_caps.hs_max_dtr == 0)
		return 0;

	err = mmc_sd_switch(card, 1, 0, 1, status);
	if (err)
		return err;

	if ((status[16] & 0xF) != 1) {
		printf("%s: Problem switching card into high-speed mode!\n", __func__);
		err = 0;
	} else {
		err = 1;
	}

	return err;
}

unsigned mmc_sd_get_max_clock(struct mmc_card *card)
{
	unsigned max_dtr = (unsigned int)-1;

	if (mmc_card_hs(card)) {
		if (max_dtr > card->sw_caps.hs_max_dtr)
			max_dtr = card->sw_caps.hs_max_dtr;
	} else if (max_dtr > card->csd.max_dtr) {
		max_dtr = card->csd.max_dtr;
	}

	return max_dtr;
}


static int mmc_sd_init_card(struct mmc_host *host, uint32_t ocr)
{
	struct mmc_card *card = &global_mmc_card;
	int err;
	uint32_t cid[4];
	uint32_t rocr;

	err = mmc_sd_get_cid(host, ocr, cid, &rocr);
	pr_debug("%s, rocr:0x%x\n", __func__, rocr);
	if (err)
		return err;

	zeromem(card, sizeof(struct mmc_card));
	host->card = card;
	card->host = host;
	card->ocr = rocr;
	card->type = MMC_TYPE_SD;
	memcpy(card->raw_cid, cid, sizeof(card->raw_cid));

	err = mmc_send_relative_addr(host, &card->rca);
	if (err)
		return err;

	pr_debug("%s rca:0x%x\n", __func__, card->rca);
	err = mmc_sd_get_csd(host, card);
	if (err)
		return err;
	mmc_sd_decode_cid(card);

	err = mmc_select_card(card);
	if (err)
		return err;

	err = mmc_sd_setup_card(host, card, 0);
	if (err)
		return err;

	if (rocr & SD_ROCR_S18A) {
		if (host->force_bus_width_1) {
			printf("%s UHS not support bus width to 1 line!\n", __func__);
			return -1;
		}
		err = mmc_sd_init_uhs_card(card);
		if (err)
			return err;
	} else {
		pr_debug("%s 3.3v speed\n", __func__);

		/*
		 * Attempt to change to high-speed (if supported)
		 */
		err = mmc_sd_switch_hs(card);
		if (err > 0)
			mmc_set_timing(card->host, MMC_TIMING_SD_HS);
		else
			mmc_set_timing(card->host, MMC_TIMING_LEGACY);

		/*
		 * Set bus speed.
		 */
		if (host->force_bus_clock)
			mmc_set_clock(host, host->force_bus_clock);
		else
			mmc_set_clock(host, mmc_sd_get_max_clock(card));

		/*
		 * Switch to wider bus (if supported).
		 */
		if (card->scr.bus_widths & SD_SCR_BUS_WIDTH_4 && !host->force_bus_width_1) {
			err = mmc_app_set_bus_width(card, MMC_BUS_WIDTH_4);
			if (err)
				return -1;

			mmc_set_bus_width(host, MMC_BUS_WIDTH_4);
		} else if (host->force_bus_width_1) {
			printf("Card not support bus width 4!!!\n");
			return -1;
		}
	}

	return 0;
}

int mmc_attach_sd(struct mmc_host *host)
{
	mmc_ops_t *ops = host->ops;
	int err;
	uint32_t ocr, rocr;

	ASSERT(host);
	ASSERT(ops);

	err = mmc_send_if_cond(host, host->ocr_avail);
	if (err)
		return err;

	err = mmc_send_app_op_cond(host, 0, &ocr);
	if (err)
		return err;

	pr_debug("%s, ocr:0x%x\n", __func__, ocr);

	rocr = mmc_select_voltage(host, ocr);
	pr_debug("%s, rocr:0x%x\n", __func__, rocr);
	if (!rocr)
		return -1;

	err = mmc_sd_init_card(host, rocr);

	return err;
}

int mmc_send_op_cond(struct mmc_host *host, u32 ocr, u32 *rocr)
{
	int i, err = 0;
	mmc_cmd_t cmd;
	mmc_ops_t *ops = host->ops;

	MMC_PACK_CMD(cmd, MMC_CMD1, ocr, MMC_RSP_R3);

	for (i = 100; i; i--) {
		err = ops->send_cmd(host, &cmd);
		if (err)
			return err;

		if (ocr == 0)
			break;

		if (cmd.resp_data[0] & MMC_CARD_BUSY)
			break;

#ifndef PLATFORM_PALLADIUM
		mdelay(10);
#endif
	}

	if (rocr)
		*rocr = cmd.resp_data[0];

	return 0;
}

int mmc_set_relative_addr(struct mmc_card *card)
{
	int err;
	mmc_cmd_t cmd;
	mmc_ops_t *ops = card->host->ops;

	MMC_PACK_CMD(cmd, MMC_CMD3, card->rca << 16, MMC_RSP_R1);
	err = ops->send_cmd(card->host, &cmd);

	return err;
}

/*
 * Given a 128-bit response, decode to our card CSD structure.
 */
static int mmc_decode_csd(struct mmc_card *card)
{
	struct mmc_csd *csd = &card->csd;
	unsigned int e, m, a, b;
	uint32_t *resp = card->raw_csd;

	/*
	 * We only understand CSD structure v1.1 and v1.2.
	 * v1.2 has extra information in bits 15, 11 and 10.
	 * We also support eMMC v4.4 & v4.41.
	 */
	csd->structure = UNSTUFF_BITS(resp, 126, 2);
	if (csd->structure == 0) {
		printf("%s: unrecognised CSD structure version %d\n",
			__func__, csd->structure);
		return -EINVAL;
	}

	csd->mmca_vsn	 = UNSTUFF_BITS(resp, 122, 4);
	m = UNSTUFF_BITS(resp, 115, 4);
	e = UNSTUFF_BITS(resp, 112, 3);
	csd->tacc_ns	 = (tacc_exp[e] * tacc_mant[m] + 9) / 10;
	csd->tacc_clks	 = UNSTUFF_BITS(resp, 104, 8) * 100;

	m = UNSTUFF_BITS(resp, 99, 4);
	e = UNSTUFF_BITS(resp, 96, 3);
	csd->max_dtr	  = tran_exp[e] * tran_mant[m];
	csd->cmdclass	  = UNSTUFF_BITS(resp, 84, 12);

	e = UNSTUFF_BITS(resp, 47, 3);
	m = UNSTUFF_BITS(resp, 62, 12);
	csd->capacity	  = (1 + m) << (e + 2);

	csd->read_blkbits = UNSTUFF_BITS(resp, 80, 4);
	csd->read_partial = UNSTUFF_BITS(resp, 79, 1);
	csd->write_misalign = UNSTUFF_BITS(resp, 78, 1);
	csd->read_misalign = UNSTUFF_BITS(resp, 77, 1);
	csd->dsr_imp = UNSTUFF_BITS(resp, 76, 1);
	csd->r2w_factor = UNSTUFF_BITS(resp, 26, 3);
	csd->write_blkbits = UNSTUFF_BITS(resp, 22, 4);
	csd->write_partial = UNSTUFF_BITS(resp, 21, 1);

	if (csd->write_blkbits >= 9) {
		a = UNSTUFF_BITS(resp, 42, 5);
		b = UNSTUFF_BITS(resp, 37, 5);
		csd->erase_size = (a + 1) * (b + 1);
		csd->erase_size <<= csd->write_blkbits - 9;
	}

	return 0;
}

/*
 * Given the decoded CSD structure, decode the raw CID to our CID structure.
 */
static int mmc_decode_cid(struct mmc_card *card)
{
	uint32_t *resp = card->raw_cid;

	/*
	 * The selection of the format here is based upon published
	 * specs from sandisk and from what people have reported.
	 */
	switch (card->csd.mmca_vsn) {
	case 0: /* MMC v1.0 - v1.2 */
	case 1: /* MMC v1.4 */
		card->cid.manfid	= UNSTUFF_BITS(resp, 104, 24);
		card->cid.prod_name[0]	= UNSTUFF_BITS(resp, 96, 8);
		card->cid.prod_name[1]	= UNSTUFF_BITS(resp, 88, 8);
		card->cid.prod_name[2]	= UNSTUFF_BITS(resp, 80, 8);
		card->cid.prod_name[3]	= UNSTUFF_BITS(resp, 72, 8);
		card->cid.prod_name[4]	= UNSTUFF_BITS(resp, 64, 8);
		card->cid.prod_name[5]	= UNSTUFF_BITS(resp, 56, 8);
		card->cid.prod_name[6]	= UNSTUFF_BITS(resp, 48, 8);
		card->cid.hwrev		= UNSTUFF_BITS(resp, 44, 4);
		card->cid.fwrev		= UNSTUFF_BITS(resp, 40, 4);
		card->cid.serial	= UNSTUFF_BITS(resp, 16, 24);
		card->cid.month		= UNSTUFF_BITS(resp, 12, 4);
		card->cid.year		= UNSTUFF_BITS(resp, 8, 4) + 1997;
		break;

	case 2: /* MMC v2.0 - v2.2 */
	case 3: /* MMC v3.1 - v3.3 */
	case 4: /* MMC v4 */
		card->cid.manfid	= UNSTUFF_BITS(resp, 120, 8);
		card->cid.oemid		= UNSTUFF_BITS(resp, 104, 16);
		card->cid.prod_name[0]	= UNSTUFF_BITS(resp, 96, 8);
		card->cid.prod_name[1]	= UNSTUFF_BITS(resp, 88, 8);
		card->cid.prod_name[2]	= UNSTUFF_BITS(resp, 80, 8);
		card->cid.prod_name[3]	= UNSTUFF_BITS(resp, 72, 8);
		card->cid.prod_name[4]	= UNSTUFF_BITS(resp, 64, 8);
		card->cid.prod_name[5]	= UNSTUFF_BITS(resp, 56, 8);
		card->cid.prv		= UNSTUFF_BITS(resp, 48, 8);
		card->cid.serial	= UNSTUFF_BITS(resp, 16, 32);
		card->cid.month		= UNSTUFF_BITS(resp, 12, 4);
		card->cid.year		= UNSTUFF_BITS(resp, 8, 4) + 1997;
		break;

	default:
		printf("%s: card has unknown MMCA version %d\n",
			__func__, card->csd.mmca_vsn);
		return -EINVAL;
	}

	return 0;
}

int mmc_can_ext_csd(struct mmc_card *card)
{
	return (card && card->csd.mmca_vsn > CSD_SPEC_VER_3);
}

static int
mmc_send_cxd_data(struct mmc_card *card, struct mmc_host *host,
		uint32_t opcode, void *buf, unsigned len)
{
	int state;
	mmc_cmd_t cmd;
	int err;
	mmc_ops_t *ops = card->host->ops;
	struct mmc_data data;

	data.buf = buf;
	data.blksz = len;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	MMC_PACK_CMD_DATA(cmd, opcode, 0x0, MMC_RSP_R1, &data);
	err = ops->send_cmd(card->host, &cmd);
	if (err)
		return err;

	/* wait buffer empty */
	do {
		state = sd_device_state(card);
	} while (state != SD_STATE_TRAN);

	return 0;
}

int mmc_get_ext_csd(struct mmc_card *card, uint8_t *ext_csd)
{
	int err;

	if (!card || !ext_csd)
		return -EINVAL;

	if (!mmc_can_ext_csd(card))
		return -EINVAL;

	err = mmc_send_cxd_data(card, card->host, MMC_CMD8, ext_csd,
				512);

	return err;
}

static void mmc_select_card_type(struct mmc_card *card)
{

	unsigned int hs_max_dtr = 0, hs200_max_dtr = 0;
	unsigned int avail_type = 0;
	unsigned int bus_mode;
#ifdef PLATFORM_PALLADIUM
	bus_mode = card->host->bus_mode;
#else
	uint8_t card_type = card->ext_csd.raw_card_type;
	bus_mode = card->host->bus_mode & card_type;
	if (!bus_mode) {
		printf("card can't support the bus mode you indict! host_bus_mode:%d card_type:%d\n",
			card->host->bus_mode,card_type);
		return;
	}
#endif

	if (bus_mode & EXT_CSD_CARD_TYPE_HS_26) {
		hs_max_dtr = MMC_HIGH_26_MAX_DTR;
		avail_type |= EXT_CSD_CARD_TYPE_HS_26;
	}

	if (bus_mode & EXT_CSD_CARD_TYPE_HS_52) {
		hs_max_dtr = MMC_HIGH_52_MAX_DTR;
		avail_type |= EXT_CSD_CARD_TYPE_HS_52;
	}

	if (bus_mode & EXT_CSD_CARD_TYPE_DDR_1_8V) {
		hs_max_dtr = MMC_HIGH_DDR_MAX_DTR;
		avail_type |= EXT_CSD_CARD_TYPE_DDR_1_8V;
	}

	if (bus_mode & EXT_CSD_CARD_TYPE_DDR_1_2V) {
		hs_max_dtr = MMC_HIGH_DDR_MAX_DTR;
		avail_type |= EXT_CSD_CARD_TYPE_DDR_1_2V;
	}

	if (bus_mode & EXT_CSD_CARD_TYPE_HS200_1_8V) {
		hs200_max_dtr = MMC_HS200_MAX_DTR;
		avail_type |= EXT_CSD_CARD_TYPE_HS200_1_8V;
	}

	if (bus_mode & EXT_CSD_CARD_TYPE_HS200_1_2V) {
		hs200_max_dtr = MMC_HS200_MAX_DTR;
		avail_type |= EXT_CSD_CARD_TYPE_HS200_1_2V;
	}

#if 0
	if (card_type & EXT_CSD_CARD_TYPE_HS400_1_8V) {
		hs200_max_dtr = MMC_HS200_MAX_DTR;
		avail_type |= EXT_CSD_CARD_TYPE_HS400_1_8V;
	}

	if (card_type & EXT_CSD_CARD_TYPE_HS400_1_2V) {
		hs200_max_dtr = MMC_HS200_MAX_DTR;
		avail_type |= EXT_CSD_CARD_TYPE_HS400_1_2V;
	}
#endif

	card->ext_csd.hs_max_dtr = hs_max_dtr;
	card->ext_csd.hs200_max_dtr = hs200_max_dtr;
	card->mmc_avail_type = avail_type;
}

#define MMC_MIN_PART_SWITCH_TIME    300
#define DEFAULT_CMD6_TIMEOUT_MS		500
/*
 * Decode extended CSD.
 */
static int mmc_decode_ext_csd(struct mmc_card *card, uint8_t *ext_csd)
{
	int err = 0;

	/* Version is coded in the CSD_STRUCTURE byte in the EXT_CSD register */
	card->ext_csd.raw_ext_csd_structure = ext_csd[EXT_CSD_STRUCTURE];
	if (card->csd.structure == 3) {
		if (card->ext_csd.raw_ext_csd_structure > 2) {
			printf("%s: unrecognised EXT_CSD structure "
				"version %d\n", __func__,
					card->ext_csd.raw_ext_csd_structure);
			err = -EINVAL;
			return err;
		}
	}

	card->ext_csd.rev = ext_csd[EXT_CSD_REV];

	card->ext_csd.raw_sectors[0] = ext_csd[EXT_CSD_SEC_CNT + 0];
	card->ext_csd.raw_sectors[1] = ext_csd[EXT_CSD_SEC_CNT + 1];
	card->ext_csd.raw_sectors[2] = ext_csd[EXT_CSD_SEC_CNT + 2];
	card->ext_csd.raw_sectors[3] = ext_csd[EXT_CSD_SEC_CNT + 3];
	if (card->ext_csd.rev >= 2) {
		card->ext_csd.sectors =
			ext_csd[EXT_CSD_SEC_CNT + 0] << 0 |
			ext_csd[EXT_CSD_SEC_CNT + 1] << 8 |
			ext_csd[EXT_CSD_SEC_CNT + 2] << 16 |
			ext_csd[EXT_CSD_SEC_CNT + 3] << 24;
	}

	card->ext_csd.strobe_support = ext_csd[EXT_CSD_STROBE_SUPPORT];
	card->ext_csd.raw_card_type = ext_csd[EXT_CSD_CARD_TYPE];
	mmc_select_card_type(card);

	card->ext_csd.raw_s_a_timeout = ext_csd[EXT_CSD_S_A_TIMEOUT];
	card->ext_csd.raw_erase_timeout_mult =
		ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT];
	card->ext_csd.raw_hc_erase_grp_size =
		ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE];
	if (card->ext_csd.rev >= 3) {
		uint8_t sa_shift = ext_csd[EXT_CSD_S_A_TIMEOUT];
		card->ext_csd.part_config = ext_csd[EXT_CSD_PART_CONFIG];

		/* EXT_CSD value is in units of 10ms, but we store in ms */
		card->ext_csd.part_time = 10 * ext_csd[EXT_CSD_PART_SWITCH_TIME];
		/* Some eMMC set the value too low so set a minimum */
		if (card->ext_csd.part_time &&
		    card->ext_csd.part_time < MMC_MIN_PART_SWITCH_TIME)
			card->ext_csd.part_time = MMC_MIN_PART_SWITCH_TIME;

		/* Sleep / awake timeout in 100ns units */
		if (sa_shift > 0 && sa_shift <= 0x17)
			card->ext_csd.sa_timeout =
					1 << ext_csd[EXT_CSD_S_A_TIMEOUT];
		card->ext_csd.erase_group_def =
			ext_csd[EXT_CSD_ERASE_GROUP_DEF];
		card->ext_csd.hc_erase_timeout = 300 *
			ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT];
		card->ext_csd.hc_erase_size =
			ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] << 10;

		card->ext_csd.rel_sectors = ext_csd[EXT_CSD_REL_WR_SEC_C];

	}

	card->ext_csd.raw_hc_erase_gap_size =
		ext_csd[EXT_CSD_HC_WP_GRP_SIZE];
	card->ext_csd.raw_sec_trim_mult =
		ext_csd[EXT_CSD_SEC_TRIM_MULT];
	card->ext_csd.raw_sec_erase_mult =
		ext_csd[EXT_CSD_SEC_ERASE_MULT];
	card->ext_csd.raw_sec_feature_support =
		ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT];
	card->ext_csd.raw_trim_mult =
		ext_csd[EXT_CSD_TRIM_MULT];
	card->ext_csd.raw_partition_support = ext_csd[EXT_CSD_PARTITION_SUPPORT];
	card->ext_csd.raw_driver_strength = ext_csd[EXT_CSD_DRIVER_STRENGTH];
	if (card->ext_csd.rev >= 4) {
		if (ext_csd[EXT_CSD_PARTITION_SETTING_COMPLETED] &
		    EXT_CSD_PART_SETTING_COMPLETED)
			card->ext_csd.partition_setting_completed = 1;
		else
			card->ext_csd.partition_setting_completed = 0;

//		mmc_manage_enhanced_area(card, ext_csd);

//		mmc_manage_gp_partitions(card, ext_csd);

		card->ext_csd.sec_trim_mult =
			ext_csd[EXT_CSD_SEC_TRIM_MULT];
		card->ext_csd.sec_erase_mult =
			ext_csd[EXT_CSD_SEC_ERASE_MULT];
		card->ext_csd.sec_feature_support =
			ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT];
		card->ext_csd.trim_timeout = 300 *
			ext_csd[EXT_CSD_TRIM_MULT];

		/*
		 * Note that the call to mmc_part_add above defaults to read
		 * only. If this default assumption is changed, the call must
		 * take into account the value of boot_locked below.
		 */
		card->ext_csd.boot_ro_lock = ext_csd[EXT_CSD_BOOT_WP];
		card->ext_csd.boot_ro_lockable = true;

		/* Save power class values */
		card->ext_csd.raw_pwr_cl_52_195 =
			ext_csd[EXT_CSD_PWR_CL_52_195];
		card->ext_csd.raw_pwr_cl_26_195 =
			ext_csd[EXT_CSD_PWR_CL_26_195];
		card->ext_csd.raw_pwr_cl_52_360 =
			ext_csd[EXT_CSD_PWR_CL_52_360];
		card->ext_csd.raw_pwr_cl_26_360 =
			ext_csd[EXT_CSD_PWR_CL_26_360];
		card->ext_csd.raw_pwr_cl_200_195 =
			ext_csd[EXT_CSD_PWR_CL_200_195];
		card->ext_csd.raw_pwr_cl_200_360 =
			ext_csd[EXT_CSD_PWR_CL_200_360];
		card->ext_csd.raw_pwr_cl_ddr_52_195 =
			ext_csd[EXT_CSD_PWR_CL_DDR_52_195];
		card->ext_csd.raw_pwr_cl_ddr_52_360 =
			ext_csd[EXT_CSD_PWR_CL_DDR_52_360];
		card->ext_csd.raw_pwr_cl_ddr_200_360 =
			ext_csd[EXT_CSD_PWR_CL_DDR_200_360];
	}

	if (card->ext_csd.rev >= 5) {
		/* Adjust production date as per JEDEC JESD84-B451 */
		if (card->cid.year < 2010)
			card->cid.year += 16;

		card->ext_csd.rel_param = ext_csd[EXT_CSD_WR_REL_PARAM];
		card->ext_csd.rst_n_function = ext_csd[EXT_CSD_RST_N_FUNCTION];

		/*
		 * RPMB regions are defined in multiples of 128K.
		 */
		card->ext_csd.raw_rpmb_size_mult = ext_csd[EXT_CSD_RPMB_MULT];
	}

	card->ext_csd.raw_erased_mem_count = ext_csd[EXT_CSD_ERASED_MEM_CONT];
	if (ext_csd[EXT_CSD_ERASED_MEM_CONT])
		card->erased_byte = 0xFF;
	else
		card->erased_byte = 0x0;

	/* eMMC v4.5 or later */
	card->ext_csd.generic_cmd6_time = DEFAULT_CMD6_TIMEOUT_MS;
	if (card->ext_csd.rev >= 6) {
		card->ext_csd.feature_support |= MMC_DISCARD_FEATURE;

		card->ext_csd.generic_cmd6_time = 10 *
			ext_csd[EXT_CSD_GENERIC_CMD6_TIME];
		card->ext_csd.power_off_longtime = 10 *
			ext_csd[EXT_CSD_POWER_OFF_LONG_TIME];

		card->ext_csd.cache_size =
			ext_csd[EXT_CSD_CACHE_SIZE + 0] << 0 |
			ext_csd[EXT_CSD_CACHE_SIZE + 1] << 8 |
			ext_csd[EXT_CSD_CACHE_SIZE + 2] << 16 |
			ext_csd[EXT_CSD_CACHE_SIZE + 3] << 24;

		if (ext_csd[EXT_CSD_DATA_SECTOR_SIZE] == 1)
			card->ext_csd.data_sector_size = 4096;
		else
			card->ext_csd.data_sector_size = 512;

		if ((ext_csd[EXT_CSD_DATA_TAG_SUPPORT] & 1) &&
		    (ext_csd[EXT_CSD_TAG_UNIT_SIZE] <= 8)) {
			card->ext_csd.data_tag_unit_size =
			((unsigned int) 1 << ext_csd[EXT_CSD_TAG_UNIT_SIZE]) *
			(card->ext_csd.data_sector_size);
		} else {
			card->ext_csd.data_tag_unit_size = 0;
		}

		card->ext_csd.max_packed_writes =
			ext_csd[EXT_CSD_MAX_PACKED_WRITES];
		card->ext_csd.max_packed_reads =
			ext_csd[EXT_CSD_MAX_PACKED_READS];
	} else {
		card->ext_csd.data_sector_size = 512;
	}

	/* eMMC v5 or later */
	if (card->ext_csd.rev >= 7) {
		memcpy(card->ext_csd.fwrev, &ext_csd[EXT_CSD_FIRMWARE_VERSION],
		       MMC_FIRMWARE_LEN);
		card->ext_csd.ffu_capable =
			(ext_csd[EXT_CSD_SUPPORTED_MODE] & 0x1) &&
			!(ext_csd[EXT_CSD_FW_CONFIG] & 0x1);
	}

	return err;
}

static int mmc_read_ext_csd(struct mmc_card *card)
{
	uint8_t ext_csd[512] __attribute__((aligned (SD_BLOCK_SIZE)));
	int err;

	if (!mmc_can_ext_csd(card))
		return 0;

	err = mmc_get_ext_csd(card, ext_csd);
	if (err)
		return err;

	err = mmc_decode_ext_csd(card, ext_csd);

	return err;
}

static void mmc_set_erase_size(struct mmc_card *card)
{
	if (card->ext_csd.erase_group_def & 1)
		card->erase_size = card->ext_csd.hc_erase_size;
	else
		card->erase_size = card->csd.erase_size;
}

/*
 * Set the bus speed for the selected speed mode.
 */
static void mmc_set_bus_speed(struct mmc_card *card)
{
	unsigned int max_dtr = (unsigned int)-1;

	if ((mmc_card_hs200(card) || mmc_card_hs400(card)) &&
	     max_dtr > card->ext_csd.hs200_max_dtr)
		max_dtr = card->ext_csd.hs200_max_dtr;
	else if (mmc_card_hs(card) && max_dtr > card->ext_csd.hs_max_dtr)
		max_dtr = card->ext_csd.hs_max_dtr;
	else if (max_dtr > card->csd.max_dtr)
		max_dtr = card->csd.max_dtr;

	mmc_set_clock(card->host, max_dtr);
}

static void mmc_select_driver_type(struct mmc_card *card)
{
	int card_drv_type, drive_strength, drv_type;

	card_drv_type = card->ext_csd.raw_driver_strength |
			mmc_driver_type_mask(0);

	drive_strength = mmc_select_drive_strength(card,
						   card->ext_csd.hs200_max_dtr,
						   card_drv_type, &drv_type);

	card->drive_strength = drive_strength;

	if (drv_type)
		mmc_set_driver_type(card->host, drv_type);
}

static inline int __mmc_send_status(struct mmc_card *card, uint32_t *status,
				    int ignore_crc)
{
	int err;
	mmc_cmd_t cmd;
	mmc_ops_t *ops = card->host->ops;
	uint32_t flags;

	flags = MMC_RSP_R1;
	if (ignore_crc)
		flags &= ~MMC_RSP_CRC;

	MMC_PACK_CMD(cmd, MMC_CMD13, card->rca << 16, flags);
	err = ops->send_cmd(card->host, &cmd);
	if (err)
		return err;

	if (status)
		*status = cmd.resp_data[0];

	return 0;
}

int mmc_switch_status_error(struct mmc_host *host, u32 status)
{
	if (status & 0xFDFFA000)
		printf("%s: unexpected status %#x after switch\n",
				__func__, status);
	if (status & R1_SWITCH_ERROR)
		return -EBADMSG;

	return 0;
}

/**
 *	__mmc_switch - modify EXT_CSD register
 *	@card: the MMC card associated with the data transfer
 *	@set: cmd set values
 *	@index: EXT_CSD register index
 *	@value: value to program into EXT_CSD register
 *	@timeout_ms: timeout (ms) for operation performed by register write,
 *                   timeout of zero implies maximum possible timeout
 *	@use_busy_signal: use the busy signal as response type
 *	@send_status: send status cmd to poll for busy
 *	@ignore_crc: ignore CRC errors when sending status cmd to poll for busy
 *
 *	Modifies the EXT_CSD register for selected card.
 */
int __mmc_switch(struct mmc_card *card, uint8_t set, uint8_t index, uint8_t value,
		unsigned int timeout_ms, int use_busy_signal, int send_status,
		int ignore_crc)
{
	struct mmc_host *host = card->host;
	int err;
	mmc_cmd_t cmd;
	mmc_ops_t *ops = card->host->ops;
	uint32_t status = 0;
	int use_r1b_resp = use_busy_signal;
	uint32_t arg, flags = 0;

	arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
		  (index << 16) |
		  (value << 8) |
		  set;

	if (use_r1b_resp) {
		flags |= MMC_RSP_R1B;
	} else {
		flags |= MMC_RSP_R1;
	}

	MMC_PACK_CMD(cmd, MMC_CMD6, arg, flags);
	err = ops->send_cmd(host, &cmd);
	if (err)
		return err;

	do {
		if (send_status) {
			err = __mmc_send_status(card, &status, ignore_crc);
			if (err)
				return err;
		}

#ifndef PLATFORM_PALLADIUM
		mdelay(timeout_ms);
#else
		udelay(timeout_ms);
#endif

	} while (R1_CURRENT_STATE(status) == R1_STATE_PRG);

	err = mmc_switch_status_error(host, status);

	return err;
}

int mmc_switch(struct mmc_card *card, uint8_t set, uint8_t index, uint8_t value,
		unsigned int timeout_ms)
{
	return __mmc_switch(card, set, index, value, timeout_ms, 1, 1,
				0);
}

int mmc_select_bus_width(struct mmc_card *card)
{
	static unsigned ext_csd_bits[] = {
		EXT_CSD_BUS_WIDTH_8,
		EXT_CSD_BUS_WIDTH_4,
		EXT_CSD_BUS_WIDTH_1,
	};
	static unsigned bus_widths[] = {
		MMC_BUS_WIDTH_8,
		MMC_BUS_WIDTH_4,
		MMC_BUS_WIDTH_1,
	};
	struct mmc_host *host = card->host;
	unsigned idx, bus_width = 0;
	int err = 0;

	if (!mmc_can_ext_csd(card))
		return 0;

	if (host->force_bus_width_1)
		idx = 2;
	else
		idx = 1;

	err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
			EXT_CSD_BUS_WIDTH,
			ext_csd_bits[idx],
			card->ext_csd.generic_cmd6_time);
	if (err)
		return err;

	bus_width = bus_widths[idx];
	mmc_set_bus_width(host, bus_width);

	return bus_width;
}

int mmc_send_status(struct mmc_card *card, u32 *status)
{
	return __mmc_send_status(card, status, 0);
}

/* Caller must hold re-tuning */
static int mmc_switch_status(struct mmc_card *card)
{
	uint32_t status;
	int err;

	err = mmc_send_status(card, &status);
	if (err)
		return err;

	return mmc_switch_status_error(card->host, status);
}

/*
 * For device supporting HS200 mode, the following sequence
 * should be done before executing the tuning process.
 * 1. set the desired bus width(4-bit or 8-bit, 1-bit is not supported)
 * 2. switch to HS200 mode
 * 3. set the clock to > 52Mhz and <=200MHz
 */
static int mmc_select_hs200(struct mmc_card *card)
{
	struct mmc_host *host = card->host;
	unsigned int old_timing, old_signal_voltage;
	int err = -EINVAL;
	u8 val;

	old_signal_voltage = host->ios.signal_voltage;
	if (card->mmc_avail_type & EXT_CSD_CARD_TYPE_HS200_1_2V)
		err = __mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_120);

	if (err && card->mmc_avail_type & EXT_CSD_CARD_TYPE_HS200_1_8V)
		err = __mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_180);

	/* If fails try again during next card power cycle */
	if (err)
		return err;

	mmc_select_driver_type(card);

	/*
	 * Set the bus width(4 or 8) with host's support and
	 * switch to HS200 mode if bus width is set successfully.
	 */
	err = mmc_select_bus_width(card);
	if (err > 0) {
		val = EXT_CSD_TIMING_HS200 |
		      card->drive_strength << EXT_CSD_DRV_STR_SHIFT;
		err = __mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				   EXT_CSD_HS_TIMING, val,
				   card->ext_csd.generic_cmd6_time,
				   1, 0, 1);
		if (err) {
			if (__mmc_set_signal_voltage(host, old_signal_voltage))
				err = -EIO;
			return err;
		}
		old_timing = host->ios.timing;
		mmc_set_timing(host, MMC_TIMING_MMC_HS200);

		err = mmc_switch_status(card);
		if (err == -EBADMSG)
			mmc_set_timing(host, old_timing);
	}

	return err;
}

/*
 * Switch to the high-speed mode
 */
static int mmc_select_hs(struct mmc_card *card)
{
	int err;

	err = __mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
			   EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS,
			   card->ext_csd.generic_cmd6_time,
			   1, 0, 1);
	if (!err) {
		mmc_set_timing(card->host, MMC_TIMING_MMC_HS);
		err = mmc_switch_status(card);
	}

	if (err)
		printf("%s: switch to high-speed failed, err:%d\n",
			__func__, err);

	if (card->host->force_1_8v)
		__mmc_set_signal_voltage(card->host, MMC_SIGNAL_VOLTAGE_180);

	return err;
}

/*
 * Activate High Speed, HS200 or HS400ES mode if supported.
 */
static int mmc_select_timing(struct mmc_card *card)
{
	int err = 0;

	if (!mmc_can_ext_csd(card))
		return -1;

	if (card->mmc_avail_type & EXT_CSD_CARD_TYPE_HS200)
		err = mmc_select_hs200(card);
	else if (card->mmc_avail_type & EXT_CSD_CARD_TYPE_HS)
		err = mmc_select_hs(card);

	if (err && err != -EBADMSG)
		return err;

	/*
	 * Set the bus speed to the selected bus timing.
	 * If timing is not selected, backward compatible is the default.
	 */
	mmc_set_bus_speed(card);

	return 0;
}

/*
 * Activate wide bus and DDR if supported.
 */
static int mmc_select_hs_ddr(struct mmc_card *card)
{
	struct mmc_host *host = card->host;
	uint32_t bus_width, ext_csd_bits;
	int err = 0;

	if (!(card->mmc_avail_type & EXT_CSD_CARD_TYPE_DDR_52))
		return 0;

	bus_width = host->ios.bus_width;
	if (bus_width == MMC_BUS_WIDTH_1)
		return 0;

	ext_csd_bits = (bus_width == MMC_BUS_WIDTH_8) ?
		EXT_CSD_DDR_BUS_WIDTH_8 : EXT_CSD_DDR_BUS_WIDTH_4;

	err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
			EXT_CSD_BUS_WIDTH,
			ext_csd_bits,
			card->ext_csd.generic_cmd6_time);
	if (err) {
		printf("%s: switch to bus width %d ddr failed\n",
			__func__, 1 << bus_width);
		return err;
	}

	/*
	 * eMMC cards can support 3.3V to 1.2V i/o (vccq)
	 * signaling.
	 *
	 * EXT_CSD_CARD_TYPE_DDR_1_8V means 3.3V or 1.8V vccq.
	 *
	 * 1.8V vccq at 3.3V core voltage (vcc) is not required
	 * in the JEDEC spec for DDR.
	 *
	 * Even (e)MMC card can support 3.3v to 1.2v vccq, but not all
	 * host controller can support this, like some of the SDHCI
	 * controller which connect to an eMMC device. Some of these
	 * host controller still needs to use 1.8v vccq for supporting
	 * DDR mode.
	 *
	 * So the sequence will be:
	 * if (host and device can both support 1.2v IO)
	 *	use 1.2v IO;
	 * else if (host and device can both support 1.8v IO)
	 *	use 1.8v IO;
	 * so if host and device can only support 3.3v IO, this is the
	 * last choice.
	 *
	 * WARNING: eMMC rules are NOT the same as SD DDR
	 */
	err = -EINVAL;
	if (card->mmc_avail_type & EXT_CSD_CARD_TYPE_DDR_1_2V)
		err = __mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_120);

	if (err && (card->mmc_avail_type & EXT_CSD_CARD_TYPE_DDR_1_8V))
		err = __mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_180);

	/* make sure vccq is 3.3v after switching disaster */
	if (err)
		err = __mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_330);

	if (!err)
		mmc_set_timing(host, MMC_TIMING_MMC_DDR52);

	return err;
}

#if 0
/*
 * Select the PowerClass for the current bus width
 * If power class is defined for 4/8 bit bus in the
 * extended CSD register, select it by executing the
 * mmc_switch command.
 */
static int __mmc_select_powerclass(struct mmc_card *card,
				   unsigned int bus_width)
{
	struct mmc_host *host = card->host;
	struct mmc_ext_csd *ext_csd = &card->ext_csd;
	unsigned int pwrclass_val = 0;
	int err = 0;

	switch (1 << host->ios.vdd) {
	case MMC_VDD_165_195:
		if (host->ios.clock <= MMC_HIGH_26_MAX_DTR)
			pwrclass_val = ext_csd->raw_pwr_cl_26_195;
		else if (host->ios.clock <= MMC_HIGH_52_MAX_DTR)
			pwrclass_val = (bus_width <= EXT_CSD_BUS_WIDTH_8) ?
				ext_csd->raw_pwr_cl_52_195 :
				ext_csd->raw_pwr_cl_ddr_52_195;
		else if (host->ios.clock <= MMC_HS200_MAX_DTR)
			pwrclass_val = ext_csd->raw_pwr_cl_200_195;
		break;
	case MMC_VDD_27_28:
	case MMC_VDD_28_29:
	case MMC_VDD_29_30:
	case MMC_VDD_30_31:
	case MMC_VDD_31_32:
	case MMC_VDD_32_33:
	case MMC_VDD_33_34:
	case MMC_VDD_34_35:
	case MMC_VDD_35_36:
		if (host->ios.clock <= MMC_HIGH_26_MAX_DTR)
			pwrclass_val = ext_csd->raw_pwr_cl_26_360;
		else if (host->ios.clock <= MMC_HIGH_52_MAX_DTR)
			pwrclass_val = (bus_width <= EXT_CSD_BUS_WIDTH_8) ?
				ext_csd->raw_pwr_cl_52_360 :
				ext_csd->raw_pwr_cl_ddr_52_360;
		else if (host->ios.clock <= MMC_HS200_MAX_DTR)
			pwrclass_val = (bus_width == EXT_CSD_DDR_BUS_WIDTH_8) ?
				ext_csd->raw_pwr_cl_ddr_200_360 :
				ext_csd->raw_pwr_cl_200_360;
		break;
	default:
		printf("%s: Voltage range not supported for power class\n",
			__func__);
		return -EINVAL;
	}

	if (bus_width & (EXT_CSD_BUS_WIDTH_8 | EXT_CSD_DDR_BUS_WIDTH_8))
		pwrclass_val = (pwrclass_val & EXT_CSD_PWR_CL_8BIT_MASK) >>
				EXT_CSD_PWR_CL_8BIT_SHIFT;
	else
		pwrclass_val = (pwrclass_val & EXT_CSD_PWR_CL_4BIT_MASK) >>
				EXT_CSD_PWR_CL_4BIT_SHIFT;

	/* If the power class is different from the default value */
	if (pwrclass_val > 0) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_POWER_CLASS,
				 pwrclass_val,
				 card->ext_csd.generic_cmd6_time);
	}

	return err;
}

static int mmc_select_powerclass(struct mmc_card *card)
{
	struct mmc_host *host = card->host;
	uint32_t bus_width, ext_csd_bits;
	int err, ddr;

	/* Power class selection is supported for versions >= 4.0 */
	if (!mmc_can_ext_csd(card))
		return 0;

	bus_width = host->ios.bus_width;
	/* Power class values are defined only for 4/8 bit bus */
	if (bus_width == MMC_BUS_WIDTH_1)
		return 0;

	ddr = card->mmc_avail_type & EXT_CSD_CARD_TYPE_DDR_52;
	if (ddr)
		ext_csd_bits = (bus_width == MMC_BUS_WIDTH_8) ?
			EXT_CSD_DDR_BUS_WIDTH_8 : EXT_CSD_DDR_BUS_WIDTH_4;
	else
		ext_csd_bits = (bus_width == MMC_BUS_WIDTH_8) ?
			EXT_CSD_BUS_WIDTH_8 :  EXT_CSD_BUS_WIDTH_4;

	err = __mmc_select_powerclass(card, ext_csd_bits);
	if (err)
		printf("%s: power class selection to bus width %d ddr %d failed\n",
			__func__, 1 << bus_width, ddr);

	return err;
}
#endif

static int mmc_init_card(struct mmc_host *host, uint32_t ocr)
{
	struct mmc_card *card = &global_mmc_card;
	int err;
	uint32_t cid[4];
	uint32_t rocr;

	mmc_go_idle(host);

	/* The extra bit indicates that we support high capacity */
	err = mmc_send_op_cond(host, ocr | (1 << 30), &rocr);
	if (err)
		return err;

	pr_debug("rocr:0x%x\n", rocr);

	err = mmc_all_send_cid(host, cid);
	if (err)
		return err;

	zeromem(card, sizeof(struct mmc_card));
	host->card = card;
	card->host = host;
	card->ocr = ocr;
	card->rca = 1;
	card->type = MMC_TYPE_MMC;
	memcpy(card->raw_cid, cid, sizeof(card->raw_cid));

	err = mmc_set_relative_addr(card);
	if (err)
		return err;

	/*
	 * Fetch CSD from card.
	 */
	err = mmc_send_csd(card, card->raw_csd);
	if (err)
		return err;

	err = mmc_decode_csd(card);
	if (err)
		return err;

	err = mmc_decode_cid(card);
	if (err)
		return err;

	err = mmc_select_card(card);
	if (err)
		return err;

	/* Read extended CSD. */
	err = mmc_read_ext_csd(card);
	if (err)
		return err;


	/* Erase size depends on CSD and Extended CSD */
	mmc_set_erase_size(card);

	/*
	 * Ensure eMMC user default partition is enabled
	 */
	if (card->ext_csd.part_config & EXT_CSD_PART_CONFIG_ACC_MASK) {
		card->ext_csd.part_config &= ~EXT_CSD_PART_CONFIG_ACC_MASK;
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_PART_CONFIG,
				 card->ext_csd.part_config,
				 card->ext_csd.part_time);
		if (err && err != -EBADMSG)
			return err;
	}

	/*
	 * Select timing interface
	 */
	err = mmc_select_timing(card);
	if (err)
		return err;

	if (mmc_card_hs200(card)) {
		err = mmc_execute_tuning(card);
		if (err)
			return err;
	} else {
		/* Select the desired bus width optionally */
		err = mmc_select_bus_width(card);
		if (err > 0) {
			if(mmc_card_hs(card)) {
				err = mmc_select_hs_ddr(card);
				if (err)
					return err;
			} else {
				err = 0;
			}
		}
		if (host->force_bus_clock)
			mmc_set_clock(host, host->force_bus_clock);
	}

#if 0
	/*
	 * Choose the power class with selected bus interface
	 */
	err = mmc_select_powerclass(card);
#endif

	return err;
}

int mmc_attach_mmc(struct mmc_host *host)
{
	int err;
	uint32_t ocr, rocr;

	err = mmc_send_op_cond(host, 0, &ocr);
	if (err)
		return err;

	rocr = mmc_select_voltage(host, ocr);
	if (!rocr) {
		return -1;
	}

	err = mmc_init_card(host, rocr);

	return err;
}

int mmc_rescan(struct mmc_host *host)
{
	host->ops->init(host);

	mmc_go_idle(host);

	if (host->flags & SDHCI_TYPE_SDCARD) {
		if (mmc_attach_sd(host))
			return -1;
	}

	if (host->flags & SDHCI_TYPE_EMMC) {
		if (mmc_attach_mmc(host))
			return -1;
	}

	return 0;
}

void mmc_destory(struct mmc_host *host)
{
	mmc_go_idle(host);
}

size_t mmc_read_blocks(struct mmc_host *host, int lba, uintptr_t buf, size_t size)
{
	mmc_cmd_t cmd;
	int ret;
	mmc_ops_t *ops = host->ops;
	struct mmc_data data;

	ASSERT((ops != 0) &&
	       (ops->read != 0) &&
	       ((buf & SD_BLOCK_MASK) == 0) &&
	       ((size & SD_BLOCK_MASK) == 0));

	data.buf = (void *)buf;
	data.blksz = SD_BLOCK_SIZE;
	data.blocks = size / SD_BLOCK_SIZE;
	data.flags = MMC_DATA_READ;

	if (is_cmd23_enabled(host->flags)) {
		zeromem(&cmd, sizeof(mmc_cmd_t));
		/* set block count */
		cmd.cmd_idx = MMC_CMD23;
		cmd.cmd_arg = size / SD_BLOCK_SIZE;
		cmd.resp_type = MMC_RSP_R1;
		ret = ops->send_cmd(host, &cmd);
		ASSERT(ret == 0);

		zeromem(&cmd, sizeof(mmc_cmd_t));
		cmd.cmd_idx = MMC_CMD18;
	} else {
		if (size > SD_BLOCK_SIZE)
			cmd.cmd_idx = MMC_CMD18;
		else
			cmd.cmd_idx = MMC_CMD17;
	}

	cmd.cmd_arg = lba;
	cmd.resp_type = MMC_RSP_R1;
	cmd.data = &data;
	ret = ops->send_cmd(host, &cmd);
	ASSERT(ret == 0);

	if (is_cmd23_enabled(host->flags) == 0) {
		if (size > SD_BLOCK_SIZE) {
			zeromem(&cmd, sizeof(mmc_cmd_t));
			cmd.cmd_idx = MMC_CMD12;
			ret = ops->send_cmd(host, &cmd);
			ASSERT(ret == 0);
		}
	}

	/* wait buffer empty */
	sd_device_state(host->card);

	/* Ignore improbable errors in release builds */
	(void)ret;
	return size;
}

size_t mmc_write_blocks(struct mmc_host *host, int lba, const uintptr_t buf, size_t size)
{
	mmc_cmd_t cmd;
	int ret;
	mmc_ops_t *ops = host->ops;
	struct mmc_data data;

	ASSERT((ops != 0) &&
	       (ops->write != 0) &&
	       ((buf & SD_BLOCK_MASK) == 0) &&
	       ((size & SD_BLOCK_MASK) == 0));


	flush_dcache_range(buf, (unsigned long)buf + size);

	data.buf = (void *)buf;
	data.blksz = SD_BLOCK_SIZE;
	data.blocks = size / SD_BLOCK_SIZE;
	data.flags = MMC_DATA_WRITE;

	if (is_cmd23_enabled(host->flags)) {
		/* set block count */
		zeromem(&cmd, sizeof(mmc_cmd_t));
		cmd.cmd_idx = MMC_CMD23;
		cmd.cmd_arg = size / SD_BLOCK_SIZE;
		cmd.resp_type = MMC_RSP_R1;
		ret = ops->send_cmd(host, &cmd);
		ASSERT(ret == 0);

		zeromem(&cmd, sizeof(mmc_cmd_t));
		cmd.cmd_idx = MMC_CMD25;
	} else {
		zeromem(&cmd, sizeof(mmc_cmd_t));
		if (size > SD_BLOCK_SIZE)
			cmd.cmd_idx = MMC_CMD25;
		else
			cmd.cmd_idx = MMC_CMD24;
	}

	cmd.cmd_arg = lba;
	cmd.resp_type = MMC_RSP_R1;
	cmd.data = &data;
	ret = ops->send_cmd(host, &cmd);
	ASSERT(ret == 0);

	if (is_cmd23_enabled(host->flags) == 0) {
		if (size > SD_BLOCK_SIZE) {
			zeromem(&cmd, sizeof(mmc_cmd_t));
			cmd.cmd_idx = MMC_CMD12;
			ret = ops->send_cmd(host, &cmd);
			ASSERT(ret == 0);
		}
	}

	/* wait buffer empty */
	sd_device_state(host->card);

	/* Ignore improbable errors in release builds */
	(void)ret;
	return size;
}

size_t mmc_erase_blocks(struct mmc_host *host, int lba, size_t size)
{
	mmc_cmd_t cmd;
	int ret, state;
	mmc_ops_t *ops = host->ops;

	ASSERT(ops != 0);
	ASSERT((size != 0) && ((size % SD_BLOCK_SIZE) == 0));

	zeromem(&cmd, sizeof(mmc_cmd_t));
	if (mmc_card_mmc(host->card))
		cmd.cmd_idx = MMC_CMD35;
	else
		cmd.cmd_idx = MMC_CMD32;

	cmd.cmd_arg = lba;
	cmd.resp_type = MMC_RSP_R1;
	ret = ops->send_cmd(host, &cmd);
	ASSERT(ret == 0);

	zeromem(&cmd, sizeof(mmc_cmd_t));
	if (mmc_card_mmc(host->card))
		cmd.cmd_idx = MMC_CMD36;
	else
		cmd.cmd_idx = MMC_CMD33;

	cmd.cmd_arg = lba + (size / SD_BLOCK_SIZE) - 1;
	cmd.resp_type = MMC_RSP_R1;
	ret = ops->send_cmd(host, &cmd);
	ASSERT(ret == 0);

	zeromem(&cmd, sizeof(mmc_cmd_t));
	cmd.cmd_idx = MMC_CMD38;
	cmd.resp_type = MMC_RSP_R1B;
	ret = ops->send_cmd(host, &cmd);
	ASSERT(ret == 0);

	/* wait to TRAN state */
	do {
		state = sd_device_state(host->card);
		pr_debug("state:%d\r\n", state);
	} while (state != SD_STATE_TRAN);
	/* Ignore improbable errors in release builds */
	(void)ret;
	return size;
}

