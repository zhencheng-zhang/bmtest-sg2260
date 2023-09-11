/*
 * Copyright (c) 2016-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __BM_SD_H__
#define __BM_SD_H__

#include <strings.h>
#include "system_common.h"
#include "utils_def.h"
#include "timer.h"

#include "mmc.h"

#define MMC_TEST_DEBUG
#ifdef MMC_TEST_DEBUG
#define pr_debug	printf
#else
#define pr_debug(x, args...)
#endif

/*#define SDIO_INTR			(46-32) // cxk add*/
#define SDIO_INTR			136
#define SD0_BASE 			0x29310000
#define SD1_BASE			0x29320000
#define SD2_BASE			0x29330000

/*#define EMMC_BASE			0x29300000 */ // cxk del
#if defined(TEST_SD0)
#define SDIO_BASE			SD0_BASE
#elif defined(TEST_SD1)
#define SDIO_BASE			SD1_BASE
#elif defined(TEST_SD2)
#define SDIO_BASE			SD2_BASE
#endif

struct bm_mmc_params {
	uintptr_t       reg_base;
	unsigned int	f_min;
	unsigned int	f_max;

#define MMC_FLAG_CMD23			(1 << 0)
#define MMC_FLAG_HOST_RESET		(1 << 1)
#define MMC_FLAG_HAVE_PHY		(1 << 2)
#define MMC_FLAG_INIT_PHY		(1 << 3)
#define MMC_FLAG_PIO_TRANS		(1 << 4)
#define MMC_FLAG_SDMA_TRANS		(1 << 5)
#define MMC_FLAG_ADMA2_TRANS	(1 << 6)
#define MMC_FLAG_ADMA3_TRANS	(1 << 7)
#define MMC_FLAG_SUPPORT_IRQ	(1 << 8)
#define MMC_FLAG_SD_DDR50			(1 << 9)
#define MMC_FLAG_SD_SDR104			(1 << 10)
#define MMC_FLAG_SD_SDR50			(1 << 11)
#define MMC_FLAG_SD_SDR25			(1 << 12)
#define MMC_FLAG_SD_SDR12			(1 << 13)
#define MMC_FLAG_SD_HIGHSPEED		(1 << 14)
#define MMC_FLAG_SD_DFLTSPEED		(1 << 15)
#define MMC_FLAG_BUS_WIDTH_1	(1 << 16)
#define MMC_FLAG_BUS_WIDTH_4	(1 << 17)
#define MMC_FLAG_SDCARD			(1 << 18)
#define MMC_FLAG_EMMC			(1 << 19)
#define MMC_FLAG_EMMC_HS200		(1 << 20)
#define MMC_FLAG_EMMC_DDR52		(1 << 21)
#define MMC_FLAG_EMMC_SDR52		(1 << 22)
#define MMC_FLAG_EMMC_SDR26		(1 << 23)
	unsigned long long	flags;
	unsigned long force_trans_freq;

};

void bm_mmc_phy_init(struct mmc_host *host);
void bm_mmc_hw_reset(struct mmc_host *host); // add cxk
int bm_mmc_setup_host(struct bm_mmc_params *param, struct mmc_host *host);
int bm_sd_card_detect(struct mmc_host *host);
int bm_sd_card_lock(struct mmc_host *host);

#define MMC_INIT_FREQ					(25 * 1000 * 1000)
#define SDCARD_TRAN_FREQ				(12 * 1000 * 1000)

#define SDHCI_DMA_ADDRESS               0x00
#define SDHCI_BLOCK_SIZE                0x04
#define SDHCI_MAKE_BLKSZ(dma, blksz)    ((((dma) & 0x7) << 12) | ((blksz) & 0xFFF))
#define SDHCI_BLOCK_COUNT               0x06
#define SDHCI_ARGUMENT                  0x08
#define SDHCI_TRANSFER_MODE             0x0C
#define SDHCI_TRNS_DMA                  BIT(0)
#define SDHCI_TRNS_BLK_CNT_EN           BIT(1)
#define SDHCI_TRNS_ACMD12               BIT(2)
#define SDHCI_TRNS_READ                 BIT(4)
#define SDHCI_TRNS_MULTI                BIT(5)
#define SDHCI_TRNS_RESP_INT             BIT(8)
#define SDHCI_COMMAND                   0x0E
#define SDHCI_CMD_RESP_MASK             0x03
#define SDHCI_CMD_CRC                   0x08
#define SDHCI_CMD_INDEX                 0x10
#define SDHCI_CMD_DATA                  0x20
#define SDHCI_CMD_ABORTCMD              0xC0
#define SDHCI_CMD_RESP_NONE             0x00
#define SDHCI_CMD_RESP_LONG             0x01
#define SDHCI_CMD_RESP_SHORT            0x02
#define SDHCI_CMD_RESP_SHORT_BUSY       0x03
#define SDHCI_MAKE_CMD(c, f)            ((((c) & 0xff) << 8) | ((f) & 0xff))
#define SDHCI_RESPONSE_01               0x10
#define SDHCI_RESPONSE_23               0x14
#define SDHCI_RESPONSE_45               0x18
#define SDHCI_RESPONSE_67               0x1C

