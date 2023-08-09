/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __SD_H__
#define __SD_H__

#include <stdint.h>
#include <stddef.h>
#include <asm/system.h>
#include "utils_def.h"
#include "timer.h"

#define MMC_PACK_CMD_DATA(cmd, idx, arg, type, dat)	\
{	\
	cmd.cmd_idx = idx;	\
	cmd.cmd_arg = arg;	\
	cmd.resp_type = type;	\
	cmd.data = 	dat;	\
}

#define MMC_PACK_CMD(cmd, idx, arg, type)	\
{	\
	cmd.cmd_idx = idx;	\
	cmd.cmd_arg = arg;	\
	cmd.resp_type = type;	\
	cmd.data = 	NULL;	\
}

#define SD_BLOCK_SIZE			512
#define SD_BLOCK_MASK			(SD_BLOCK_SIZE - 1)
#define SD_BOOT_CLK_RATE		(400 * 1000)

#define SCR_SPEC_VER_0      0   /* Implements system specification 1.0 - 1.01 */
#define SCR_SPEC_VER_1      1   /* Implements system specification 1.10 */
#define SCR_SPEC_VER_2      2   /* Implements system specification 2.00-3.0X */
#define CCC_SWITCH		(1<<10)	/* (10) High speed switch */
#define SD_OCR_S18R     (1 << 24)    /* 1.8V switching request */
#define SD_ROCR_S18A		SD_OCR_S18R  /* 1.8V switching accepted by card */

#define MMC_CMD0			0
#define MMC_CMD1			1
#define MMC_CMD2			2
#define MMC_CMD3			3
#define MMC_CMD6			6
#define MMC_CMD7			7
#define MMC_CMD8			8
#define MMC_CMD9			9
#define MMC_CMD11			11
#define MMC_CMD12			12
#define MMC_CMD13			13
#define MMC_CMD16			16
#define MMC_CMD17			17
#define MMC_CMD18			18
#define MMC_CMD19			19
#define MMC_CMD21			21
#define MMC_CMD23			23
#define MMC_CMD24			24
#define MMC_CMD25			25
#define MMC_CMD32			32
#define MMC_CMD33			33
#define MMC_CMD35			35
#define MMC_CMD36			36
#define MMC_CMD38			38
#define MMC_CMD52			52
#define MMC_CMD55			55
#define SD_ACMD6			6
#define SD_ACMD13			13
#define SD_ACMD41			41
#define SD_ACMD42			42
#define SD_ACMD51			51

static inline int mmc_op_multi(uint32_t opcode)
{
	return opcode == MMC_CMD25 || opcode == MMC_CMD18;
}

#define OCR_POWERUP			(1 << 31)
#define OCR_BYTE_MODE			(0 << 29)
#define OCR_SECTOR_MODE			(2 << 29)
#define OCR_ACCESS_MODE_MASK		(3 << 29)
#define OCR_VDD_MIN_2V7			(0x1ff << 15)
#define OCR_VDD_MIN_2V0			(0x7f << 8)
#define OCR_VDD_MIN_1V7			(1 << 7)

#define SD_RESPONSE_R1		1
#define SD_RESPONSE_R1B		1
#define SD_RESPONSE_R2		4
#define SD_RESPONSE_R3		1
#define SD_RESPONSE_R4		1
#define SD_RESPONSE_R5		1
#define SD_RESPONSE_R6		1
#define SD_RESPONSE_R7		1

#define MMC_RSP_PRESENT (1 << 0)
#define MMC_RSP_136 (1 << 1)        /* 136 bit response */
#define MMC_RSP_CRC (1 << 2)        /* expect valid crc */
#define MMC_RSP_BUSY    (1 << 3)        /* card may send busy */
#define MMC_RSP_OPCODE  (1 << 4)        /* response contains opcode */

#define MMC_RSP_NONE    (0)
#define MMC_RSP_R1  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1B (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE|MMC_RSP_BUSY)
#define MMC_RSP_R2  (MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC)
#define MMC_RSP_R3  (MMC_RSP_PRESENT)
#define MMC_RSP_R4  (MMC_RSP_PRESENT)
#define MMC_RSP_R5  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R6  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R7  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

#define MMC_SD_CARD_BUSY   0xc0000000  /* Card Power up status bit */

#define SD_FIX_RCA			6	/* > 1 */
#define RCA_SHIFT_OFFSET		16

#define CMD_EXTCSD_PARTITION_CONFIG	179
#define CMD_EXTCSD_BUS_WIDTH		183
#define CMD_EXTCSD_HS_TIMING		185

#define PART_CFG_BOOT_PARTITION1_ENABLE	(1 << 3)
#define PART_CFG_BOOT_PARTITION2_ENABLE (2 << 3)
#define PART_CFG_PARTITION_BOOT1_ACCESS (1 << 0)
#define PART_CFG_PARTITION_BOOT2_ACCESS (2 << 0)
#define PART_CFG_PARTITION_RPMB_ACCESS  (3 << 0)
#define PART_CFG_PARTITION_GP1_ACCESS   (4 << 0)
#define PART_CFG_PARTITION_GP2_ACCESS   (5 << 0)
#define PART_CFG_PARTITION_GP3_ACCESS   (6 << 0)
#define PART_CFG_PARTITION_GP4_ACCESS   (7 << 0)

/* values in EXT CSD register */
#define SD_BOOT_MODE_BACKWARD		(0 << 3)
#define SD_BOOT_MODE_HS_TIMING	(1 << 3)
#define SD_BOOT_MODE_DDR		(2 << 3)

#define EXTCSD_SET_CMD			(0 << 24)
#define EXTCSD_SET_BITS			(1 << 24)
#define EXTCSD_CLR_BITS			(2 << 24)
#define EXTCSD_WRITE_BYTES		(3 << 24)
#define EXTCMMC_CMD(x)			(((x) & 0xff) << 16)
#define EXTCSD_VALUE(x)			(((x) & 0xff) << 8)

#define STATUS_CURRENT_STATE(x)		(((x) & 0xf) << 9)
#define STATUS_READY_FOR_DATA		(1 << 8)
#define STATUS_SWITCH_ERROR		(1 << 7)
#define SD_GET_STATE(x)		(((x) >> 9) & 0xf)
#define SD_STATE_IDLE			0
#define SD_STATE_READY		1
#define SD_STATE_IDENT		2
#define SD_STATE_STBY			3
#define SD_STATE_TRAN			4
#define SD_STATE_DATA			5
#define SD_STATE_RCV			6
#define SD_STATE_PRG			7
#define SD_STATE_DIS			8
#define SD_STATE_BTST			9
#define SD_STATE_SLP			10

