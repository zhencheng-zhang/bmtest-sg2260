#ifndef _TESTCASE_SPI_H_
#define _TESTCASE_SPI_H_

#include "sg2260_common.h"
#include "asm/encoding.h"

#define SPI_BASE         DW_SPI0_BASE

// #define CONFIG_USE_SPI_DMA
//#define CONFIG_TEST_WITH_ADAU1372

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define SPI_CS_TOG_EN	0x030001D0
#define DMA_REMAP_UPDATE	0x1

/* Register offsets */
#define DW_SPI_CTRL0			0x00
#define DW_SPI_CTRL1			0x04
#define DW_SPI_SSIENR			0x08
#define DW_SPI_MWCR			0x0c
#define DW_SPI_SER			0x10
#define DW_SPI_BAUDR			0x14
#define DW_SPI_TXFLTR			0x18
#define DW_SPI_RXFLTR			0x1c
#define DW_SPI_TXFLR			0x20
#define DW_SPI_RXFLR			0x24
#define DW_SPI_SR			0x28
#define DW_SPI_IMR			0x2c
#define DW_SPI_ISR			0x30
#define DW_SPI_RISR			0x34
#define DW_SPI_TXOICR			0x38
#define DW_SPI_RXOICR			0x3c
#define DW_SPI_RXUICR			0x40
#define DW_SPI_MSTICR			0x44
#define DW_SPI_ICR			0x48
#define DW_SPI_DMACR			0x4c
#define DW_SPI_DMATDLR			0x50
#define DW_SPI_DMARDLR			0x54
#define DW_SPI_IDR			0x58
#define DW_SPI_VERSION			0x5c
#define DW_SPI_DR			0x60
#define DW_SPI_CTRL2		0xF4
#define DW_SPI_CS_CTRL2		0xF8

/* Bit fields in ISR, IMR, RISR, 7 bits */
#define DW_SPI_INT_MASK				((1<<7)- 1)
#define DW_SPI_INT_TXEI				BIT(0)
#define DW_SPI_INT_TXOI				BIT(1)
#define DW_SPI_INT_RXUI				BIT(2)
#define DW_SPI_INT_RXOI				BIT(3)
#define DW_SPI_INT_RXFI				BIT(4)
#define DW_SPI_INT_MSTI				BIT(5)

/* Bit fields in CTRLR1 */
#define DW_SPI_NDF_MASK				((1<<17) -1)

/* Bit fields in SR, 7 bits */
#define DW_SPI_SR_MASK				((1<<8) -1)
#define DW_SPI_SR_BUSY				BIT(0)
#define DW_SPI_SR_TF_NOT_FULL			BIT(1)
#define DW_SPI_SR_TF_EMPT			BIT(2)
#define DW_SPI_SR_RF_NOT_EMPT			BIT(3)
#define DW_SPI_SR_RF_FULL			BIT(4)
#define DW_SPI_SR_TX_ERR			BIT(5)
#define DW_SPI_SR_DCOL				BIT(6)

/* CTRL0 filed*/
#define SPI_TMOD_MASK		(0x3 << 8)
#define	SPI_TMOD_TR			0x0000		/* xmit & recv */
#define SPI_TMOD_TO			0x0100		/* xmit only */
#define SPI_TMOD_RO			0x0200		/* recv only */
#define SPI_EERPOMREAD      0x0300      /* EEPROM READ */
#define SPI_FRF_MASK        0x0030
#define SPI_SCPOL_MASK      0x0080
#define SPI_SCPOL_LOW       0x0000
#define SPI_SCPOL_HIGH      0x0080
#define SPI_SCPH_MASK       0x0040
#define SPI_SCPH_MIDDLE     0x0000
#define SPI_SCPH_START      0x0040
#define SPI_FRF_SPI			0x0
#define SPI_DFS_MASK        0x000F
#define SPI_DFS_8BIT        0x0007
#define SPI_DFS_16BIT       0x000F


/* RISR filed */
#define SPI_RISR_RXFIR      0x10
#define SPI_RISR_RXOIR      0x08
#define SPI_RISR_RXUIR      0x04
#define SPI_RISR_TXOIR      0x02
#define SPI_RISR_TXEIR      0x01

/* DMACR field */
#define SPI_DMA_TDMAE_ON    0x01
#define SPI_DMA_TDMAE_OFF   0x00
#define SPI_DMA_RDMAE_ON    0x02
#define SPI_DMA_RDMAE_OFF   0x00

/* CTRL2 filed*/
#define CVI_CS_SS_OW_EN		(0x1 << 0)
#define CVI_SS_INV_EN		(0x1 << 1)
#define CVI_LAB_FIRST		(0x1 << 2)
#define CVI_CS_TOG_OW_EN	(0x1 << 3)
#define CVI_CS_TOG_EN		(0x1 << 4)
#define CVI_SS_MD1_EN		(0x1 << 5)
#define CVI_RX_FULL_STOP_EN	(0x1 << 6)

/* CS_CTRL2 filed*/
#define CVI_SS_N_ACTIVE		(0x1)
#define CVI_SS_N_INACTIVE	(0x0)