#define SDHCI_RESPONSE		0x10

#define SDHCI_BUFFER		0x20

#define SDHCI_PRESENT_STATE	0x24
#define  SDHCI_DATA_INHIBIT	0x00000002
#define  SDHCI_DOING_WRITE	0x00000100
#define  SDHCI_DOING_READ	0x00000200
#define  SDHCI_SPACE_AVAILABLE	0x00000400
#define  SDHCI_DATA_AVAILABLE	0x00000800
#define  SDHCI_CARD_PRESENT	0x00010000
#define  SDHCI_WRITE_PROTECT	0x00080000
#define  SDHCI_DATA_LVL_MASK	0x00F00000
#define   SDHCI_DATA_LVL_SHIFT	20
#define   SDHCI_DATA_0_LVL_MASK	0x00100000
#define  SDHCI_CMD_LVL		0x01000000

#define SDHCI_CMD_INHIBIT               BIT(0)
#define SDHCI_CMD_INHIBIT_DAT           BIT(1)
#define SDHCI_CARD_INSERTED             BIT(16)
#define SDHCI_WR_PROTECT_SW_LVL         BIT(19)
#define SDHCI_HOST_CONTROL              0x28
#define SDHCI_CTRL_HISPD                0x04
#define SDHCI_DAT_XFER_WIDTH            BIT(1)
#define SDHCI_EXT_DAT_XFER              BIT(5)
#define SDHCI_CTRL_DMA_MASK             0x18
#define SDHCI_CTRL_SDMA                 0x00
#define SDHCI_CTRL_ADMA1				0x08
#define SDHCI_CTRL_ADMA2				0x10
#define SDHCI_CTRL_ADMA3				0x18
#define SDHCI_PWR_CONTROL               0x29
#define SDHCI_BUS_VOL_VDD1_1_8V         0xC
#define SDHCI_BUS_VOL_VDD1_3_0V         0xE
#define SDHCI_BUF_DATA_R                0x20
#define SDHCI_BLOCK_GAP_CONTROL         0x2A
#define SDHCI_CLK_CTRL                  0x2C
#define SDHCI_TOUT_CTRL                 0x2E
#define SDHCI_SOFTWARE_RESET            0x2F
#define SDHCI_RESET_CMD                 0x02
#define SDHCI_RESET_DATA                0x04
#define SDHCI_INT_STATUS                0x30
#define SDHCI_ERR_INT_STATUS            0x32
#define SDHCI_INT_XFER_COMPLETE         BIT(1)
#define SDHCI_INT_BUF_RD_READY          BIT(5)
#define SDHCI_INT_STATUS_EN             0x34
#define SDHCI_ERR_INT_STATUS_EN         0x36
#define SDHCI_INT_XFER_COMPLETE_EN      BIT(1)
#define SDHCI_INT_DMA_END_EN            BIT(3)
#define SDHCI_INT_ERROR_EN              BIT(15)
#define SDHCI_HOST_CONTROL2             0x3E
#define SDHCI_HOST_ADMA2_LEN_MODE       BIT(10)
#define SDHCI_CTRL_UHS_MASK        0x0007
#define SDHCI_CTRL_UHS_SDR12      0x0000
#define SDHCI_CTRL_UHS_SDR25      0x0001
#define SDHCI_CTRL_UHS_SDR50      0x0002
#define SDHCI_CTRL_UHS_SDR104     0x0003
#define SDHCI_CTRL_UHS_DDR50      0x0004
#define SDHCI_CTRL_HS400      0x0005 /* Non-standard */
#define SDHCI_CTRL_VDD_180     0x0008
#define SDHCI_CTRL_DRV_TYPE_MASK   0x0030
#define SDHCI_CTRL_DRV_TYPE_B     0x0000
#define SDHCI_CTRL_DRV_TYPE_A     0x0010
#define SDHCI_CTRL_DRV_TYPE_C     0x0020
#define SDHCI_CTRL_DRV_TYPE_D     0x0030
#define SDHCI_CTRL_EXEC_TUNING     0x0040
#define SDHCI_CTRL_TUNED_CLK       0x0080
#define SDHCI_CTRL_PRESET_VAL_ENABLE   0x8000