typedef enum {
  SD_PARTITION_BOOT1   = 1,
  SD_PARTITION_BOOT2   = 2,
  SD_PARTITION_RPMB    = 3,
  SD_PARTITION_GP1     = 4,
  SD_PARTITION_GP2     = 5,
  SD_PARTITION_GP3     = 6,
  SD_PARTITION_GP4     = 7,
  SD_PARTITION_UDA     = 8,

  SD_PARTITION_MAX     = 0xFF
}sd_partition_type;

struct mmc_data {
	void			*buf;
	unsigned int		blksz;		/* data block size */
	unsigned int		blocks;		/* number of blocks */
	unsigned int		flags;
#define MMC_DATA_WRITE	(1 << 8)
#define MMC_DATA_READ	(1 << 9)
	int				error;
};

typedef struct mmc_cmd {
	unsigned int	cmd_idx;
	unsigned int	cmd_arg;
	unsigned int	resp_type;
	unsigned int	resp_data[4];
	volatile int	error;
	struct mmc_data *data;
} mmc_cmd_t;

struct sd_switch_caps {
	unsigned int		hs_max_dtr;
	unsigned int		uhs_max_dtr;
#define HIGH_SPEED_MAX_DTR	50000000
#define UHS_SDR104_MAX_DTR	208000000
#define UHS_SDR50_MAX_DTR	100000000
#define UHS_DDR50_MAX_DTR	50000000
#define UHS_SDR25_MAX_DTR	UHS_DDR50_MAX_DTR
#define UHS_SDR12_MAX_DTR	25000000
	unsigned int		sd3_bus_mode;
#define DFLT_SPEED_BUS_SPEED	0
#define UHS_SDR12_BUS_SPEED		0
#define HIGH_SPEED_BUS_SPEED	1
#define UHS_SDR25_BUS_SPEED		1
#define UHS_SDR50_BUS_SPEED		2
#define UHS_SDR104_BUS_SPEED	3
#define UHS_DDR50_BUS_SPEED		4

#define SD_MODE_DFLT_SPEED	(1 << DFLT_SPEED_BUS_SPEED)
#define SD_MODE_HIGH_SPEED	(1 << HIGH_SPEED_BUS_SPEED)
#define SD_MODE_UHS_SDR12	(1 << UHS_SDR12_BUS_SPEED)
#define SD_MODE_UHS_SDR25	(1 << UHS_SDR25_BUS_SPEED)
#define SD_MODE_UHS_SDR50	(1 << UHS_SDR50_BUS_SPEED)
#define SD_MODE_UHS_SDR104	(1 << UHS_SDR104_BUS_SPEED)
#define SD_MODE_UHS_DDR50	(1 << UHS_DDR50_BUS_SPEED)
#define SD_MODE_UHS_MASK	(SD_MODE_UHS_SDR50 | SD_MODE_UHS_SDR104 | SD_MODE_UHS_DDR50)
#define SD_MODE_MASK		(SD_MODE_DFLT_SPEED | SD_MODE_HIGH_SPEED | SD_MODE_UHS_SDR12 | \
									SD_MODE_UHS_SDR25 | SD_MODE_UHS_SDR50 | \
									SD_MODE_UHS_SDR104 | SD_MODE_UHS_DDR50)
	unsigned int		sd3_drv_type;
#define SD_DRIVER_TYPE_B	0x01
#define SD_DRIVER_TYPE_A	0x02
#define SD_DRIVER_TYPE_C	0x04
#define SD_DRIVER_TYPE_D	0x08
	unsigned int		sd3_curr_limit;
#define SD_SET_CURRENT_LIMIT_200	0
#define SD_SET_CURRENT_LIMIT_400	1
#define SD_SET_CURRENT_LIMIT_600	2
#define SD_SET_CURRENT_LIMIT_800	3
#define SD_SET_CURRENT_NO_CHANGE	(-1)

#define SD_MAX_CURRENT_200	(1 << SD_SET_CURRENT_LIMIT_200)
#define SD_MAX_CURRENT_400	(1 << SD_SET_CURRENT_LIMIT_400)
#define SD_MAX_CURRENT_600	(1 << SD_SET_CURRENT_LIMIT_600)
#define SD_MAX_CURRENT_800	(1 << SD_SET_CURRENT_LIMIT_800)
};

struct mmc_ios {
	unsigned int	clock;			/* clock rate */

	unsigned char	power_mode;		/* power supply mode */

#define MMC_POWER_OFF		0
#define MMC_POWER_UP		1
#define MMC_POWER_ON		2
#define MMC_POWER_UNDEFINED	3

	unsigned char	bus_width;		/* data bus width */

#define MMC_BUS_WIDTH_1		0
#define MMC_BUS_WIDTH_4		2
#define MMC_BUS_WIDTH_8		3

	unsigned char	timing;			/* timing specification used */

#define MMC_TIMING_LEGACY	0
#define MMC_TIMING_MMC_HS	1
#define MMC_TIMING_SD_HS	2
#define MMC_TIMING_UHS_SDR12	3
#define MMC_TIMING_UHS_SDR25	4
#define MMC_TIMING_UHS_SDR50	5
#define MMC_TIMING_UHS_SDR104	6
#define MMC_TIMING_UHS_DDR50	7
#define MMC_TIMING_MMC_DDR52	8
#define MMC_TIMING_MMC_HS200	9
#define MMC_TIMING_MMC_HS400	10

	unsigned char	signal_voltage;		/* signalling voltage (1.8V or 3.3V) */

#define MMC_SIGNAL_VOLTAGE_330	0
#define MMC_SIGNAL_VOLTAGE_180	1
#define MMC_SIGNAL_VOLTAGE_120	2

	unsigned char	drv_type;		/* driver type (A, B, C, D) */

#define MMC_SET_DRIVER_TYPE_B	0
#define MMC_SET_DRIVER_TYPE_A	1
#define MMC_SET_DRIVER_TYPE_C	2
#define MMC_SET_DRIVER_TYPE_D	3

};