#define SPI_BAUDR_DIV       0x32
#define DW_SPI_WAIT_RETRIES			5

#define SPI_WRITE(offset, value) \
	writel(SPI_BASE + offset , (u32)value)

#define SPI_READ(offset) \
	readl(SPI_BASE + offset)


#define DO_SPI_FW_PROG
#define DO_SPI_FW_PROG_BUF_ADDR     0x10100000 //0x45000000
#define DO_SPI_FW_PROG_BUF_SIZE     0x400 		//0x10000
// #define PLATFORM_ASIC

#ifdef PLATFORM_ASIC
#define SPI_PAGE_SIZE           256
#define SPI_SECTOR_SIZE         (SPI_PAGE_SIZE * 16)    //4KB
#define SPI_MAX_SIZE            (SPI_BLOCK1_SIZE * 256) //16MB
#elif defined(PLATFORM_FPGA) || defined(PLATFORM_FPGA_MUL)
#define SPI_PAGE_SIZE           256
#define SPI_SECTOR_SIZE         (SPI_PAGE_SIZE * 1024)
#define SPI_MAX_SIZE            (SPI_SECTOR_SIZE * 64)
#elif defined(PLATFORM_PALLADIUM)
#define SPI_PAGE_SIZE           256
#define SPI_SECTOR_SIZE         (SPI_PAGE_SIZE * 1024)
#define SPI_MAX_SIZE            (SPI_SECTOR_SIZE * 64)	//16MB
#else
#error "Undefined PLATFORM"
#endif

#define SPI_BLOCK0_SIZE         (SPI_SECTOR_SIZE * 8)   //32KB
#define SPI_BLOCK1_SIZE         (SPI_SECTOR_SIZE * 16)  //64KB
#define SPI_FLASH_SERIAL_NUMBER_START_ADDR  (SPI_MAX_SIZE - SPI_SECTOR_SIZE)
#define SPI_FLASH_CONV_RESULT_START_ADDR    (SPI_MAX_SIZE - SPI_BLOCK1_SIZE * 4 - SPI_SECTOR_SIZE)

/*spi flash model*/
#define SPI_ID_M25P128          0x00182020
#define SPI_ID_N25Q128          0x0018ba20
#define SPI_ID_W25Q128FV        0x001840ef
#define SPI_ID_GD25LB512ME      0x001a67c8

/* cmd for M25P128 on FPGA */
#define SPI_CMD_WREN            0x06
#define SPI_CMD_WRDI            0x04
#define SPI_CMD_RDID            0x9F

#define SPI_CMD_RDSR            0x05    //read status reg
#define SPI_CMD_RDSR2           0x35
#define SPI_CMD_RDSR3           0x15

#define SPI_CMD_WRSR            0x01
#define SPI_CMD_WRSR2           0x31
#define SPI_CMD_WRSR3           0x11

#define SPI_CMD_READ            0x03
#define SPI_CMD_FAST_READ       0x0B
#define SPI_CMD_PP              0x02
#define SPI_CMD_SE              0xD8    //64kb block erase
#define SPI_CMD_BE              0xC7


#define SPI_STATUS_WIP          (0x01 << 0)
#define SPI_STATUS_WEL          (0x01 << 1)
#define SPI_STATUS_BP0          (0x01 << 2)
#define SPI_STATUS_BP1          (0x01 << 3)
#define SPI_STATUS_BP2          (0x01 << 4)
#define SPI_STATUS_SRWD         (0x01 << 7)

#define DW_PSSI_CTRLR0_TMOD_MASK		GENMASK(9, 8)
#define DW_SPI_CTRLR0_TMOD_TR			0x0	/* xmit & recv */
#define DW_SPI_CTRLR0_TMOD_TO			0x1	/* xmit only */
#define DW_SPI_CTRLR0_TMOD_RO			0x2	/* recv only */
#define DW_SPI_CTRLR0_TMOD_EPROMREAD		0x3	/* eeprom read mode */

enum spi_mem_data_dir {
	SPI_MEM_NO_DATA,
	SPI_MEM_DATA_IN,
	SPI_MEM_DATA_OUT,
};

#define DW_SPI_INTR		38

void spi_init(u8 n_bytes);
u64 spi_flash_map_enable(u8 enable);
int do_sector_erase(u64 spi_base, u32 addr, u32 sector_num);
int do_full_chip_erase(u64 spi_base);
//int spi_flash_program(u64 spi_base, u32 sector_addr);
u32 spi_flash_read_id(u64 spi_base);

int spi_flash_write_by_page(u64 spi_base, u32 fa, u8 *data, u32 size);
void spi_flash_read_by_page(u64 spi_base, u32 fa, u8 *data, u32 size);


int spi_flash_program(u64 spi_base, u32 sector_addr, u32 len);
int do_page_prog(u64 spi_base, u8 *src_buf, u32 addr, u32 size);
u8 spi_data_read(u64 spi_base, u8* dst_buf, u32 addr, u32 size);

#endif  /* _TESTCASE_SPI_H_ */