#define SDHCI_GET_CMD(c) ((c>>8) & 0x3f)

#define SDHCI_SIGNAL_ENABLE	0x38
#define SDHCI_ERROR_SIGNAL_ENABLE	0x3a
#define  SDHCI_INT_RESPONSE	0x00000001
#define  SDHCI_INT_DATA_END	0x00000002
#define  SDHCI_INT_BLK_GAP	0x00000004
#define  SDHCI_INT_DMA_END	0x00000008
#define  SDHCI_INT_SPACE_AVAIL	0x00000010
#define  SDHCI_INT_DATA_AVAIL	0x00000020
#define  SDHCI_INT_CARD_INSERT	0x00000040
#define  SDHCI_INT_CARD_REMOVE	0x00000080
#define  SDHCI_INT_CARD_INT	0x00000100
#define  SDHCI_INT_RETUNE	0x00001000
#define  SDHCI_INT_ERROR	0x00008000
#define  SDHCI_INT_TIMEOUT	0x00010000
#define  SDHCI_INT_CRC		0x00020000
#define  SDHCI_INT_END_BIT	0x00040000
#define  SDHCI_INT_INDEX	0x00080000
#define  SDHCI_INT_DATA_TIMEOUT	0x00100000
#define  SDHCI_INT_DATA_CRC	0x00200000
#define  SDHCI_INT_DATA_END_BIT	0x00400000
#define  SDHCI_INT_BUS_POWER	0x00800000
#define  SDHCI_INT_ACMD12ERR	0x01000000
#define  SDHCI_INT_ADMA_ERROR	0x02000000

#define  SDHCI_INT_NORMAL_MASK	0x00007FFF
#define  SDHCI_INT_ERROR_MASK	0xFFFF8000

#define  SDHCI_INT_CMD_MASK	(SDHCI_INT_RESPONSE | SDHCI_INT_TIMEOUT | \
		SDHCI_INT_CRC | SDHCI_INT_END_BIT | SDHCI_INT_INDEX)
#define  SDHCI_INT_DATA_MASK	(SDHCI_INT_DATA_END | SDHCI_INT_DMA_END | \
		SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL | \
		SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_DATA_CRC | \
		SDHCI_INT_DATA_END_BIT | SDHCI_INT_ADMA_ERROR | \
		SDHCI_INT_BLK_GAP)
#define SDHCI_INT_ALL_MASK	((unsigned int)-1)

#define SDHCI_ADMA_ID_LOW_R	0x78
#define SDHCI_ADMA_ID_HIGH_R 0x7c

#define SDHCI_CTRL_VDD_180              0x0008
#define SDHCI_HOST_VER4_ENABLE          BIT(12)
#define SDHCI_CAPABILITIES1             0x40
#define SDHCI_CAPABILITIES2             0x44
#define SDHCI_ADMA_SA_LOW               0x58
#define SDHCI_ADMA_SA_HIGH              0x5C
#define SDHCI_HOST_CNTRL_VERS			0xFE
#define SDHCI_UHS_2_TIMER_CNTRL			0xC2

#define P_VENDOR_SPECIFIC_AREA          0xE8
#define P_VENDOR2_SPECIFIC_AREA         0xEA
#define VENDOR_SD_CTRL                0x2C

#define SDHCI_PHY_R_OFFSET			0x300