typedef struct mmc_ops mmc_ops_t;
struct mmc_host {
#define MMC_VDD_165_195		0x00000080	/* VDD voltage 1.65 - 1.95 */
#define MMC_VDD_20_21		0x00000100	/* VDD voltage 2.0 ~ 2.1 */
#define MMC_VDD_21_22		0x00000200	/* VDD voltage 2.1 ~ 2.2 */
#define MMC_VDD_22_23		0x00000400	/* VDD voltage 2.2 ~ 2.3 */
#define MMC_VDD_23_24		0x00000800	/* VDD voltage 2.3 ~ 2.4 */
#define MMC_VDD_24_25		0x00001000	/* VDD voltage 2.4 ~ 2.5 */
#define MMC_VDD_25_26		0x00002000	/* VDD voltage 2.5 ~ 2.6 */
#define MMC_VDD_26_27		0x00004000	/* VDD voltage 2.6 ~ 2.7 */
#define MMC_VDD_27_28		0x00008000	/* VDD voltage 2.7 ~ 2.8 */
#define MMC_VDD_28_29		0x00010000	/* VDD voltage 2.8 ~ 2.9 */
#define MMC_VDD_29_30		0x00020000	/* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31		0x00040000	/* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_31_32		0x00080000	/* VDD voltage 3.1 ~ 3.2 */
#define MMC_VDD_32_33		0x00100000	/* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34		0x00200000	/* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_34_35		0x00400000	/* VDD voltage 3.4 ~ 3.5 */
#define MMC_VDD_35_36		0x00800000	/* VDD voltage 3.5 ~ 3.6 */
	uint32_t			ocr_avail;

	int flags;		/* Host attributes */
#define SDHCI_USE_SDMA		(1<<0)	/* Host is SDMA capable */
#define SDHCI_USE_ADMA2		(1<<1)	/* Host is ADMA2 capable */
#define SDHCI_REQ_USE_DMA	(1<<2)	/* Use DMA for this req. */
#define SDHCI_DEVICE_DEAD	(1<<3)	/* Device unresponsive */
#define SDHCI_SDR50_NEEDS_TUNING (1<<4)	/* SDR50 needs tuning */
#define SDHCI_AUTO_CMD12	(1<<6)	/* Auto CMD12 support */
#define SDHCI_AUTO_CMD23	(1<<7)	/* Auto CMD23 support */
#define SDHCI_PV_ENABLED	(1<<8)	/* Preset value enabled */
#define SDHCI_SDIO_IRQ_ENABLED	(1<<9)	/* SDIO irq enabled */
#define SDHCI_USE_64_BIT_DMA	(1<<12)	/* Use 64-bit DMA */
#define SDHCI_HS400_TUNING	(1<<13)	/* Tuning for HS400 */
#define SDHCI_SIGNALING_330	(1<<14)	/* Host is capable of 3.3V signaling */
#define SDHCI_SIGNALING_180	(1<<15)	/* Host is capable of 1.8V signaling */
#define SDHCI_SIGNALING_120	(1<<16)	/* Host is capable of 1.2V signaling */
#define SDHCI_USE_ADMA3		(1<<17)	/* Host is ADMA3 capable */
#define SDHCI_USE_VERSION4	(1<<18)	/* Host is VERSION4 capable */
#define SDHCI_USE_CMD23		(1<<19)	/* Host is support CMD23 */
#define SDHCI_TYPE_SDCARD	(1<<20)
#define SDHCI_TYPE_EMMC		(1<<21)

	uint32_t			bus_mode;
	uint32_t			force_1_8v;
	uint32_t			force_bus_width_1;
	uint32_t			force_bus_clock;

#define SDHCI_ADMA2_BUF_SIZE	65536
#define SDHCI_ADMA_ALIGN	4
#define SDHCI_ADMA_MASK	(SDHCI_ADMA_ALIGN - 1)
#define SDHCI_ADMA2_64_DESC_SZ	16
#define SDHCI_ADMA2_MAX_DESC_COUNT	8
#define ADMA2_TRAN_VALID    0x21
#define ADMA2_TRAN_END_VALID    0x23
#define ADMA2_NOP_END_VALID 0x3
#define ADMA2_END       0x2
#define ADMA3_CMD_VALID	0x9
#define ADMA3_CMD_END_VALID	0xB
#define ADMA3_INTEGRATED_VALID	0x39
#define ADMA3_INTEGRATED_END_VALID	0x3B
#define SDHCI_ADMA3_CMD_DESC_SZ	8
#define SDHCI_ADMA3_CMD_DESC_COUNT	4
	unsigned int desc_sz;
	void *adma_table;
	size_t adma_table_sz;
	unsigned int adma_buf_size;
	void *intgr_table;

	struct mmc_card		*card;		/* device attached to this host */
	mmc_ops_t *ops;
	volatile mmc_cmd_t *cmd;
	volatile mmc_cmd_t *data_cmd;
	volatile struct mmc_data *data;
	struct mmc_ios ios;
	void *priv;
	unsigned int	clock;
	unsigned char	timing;			/* timing specification used */
	unsigned int	ier;
	volatile unsigned int	cmd_completion;
	volatile unsigned int	data_completion;
};

struct sdhci_adma2_64_desc {
	unsigned int attr:6;
	unsigned int len_10:10;
	unsigned int len_16:16;
	uint64_t addr;
	unsigned int reserved;
} __attribute__((packed)) __attribute__((aligned(4)));

struct sdhci_adma3_cmd_desc {
	uint32_t attr:6;
	uint32_t reserved:10;
	uint16_t reserved2;
	uint32_t reg;
} __attribute__((packed)) __attribute__((aligned(4)));

struct mmc_ops {
	void (*init)(struct mmc_host *host);
	int (*send_cmd)(struct mmc_host *host, mmc_cmd_t *cmd);
	int (*set_ios)(struct mmc_host *host, struct mmc_ios *ios);
	int (*read)(int lba, uintptr_t buf, size_t size);
	int (*write)(int lba, const uintptr_t buf, size_t size);
	int (*start_signal_voltage_switch)(struct mmc_host *mmc, struct mmc_ios *ios);
	int (*select_drive_strength)(struct mmc_card *card, unsigned int max_dtr, int host_drv,int card_drv, int *drv_type);
	int (*card_busy)(struct mmc_host *host);
	int (*execute_tuning)(struct mmc_host *host, uint32_t opcode);
};

typedef struct sd_ops {
	void (*init)(void);
	int (*send_cmd)(mmc_cmd_t *cmd);
	int (*set_ios)(int clk, int width);
	int (*prepare)(int lba, uintptr_t buf, size_t size);
	int (*read)(int lba, uintptr_t buf, size_t size);
	int (*write)(int lba, const uintptr_t buf, size_t size);
	int (*start_signal_voltage_switch)(struct mmc_host *mmc);
	int (*card_busy)(struct mmc_host *mmc);
} sd_ops_t;

struct mmc_cid {
	unsigned int		manfid;
	unsigned int		reserve;
	char			prod_name[8];
	unsigned char		prv;
	unsigned int		serial;
	unsigned short		oemid;
	unsigned short		year;
	unsigned char		hwrev;
	unsigned char		fwrev;
	unsigned char		month;
};

struct mmc_csd {
	unsigned char		structure;
	unsigned char		mmca_vsn;
	unsigned short		cmdclass;
	unsigned short		tacc_clks;
	unsigned int		tacc_ns;
	unsigned int		c_size;
	unsigned int		r2w_factor;
	unsigned int		max_dtr;
	unsigned int		erase_size;		/* In sectors */
	unsigned int		read_blkbits;
	unsigned int		write_blkbits;
	unsigned int		capacity;
	unsigned int		read_partial:1,
				read_misalign:1,
				write_partial:1,
				write_misalign:1,
				dsr_imp:1;
};

struct sd_scr {
	unsigned char		sda_vsn;
	unsigned char		sda_spec3;
	unsigned char		bus_widths;
#define SD_SCR_BUS_WIDTH_1	(1<<0)
#define SD_SCR_BUS_WIDTH_4	(1<<2)
	unsigned char		cmds;
#define SD_SCR_CMD20_SUPPORT   (1<<0)
#define SD_SCR_CMD23_SUPPORT   (1<<1)
};

struct sd_ssr {
	unsigned int		au;			/* In sectors */
	unsigned int		erase_timeout;		/* In milliseconds */
	unsigned int		erase_offset;		/* In milliseconds */
};

struct mmc_ext_csd {
	uint8_t			rev;
	uint8_t			erase_group_def;
	uint8_t			sec_feature_support;
	uint8_t			rel_sectors;
	uint8_t			rel_param;
	uint8_t			part_config;
	uint8_t			cache_ctrl;
	uint8_t			rst_n_function;
	uint8_t			max_packed_writes;
	uint8_t			max_packed_reads;
	uint8_t			packed_event_en;
	unsigned int		part_time;		/* Units: ms */
	unsigned int		sa_timeout;		/* Units: 100ns */
	unsigned int		generic_cmd6_time;	/* Units: 10ms */
	unsigned int            power_off_longtime;     /* Units: ms */
	uint8_t			power_off_notification;	/* state */
	unsigned int		hs_max_dtr;
	unsigned int		hs200_max_dtr;
#define MMC_HIGH_26_MAX_DTR	26000000
#define MMC_HIGH_52_MAX_DTR	52000000
#define MMC_HIGH_DDR_MAX_DTR	52000000
#define MMC_HS200_MAX_DTR	200000000
	unsigned int		sectors;
	unsigned int		hc_erase_size;		/* In sectors */
	unsigned int		hc_erase_timeout;	/* In milliseconds */
	unsigned int		sec_trim_mult;	/* Secure trim multiplier  */
	unsigned int		sec_erase_mult;	/* Secure erase multiplier */
	unsigned int		trim_timeout;		/* In milliseconds */
	int			partition_setting_completed;	/* enable bit */
	unsigned long long	enhanced_area_offset;	/* Units: Byte */
	unsigned int		enhanced_area_size;	/* Units: KB */
	unsigned int		cache_size;		/* Units: KB */
	int			hpi_en;			/* HPI enablebit */
	int			hpi;			/* HPI support bit */
	unsigned int		hpi_cmd;		/* cmd used as HPI */
	int			bkops;		/* background support bit */
	int			man_bkops_en;	/* manual bkops enable bit */
	unsigned int            data_sector_size;       /* 512 bytes or 4KB */
	unsigned int            data_tag_unit_size;     /* DATA TAG UNIT size */
	unsigned int		boot_ro_lock;		/* ro lock support */
	int			boot_ro_lockable;
	int			ffu_capable;	/* Firmware upgrade support */
#define MMC_FIRMWARE_LEN 8
	uint8_t			fwrev[MMC_FIRMWARE_LEN];  /* FW version */
	uint8_t			raw_exception_status;	/* 54 */
	uint8_t			raw_partition_support;	/* 160 */
	uint8_t			raw_rpmb_size_mult;	/* 168 */
	uint8_t			raw_erased_mem_count;	/* 181 */
	uint8_t			strobe_support;		/* 184 */
	uint8_t			raw_ext_csd_structure;	/* 194 */
	uint8_t			raw_card_type;		/* 196 */
	uint8_t			raw_driver_strength;	/* 197 */
	uint8_t			out_of_int_time;	/* 198 */
	uint8_t			raw_pwr_cl_52_195;	/* 200 */
	uint8_t			raw_pwr_cl_26_195;	/* 201 */
	uint8_t			raw_pwr_cl_52_360;	/* 202 */
	uint8_t			raw_pwr_cl_26_360;	/* 203 */
	uint8_t			raw_s_a_timeout;	/* 217 */
	uint8_t			raw_hc_erase_gap_size;	/* 221 */
	uint8_t			raw_erase_timeout_mult;	/* 223 */
	uint8_t			raw_hc_erase_grp_size;	/* 224 */
	uint8_t			raw_sec_trim_mult;	/* 229 */
	uint8_t			raw_sec_erase_mult;	/* 230 */
	uint8_t			raw_sec_feature_support;/* 231 */
	uint8_t			raw_trim_mult;		/* 232 */
	uint8_t			raw_pwr_cl_200_195;	/* 236 */
	uint8_t			raw_pwr_cl_200_360;	/* 237 */
	uint8_t			raw_pwr_cl_ddr_52_195;	/* 238 */
	uint8_t			raw_pwr_cl_ddr_52_360;	/* 239 */
	uint8_t			raw_pwr_cl_ddr_200_360;	/* 253 */
	uint8_t			raw_bkops_status;	/* 246 */
	uint8_t			raw_sectors[4];		/* 212 - 4 bytes */

	unsigned int            feature_support;
#define MMC_DISCARD_FEATURE	BIT(0)                  /* CMD38 feature */
};

#define mmc_card_mmc(c)     ((c)->type == MMC_TYPE_MMC)
#define mmc_card_sd(c)      ((c)->type == MMC_TYPE_SD)