#define SDHCI_P_PHY_CNFG			(SDHCI_PHY_R_OFFSET + 0x00)
#define SDHCI_P_CMDPAD_CNFG			(SDHCI_PHY_R_OFFSET + 0x04)
#define SDHCI_P_DATPAD_CNFG			(SDHCI_PHY_R_OFFSET + 0x06)
#define SDHCI_P_CLKPAD_CNFG			(SDHCI_PHY_R_OFFSET + 0x08)
#define SDHCI_P_STBPAD_CNFG			(SDHCI_PHY_R_OFFSET + 0x0A)
#define SDHCI_P_RSTNPAD_CNFG		(SDHCI_PHY_R_OFFSET + 0x0C)
#define SDHCI_P_PADTEST_CNFG		(SDHCI_PHY_R_OFFSET + 0x0E)
#define SDHCI_P_PADTEST_OUT 		(SDHCI_PHY_R_OFFSET + 0x10)
#define SDHCI_P_PADTEST_IN	 		(SDHCI_PHY_R_OFFSET + 0x12)
#define SDHCI_P_COMMDL_CNFG 		(SDHCI_PHY_R_OFFSET + 0x1C)
#define SDHCI_P_SDCLKDL_CNFG 		(SDHCI_PHY_R_OFFSET + 0x1D)
#define SDHCI_P_SDCLKDL_DC	 		(SDHCI_PHY_R_OFFSET + 0x1E)
#define SDHCI_P_SMPLDL_CNFG	 		(SDHCI_PHY_R_OFFSET + 0x20)
#define SDHCI_P_ATDL_CNFG	 		(SDHCI_PHY_R_OFFSET + 0x21)
#define SDHCI_P_DLL_CTRL	 		(SDHCI_PHY_R_OFFSET + 0x24)
#define SDHCI_P_DLL_CNFG1	 		(SDHCI_PHY_R_OFFSET + 0x25)
#define SDHCI_P_DLL_CNFG2	 		(SDHCI_PHY_R_OFFSET + 0x26)
#define SDHCI_P_DLLDL_CNFG	 		(SDHCI_PHY_R_OFFSET + 0x28)
#define SDHCI_P_DLL_OFFST	 		(SDHCI_PHY_R_OFFSET + 0x29)
#define SDHCI_P_DLLMST_TSTDC 		(SDHCI_PHY_R_OFFSET + 0x2A)
#define SDHCI_P_DLLLBT_CNFG	 		(SDHCI_PHY_R_OFFSET + 0x2C)
#define SDHCI_P_DLL_STATUS	 		(SDHCI_PHY_R_OFFSET + 0x2E)
#define SDHCI_P_DLLDBG_MLKDC 		(SDHCI_PHY_R_OFFSET + 0x30)
#define SDHCI_P_DLLDBG_SLKDC 		(SDHCI_PHY_R_OFFSET + 0x32)

#define PHY_CNFG_PHY_RSTN				0
#define PHY_CNFG_PHY_PWRGOOD			1
#define PHY_CNFG_PAD_SP					16
#define PHY_CNFG_PAD_SP_MSK				0xf
#define PHY_CNFG_PAD_SN					20
#define PHY_CNFG_PAD_SN_MSK				0xf

#define PAD_CNFG_RXSEL					0
#define PAD_CNFG_RXSEL_MSK				0x7
#define PAD_CNFG_WEAKPULL_EN			3
#define PAD_CNFG_WEAKPULL_EN_MSK		0x3
#define PAD_CNFG_TXSLEW_CTRL_P			5
#define PAD_CNFG_TXSLEW_CTRL_P_MSK		0xf
#define PAD_CNFG_TXSLEW_CTRL_N			9
#define PAD_CNFG_TXSLEW_CTRL_N_MSK		0xf

#define COMMDL_CNFG_DLSTEP_SEL			0
#define COMMDL_CNFG_DLOUT_EN			1

#define SDCLKDL_CNFG_EXTDLY_EN			0
#define SDCLKDL_CNFG_BYPASS_EN			1
#define SDCLKDL_CNFG_INPSEL_CNFG		2
#define SDCLKDL_CNFG_INPSEL_CNFG_MSK	0x3
#define SDCLKDL_CNFG_UPDATE_DC			4

#define SMPLDL_CNFG_EXTDLY_EN			0
#define SMPLDL_CNFG_BYPASS_EN			1
#define SMPLDL_CNFG_INPSEL_CNFG			2
#define SMPLDL_CNFG_INPSEL_CNFG_MSK		0x3
#define SMPLDL_CNFG_INPSEL_OVERRIDE		4

#define ATDL_CNFG_EXTDLY_EN				0
#define ATDL_CNFG_BYPASS_EN				1
#define ATDL_CNFG_INPSEL_CNFG			2
#define ATDL_CNFG_INPSEL_CNFG_MSK		0x3

#endif	/* __BM_SD_H__ */