struct mmc_card {
	struct mmc_host *host;
	unsigned int		type;		/* card type */
#define MMC_TYPE_MMC		0		/* MMC card */
#define MMC_TYPE_SD			1		/* SD card */
#define MMC_TYPE_SDIO		2		/* SDIO card */
#define MMC_TYPE_SD_COMBO	3		/* SD combo (IO+mem) card */
	uint32_t ocr;
	uint32_t rca;
	uint32_t quirks;
	uint32_t erase_size;
	uint32_t erase_shift;
	uint32_t erased_byte;
	uint32_t reserved;

	uint32_t raw_cid[4];
	uint32_t raw_csd[4];
	uint32_t raw_scr[2];
	uint32_t raw_ssr[16];

	struct mmc_cid cid;
	struct mmc_csd csd;
	struct mmc_ext_csd ext_csd;
	struct sd_scr scr;
	struct sd_ssr ssr;
	struct sd_switch_caps sw_caps;

	uint32_t sd_bus_speed;
	uint32_t drive_strength;
	unsigned int mmc_avail_type;	/* supported device type by both host and card */
};

typedef struct sd_cid {
  unsigned int    manufacturing_date:     8;
  unsigned int    product_serial_low:     24;
  unsigned short  product_serial_high:    8;
  unsigned short  product_revision:       8;
  char            product_name[6];
  unsigned int    oem_application_id:     8;
  unsigned int    card_bga:               2;
  unsigned int    reserverd_0:            6;
  unsigned int    manufacturer_id:        8;
  unsigned int    reserved_1:             8;/*dw spec page 231 response135-104 */
} sd_cid_t;

typedef struct sd_csd {
  /*data0*/
  unsigned int    ecc:                   2;
  unsigned int    file_format:           2;
  unsigned int    tmp_write_protect:     1;
  unsigned int    perm_write_protect:    1;
  unsigned int    copy:                  1;
  unsigned int    file_format_grp:       1;
  unsigned int    content_prot_app:      1;
  unsigned int    reserved_1:            4;
  unsigned int    write_bl_partial:      1;
  unsigned int    write_bl_len:          4;
  unsigned int    r2w_factor:            3;
  unsigned int    default_ecc:           2;
  unsigned int    wp_grp_enable:         1;
  unsigned int    wp_grp_size:           5;
  unsigned int    erase_grp_mult_low:    3;

  /*data1*/
  unsigned int    erase_grp_mult_high:   2;
  unsigned int    erase_grp_size:        5;
  unsigned int    c_size_mult:           3;
  unsigned int    vdd_w_curr_max:        3;
  unsigned int    vdd_w_curr_min:        3;
  unsigned int    vdd_r_curr_max:        3;
  unsigned int    vdd_r_curr_min:        3;
  unsigned int    c_size_low:            10;

  /*data2*/
  unsigned int    c_size_high:           2;
  unsigned int    reserved_2:            2;
  unsigned int    dsr_imp:               1;
  unsigned int    read_blk_misalign:     1;
  unsigned int    write_blk_misalign:    1;
  unsigned int    read_bl_partial:       1;
  unsigned int    read_bl_len:           4;
  unsigned int    ccc:                   12;
  unsigned int    tran_speed:            8;

  /*data3*/
  unsigned int    nsac:                  8; /*bit 104*/
  unsigned int    taac:                  8;
  unsigned int    reserved_3:            2;
  unsigned int    spec_vers:             4;
  unsigned int    csd_structure:         2;
  unsigned int    reserved_4:            8; /*dw spec page 231 response135-104 */
} sd_csd_t;

size_t mmc_read_blocks(struct mmc_host *host, int lba, uintptr_t buf, size_t size);
size_t mmc_write_blocks(struct mmc_host *host, int lba, const uintptr_t buf, size_t size);
size_t mmc_erase_blocks(struct mmc_host *host, int lba, size_t size);

void mmc_destory(struct mmc_host *host);
int mmc_rescan(struct mmc_host *host);
int mmc_execute_tuning(struct mmc_card *card);
int mmc_app_set_bus_width(struct mmc_card *card, int width);
void mmc_set_bus_width(struct mmc_host *host, uint32_t width);
int mmc_select_bus_width(struct mmc_card *card);

/* --------------------- emmc ------------------------ */
/*
 * MMC_SWITCH argument format:
 *
 *	[31:26] Always 0
 *	[25:24] Access Mode
 *	[23:16] Location of target Byte in EXT_CSD
 *	[15:08] Value Byte
 *	[07:03] Always 0
 *	[02:00] Command Set
 */

/*
  MMC status in R1, for native mode (SPI bits are different)
  Type
	e : error bit
	s : status bit
	r : detected and set for the actual command response
	x : detected and set during command execution. the host must poll
            the card by sending status command in order to read these bits.
  Clear condition
	a : according to the card state
	b : always related to the previous command. Reception of
            a valid command will clear it (with a delay of one command)
	c : clear by read
 */

#define R1_OUT_OF_RANGE		(1 << 31)	/* er, c */
#define R1_ADDRESS_ERROR	(1 << 30)	/* erx, c */
#define R1_BLOCK_LEN_ERROR	(1 << 29)	/* er, c */
#define R1_ERASE_SEQ_ERROR      (1 << 28)	/* er, c */
#define R1_ERASE_PARAM		(1 << 27)	/* ex, c */
#define R1_WP_VIOLATION		(1 << 26)	/* erx, c */
#define R1_CARD_IS_LOCKED	(1 << 25)	/* sx, a */
#define R1_LOCK_UNLOCK_FAILED	(1 << 24)	/* erx, c */
#define R1_COM_CRC_ERROR	(1 << 23)	/* er, b */
#define R1_ILLEGAL_COMMAND	(1 << 22)	/* er, b */
#define R1_CARD_ECC_FAILED	(1 << 21)	/* ex, c */
#define R1_CC_ERROR		(1 << 20)	/* erx, c */
#define R1_ERROR		(1 << 19)	/* erx, c */
#define R1_UNDERRUN		(1 << 18)	/* ex, c */
#define R1_OVERRUN		(1 << 17)	/* ex, c */
#define R1_CID_CSD_OVERWRITE	(1 << 16)	/* erx, c, CID/CSD overwrite */
#define R1_WP_ERASE_SKIP	(1 << 15)	/* sx, c */
#define R1_CARD_ECC_DISABLED	(1 << 14)	/* sx, a */
#define R1_ERASE_RESET		(1 << 13)	/* sr, c */
#define R1_STATUS(x)            (x & 0xFFFFE000)
#define R1_CURRENT_STATE(x)	((x & 0x00001E00) >> 9)	/* sx, b (4 bits) */
#define R1_READY_FOR_DATA	(1 << 8)	/* sx, a */
#define R1_SWITCH_ERROR		(1 << 7)	/* sx, c */
#define R1_EXCEPTION_EVENT	(1 << 6)	/* sr, a */
#define R1_APP_CMD		(1 << 5)	/* sr, c */

#define R1_STATE_IDLE	0
#define R1_STATE_READY	1
#define R1_STATE_IDENT	2
#define R1_STATE_STBY	3
#define R1_STATE_TRAN	4
#define R1_STATE_DATA	5
#define R1_STATE_RCV	6
#define R1_STATE_PRG	7
#define R1_STATE_DIS	8

/*
 * MMC/SD in SPI mode reports R1 status always, and R2 for SEND_STATUS
 * R1 is the low order byte; R2 is the next highest byte, when present.
 */
#define R1_SPI_IDLE		(1 << 0)
#define R1_SPI_ERASE_RESET	(1 << 1)
#define R1_SPI_ILLEGAL_COMMAND	(1 << 2)
#define R1_SPI_COM_CRC		(1 << 3)
#define R1_SPI_ERASE_SEQ	(1 << 4)
#define R1_SPI_ADDRESS		(1 << 5)
#define R1_SPI_PARAMETER	(1 << 6)
/* R1 bit 7 is always zero */
#define R2_SPI_CARD_LOCKED	(1 << 8)
#define R2_SPI_WP_ERASE_SKIP	(1 << 9)	/* or lock/unlock fail */
#define R2_SPI_LOCK_UNLOCK_FAIL	R2_SPI_WP_ERASE_SKIP
#define R2_SPI_ERROR		(1 << 10)
#define R2_SPI_CC_ERROR		(1 << 11)
#define R2_SPI_CARD_ECC_ERROR	(1 << 12)
#define R2_SPI_WP_VIOLATION	(1 << 13)
#define R2_SPI_ERASE_PARAM	(1 << 14)
#define R2_SPI_OUT_OF_RANGE	(1 << 15)	/* or CSD overwrite */
#define R2_SPI_CSD_OVERWRITE	R2_SPI_OUT_OF_RANGE

/* These are unpacked versions of the actual responses */

struct _mmc_csd {
	uint8_t  csd_structure;
	uint8_t  spec_vers;
	uint8_t  taac;
	uint8_t  nsac;
	uint8_t  tran_speed;
	u16 ccc;
	uint8_t  read_bl_len;
	uint8_t  read_bl_partial;
	uint8_t  write_blk_misalign;
	uint8_t  read_blk_misalign;
	uint8_t  dsr_imp;
	u16 c_size;
	uint8_t  vdd_r_curr_min;
	uint8_t  vdd_r_curr_max;
	uint8_t  vdd_w_curr_min;
	uint8_t  vdd_w_curr_max;
	uint8_t  c_size_mult;
	union {
		struct { /* MMC system specification version 3.1 */
			uint8_t  erase_grp_size;
			uint8_t  erase_grp_mult;
		} v31;
		struct { /* MMC system specification version 2.2 */
			uint8_t  sector_size;
			uint8_t  erase_grp_size;
		} v22;
	} erase;
	uint8_t  wp_grp_size;
	uint8_t  wp_grp_enable;
	uint8_t  default_ecc;
	uint8_t  r2w_factor;
	uint8_t  write_bl_len;
	uint8_t  write_bl_partial;
	uint8_t  file_format_grp;
	uint8_t  copy;
	uint8_t  perm_write_protect;
	uint8_t  tmp_write_protect;
	uint8_t  file_format;
	uint8_t  ecc;
};

/*
 * OCR bits are mostly in host.h
 */
#define MMC_CARD_BUSY	0x80000000	/* Card Power up status bit */

/*
 * Card Command Classes (CCC)
 */
#define CCC_BASIC		(1<<0)	/* (0) Basic protocol functions */
					/* (CMD0,1,2,3,4,7,9,10,12,13,15) */
					/* (and for SPI, CMD58,59) */
#define CCC_STREAM_READ		(1<<1)	/* (1) Stream read commands */
					/* (CMD11) */
#define CCC_BLOCK_READ		(1<<2)	/* (2) Block read commands */
					/* (CMD16,17,18) */
#define CCC_STREAM_WRITE	(1<<3)	/* (3) Stream write commands */
					/* (CMD20) */
#define CCC_BLOCK_WRITE		(1<<4)	/* (4) Block write commands */
					/* (CMD16,24,25,26,27) */
#define CCC_ERASE		(1<<5)	/* (5) Ability to erase blocks */
					/* (CMD32,33,34,35,36,37,38,39) */
#define CCC_WRITE_PROT		(1<<6)	/* (6) Able to write protect blocks */
					/* (CMD28,29,30) */
#define CCC_LOCK_CARD		(1<<7)	/* (7) Able to lock down card */
					/* (CMD16,CMD42) */
#define CCC_APP_SPEC		(1<<8)	/* (8) Application specific */
					/* (CMD55,56,57,ACMD*) */
#define CCC_IO_MODE		(1<<9)	/* (9) I/O mode */
					/* (CMD5,39,40,52,53) */
#define CCC_SWITCH		(1<<10)	/* (10) High speed switch */
					/* (CMD6,34,35,36,37,50) */
					/* (11) Reserved */
					/* (CMD?) */

/*
 * CSD field definitions
 */

#define CSD_STRUCT_VER_1_0  0           /* Valid for system specification 1.0 - 1.2 */
#define CSD_STRUCT_VER_1_1  1           /* Valid for system specification 1.4 - 2.2 */
#define CSD_STRUCT_VER_1_2  2           /* Valid for system specification 3.1 - 3.2 - 3.31 - 4.0 - 4.1 */
#define CSD_STRUCT_EXT_CSD  3           /* Version is coded in CSD_STRUCTURE in EXT_CSD */

#define CSD_SPEC_VER_0      0           /* Implements system specification 1.0 - 1.2 */
#define CSD_SPEC_VER_1      1           /* Implements system specification 1.4 */
#define CSD_SPEC_VER_2      2           /* Implements system specification 2.0 - 2.2 */
#define CSD_SPEC_VER_3      3           /* Implements system specification 3.1 - 3.2 - 3.31 */
#define CSD_SPEC_VER_4      4           /* Implements system specification 4.0 - 4.1 */

/*
 * EXT_CSD fields
 */

#define EXT_CSD_FLUSH_CACHE		32      /* W */
#define EXT_CSD_CACHE_CTRL		33      /* R/W */
#define EXT_CSD_POWER_OFF_NOTIFICATION	34	/* R/W */
#define EXT_CSD_PACKED_FAILURE_INDEX	35	/* RO */
#define EXT_CSD_PACKED_CMD_STATUS	36	/* RO */
#define EXT_CSD_EXP_EVENTS_STATUS	54	/* RO, 2 bytes */
#define EXT_CSD_EXP_EVENTS_CTRL		56	/* R/W, 2 bytes */
#define EXT_CSD_DATA_SECTOR_SIZE	61	/* R */
#define EXT_CSD_GP_SIZE_MULT		143	/* R/W */
#define EXT_CSD_PARTITION_SETTING_COMPLETED 155	/* R/W */
#define EXT_CSD_PARTITION_ATTRIBUTE	156	/* R/W */
#define EXT_CSD_PARTITION_SUPPORT	160	/* RO */
#define EXT_CSD_HPI_MGMT		161	/* R/W */
#define EXT_CSD_RST_N_FUNCTION		162	/* R/W */
#define EXT_CSD_BKOPS_EN		163	/* R/W */
#define EXT_CSD_BKOPS_START		164	/* W */
#define EXT_CSD_SANITIZE_START		165     /* W */
#define EXT_CSD_WR_REL_PARAM		166	/* RO */
#define EXT_CSD_RPMB_MULT		168	/* RO */
#define EXT_CSD_FW_CONFIG		169	/* R/W */
#define EXT_CSD_BOOT_WP			173	/* R/W */
#define EXT_CSD_ERASE_GROUP_DEF		175	/* R/W */
#define EXT_CSD_PART_CONFIG		179	/* R/W */
#define EXT_CSD_ERASED_MEM_CONT		181	/* RO */
#define EXT_CSD_BUS_WIDTH		183	/* R/W */
#define EXT_CSD_STROBE_SUPPORT		184	/* RO */
#define EXT_CSD_HS_TIMING		185	/* R/W */
#define EXT_CSD_POWER_CLASS		187	/* R/W */
#define EXT_CSD_REV			192	/* RO */
#define EXT_CSD_STRUCTURE		194	/* RO */
#define EXT_CSD_CARD_TYPE		196	/* RO */
#define EXT_CSD_DRIVER_STRENGTH		197	/* RO */
#define EXT_CSD_OUT_OF_INTERRUPT_TIME	198	/* RO */
#define EXT_CSD_PART_SWITCH_TIME        199     /* RO */
#define EXT_CSD_PWR_CL_52_195		200	/* RO */
#define EXT_CSD_PWR_CL_26_195		201	/* RO */
#define EXT_CSD_PWR_CL_52_360		202	/* RO */
#define EXT_CSD_PWR_CL_26_360		203	/* RO */
#define EXT_CSD_SEC_CNT			212	/* RO, 4 bytes */
#define EXT_CSD_S_A_TIMEOUT		217	/* RO */
#define EXT_CSD_REL_WR_SEC_C		222	/* RO */
#define EXT_CSD_HC_WP_GRP_SIZE		221	/* RO */
#define EXT_CSD_ERASE_TIMEOUT_MULT	223	/* RO */
#define EXT_CSD_HC_ERASE_GRP_SIZE	224	/* RO */
#define EXT_CSD_BOOT_MULT		226	/* RO */
#define EXT_CSD_SEC_TRIM_MULT		229	/* RO */
#define EXT_CSD_SEC_ERASE_MULT		230	/* RO */
#define EXT_CSD_SEC_FEATURE_SUPPORT	231	/* RO */
#define EXT_CSD_TRIM_MULT		232	/* RO */
#define EXT_CSD_PWR_CL_200_195		236	/* RO */
#define EXT_CSD_PWR_CL_200_360		237	/* RO */
#define EXT_CSD_PWR_CL_DDR_52_195	238	/* RO */
#define EXT_CSD_PWR_CL_DDR_52_360	239	/* RO */
#define EXT_CSD_BKOPS_STATUS		246	/* RO */
#define EXT_CSD_POWER_OFF_LONG_TIME	247	/* RO */
#define EXT_CSD_GENERIC_CMD6_TIME	248	/* RO */
#define EXT_CSD_CACHE_SIZE		249	/* RO, 4 bytes */
#define EXT_CSD_PWR_CL_DDR_200_360	253	/* RO */
#define EXT_CSD_FIRMWARE_VERSION	254	/* RO, 8 bytes */
#define EXT_CSD_SUPPORTED_MODE		493	/* RO */
#define EXT_CSD_TAG_UNIT_SIZE		498	/* RO */
#define EXT_CSD_DATA_TAG_SUPPORT	499	/* RO */
#define EXT_CSD_MAX_PACKED_WRITES	500	/* RO */
#define EXT_CSD_MAX_PACKED_READS	501	/* RO */
#define EXT_CSD_BKOPS_SUPPORT		502	/* RO */
#define EXT_CSD_HPI_FEATURES		503	/* RO */

/*
 * EXT_CSD field definitions
 */

#define EXT_CSD_WR_REL_PARAM_EN		(1<<2)

#define EXT_CSD_BOOT_WP_B_PWR_WP_DIS	(0x40)
#define EXT_CSD_BOOT_WP_B_PERM_WP_DIS	(0x10)
#define EXT_CSD_BOOT_WP_B_PERM_WP_EN	(0x04)
#define EXT_CSD_BOOT_WP_B_PWR_WP_EN	(0x01)

#define EXT_CSD_PART_CONFIG_ACC_MASK	(0x7)
#define EXT_CSD_PART_CONFIG_ACC_BOOT0	(0x1)
#define EXT_CSD_PART_CONFIG_ACC_RPMB	(0x3)
#define EXT_CSD_PART_CONFIG_ACC_GP0	(0x4)

#define EXT_CSD_PART_SETTING_COMPLETED	(0x1)
#define EXT_CSD_PART_SUPPORT_PART_EN	(0x1)

#define EXT_CSD_CMD_SET_NORMAL		(1<<0)
#define EXT_CSD_CMD_SET_SECURE		(1<<1)
#define EXT_CSD_CMD_SET_CPSECURE	(1<<2)

#define EXT_CSD_CARD_TYPE_HS_26	(1<<0)	/* Card can run at 26MHz */
#define EXT_CSD_CARD_TYPE_HS_52	(1<<1)	/* Card can run at 52MHz */
#define EXT_CSD_CARD_TYPE_HS	(EXT_CSD_CARD_TYPE_HS_26 | \
				 EXT_CSD_CARD_TYPE_HS_52)
#define EXT_CSD_CARD_TYPE_DDR_1_8V  (1<<2)   /* Card can run at 52MHz */
					     /* DDR mode @1.8V or 3V I/O */
#define EXT_CSD_CARD_TYPE_DDR_1_2V  (1<<3)   /* Card can run at 52MHz */
					     /* DDR mode @1.2V I/O */
#define EXT_CSD_CARD_TYPE_DDR_52       (EXT_CSD_CARD_TYPE_DDR_1_8V  \
					| EXT_CSD_CARD_TYPE_DDR_1_2V)
#define EXT_CSD_CARD_TYPE_HS200_1_8V	(1<<4)	/* Card can run at 200MHz */
#define EXT_CSD_CARD_TYPE_HS200_1_2V	(1<<5)	/* Card can run at 200MHz */
						/* SDR mode @1.2V I/O */
#define EXT_CSD_CARD_TYPE_HS200		(EXT_CSD_CARD_TYPE_HS200_1_8V | \
					 EXT_CSD_CARD_TYPE_HS200_1_2V)
#define EXT_CSD_CARD_TYPE_HS400_1_8V	(1<<6)	/* Card can run at 200MHz DDR, 1.8V */
#define EXT_CSD_CARD_TYPE_HS400_1_2V	(1<<7)	/* Card can run at 200MHz DDR, 1.2V */
#define EXT_CSD_CARD_TYPE_HS400		(EXT_CSD_CARD_TYPE_HS400_1_8V | \
					 EXT_CSD_CARD_TYPE_HS400_1_2V)
#define EXT_CSD_CARD_TYPE_HS400ES	(1<<8)	/* Card can run at HS400ES */

#define EXT_CSD_BUS_WIDTH_1	0	/* Card is in 1 bit mode */
#define EXT_CSD_BUS_WIDTH_4	1	/* Card is in 4 bit mode */
#define EXT_CSD_BUS_WIDTH_8	2	/* Card is in 8 bit mode */
#define EXT_CSD_DDR_BUS_WIDTH_4	5	/* Card is in 4 bit DDR mode */
#define EXT_CSD_DDR_BUS_WIDTH_8	6	/* Card is in 8 bit DDR mode */
#define EXT_CSD_BUS_WIDTH_STROBE BIT(7)	/* Enhanced strobe mode */

#define EXT_CSD_TIMING_BC	0	/* Backwards compatility */
#define EXT_CSD_TIMING_HS	1	/* High speed */
#define EXT_CSD_TIMING_HS200	2	/* HS200 */
#define EXT_CSD_TIMING_HS400	3	/* HS400 */
#define EXT_CSD_DRV_STR_SHIFT	4	/* Driver Strength shift */

#define EXT_CSD_SEC_ER_EN	BIT(0)
#define EXT_CSD_SEC_BD_BLK_EN	BIT(2)
#define EXT_CSD_SEC_GB_CL_EN	BIT(4)
#define EXT_CSD_SEC_SANITIZE	BIT(6)  /* v4.5 only */

#define EXT_CSD_RST_N_EN_MASK	0x3
#define EXT_CSD_RST_N_ENABLED	1	/* RST_n is enabled on card */

#define EXT_CSD_NO_POWER_NOTIFICATION	0
#define EXT_CSD_POWER_ON		1
#define EXT_CSD_POWER_OFF_SHORT		2
#define EXT_CSD_POWER_OFF_LONG		3

#define EXT_CSD_PWR_CL_8BIT_MASK	0xF0	/* 8 bit PWR CLS */
#define EXT_CSD_PWR_CL_4BIT_MASK	0x0F	/* 8 bit PWR CLS */
#define EXT_CSD_PWR_CL_8BIT_SHIFT	4
#define EXT_CSD_PWR_CL_4BIT_SHIFT	0

#define EXT_CSD_PACKED_EVENT_EN	BIT(3)

/*
 * EXCEPTION_EVENT_STATUS field
 */
#define EXT_CSD_URGENT_BKOPS		BIT(0)
#define EXT_CSD_DYNCAP_NEEDED		BIT(1)
#define EXT_CSD_SYSPOOL_EXHAUSTED	BIT(2)
#define EXT_CSD_PACKED_FAILURE		BIT(3)

#define EXT_CSD_PACKED_GENERIC_ERROR	BIT(0)
#define EXT_CSD_PACKED_INDEXED_ERROR	BIT(1)

/*
 * BKOPS status level
 */
#define EXT_CSD_BKOPS_LEVEL_2		0x2

/*
 * BKOPS modes
 */
#define EXT_CSD_MANUAL_BKOPS_MASK	0x01

/*
 * MMC_SWITCH access modes
 */

#define MMC_SWITCH_MODE_CMD_SET		0x00	/* Change the command set */
#define MMC_SWITCH_MODE_SET_BITS	0x01	/* Set bits which are 1 in value */
#define MMC_SWITCH_MODE_CLEAR_BITS	0x02	/* Clear bits which are 1 in value */
#define MMC_SWITCH_MODE_WRITE_BYTE	0x03	/* Set target to value */

#define mmc_driver_type_mask(n)		(1 << (n))

static inline int mmc_card_hs(struct mmc_card *card)
{
	return card->host->ios.timing == MMC_TIMING_SD_HS ||
		card->host->ios.timing == MMC_TIMING_MMC_HS;
}

static inline int mmc_card_uhs(struct mmc_card *card)
{
	return card->host->ios.timing >= MMC_TIMING_UHS_SDR12 &&
		card->host->ios.timing <= MMC_TIMING_UHS_DDR50;
}

static inline int mmc_card_hs200(struct mmc_card *card)
{
	return card->host->ios.timing == MMC_TIMING_MMC_HS200;
}

static inline int mmc_card_ddr52(struct mmc_card *card)
{
	return card->host->ios.timing == MMC_TIMING_MMC_DDR52;
}

static inline int mmc_card_hs400(struct mmc_card *card)
{
	return card->host->ios.timing == MMC_TIMING_MMC_HS400;
}



#endif  /* __SD_H__ */
