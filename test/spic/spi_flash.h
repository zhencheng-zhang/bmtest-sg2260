#ifndef _SPI_FLASH_
#define _SPI_FLASH_

#include "system_common.h"

#define line()  do {uartlog("\n %s %d \n", __FILE__, __LINE__);} while(0)

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
// #define SPI_SECTOR_SIZE         (SPI_PAGE_SIZE * 1024)
#define SPI_SECTOR_SIZE         (64 * 1024)
// #define SPI_MAX_SIZE            (SPI_SECTOR_SIZE * 64)	//16MB
#define SPI_MAX_SIZE            (64 * 1024 * 1024)	//16MB
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
#define SPI_CMD_READ_4B			0x13
#define SPI_CMD_FAST_READ       0x0B
#define SPI_CMD_PP              0x02
#define SPI_CMD_PP_4B			0x12
#define SPI_CMD_BE              0xD8    //64kb block erase
#define SPI_CMD_SE              0x20
#define SPI_CMD_CE              0xC7

#define SPI_CMD_EN4B			0xB7
#define SPI_CMD_EX4B			0xE9


#define SPI_STATUS_WIP          (0x01 << 0)
#define SPI_STATUS_WEL          (0x01 << 1)
#define SPI_STATUS_BP0          (0x01 << 2)
#define SPI_STATUS_BP1          (0x01 << 3)
#define SPI_STATUS_BP2          (0x01 << 4)
#define SPI_STATUS_SRWD         (0x01 << 7)

#ifdef CONFIG_CHIP_SG2042
#define SPI0_INTR 108
#define SPI1_INTR 109
#define SOFT_RESET_SPI0_BIT 23
#define SOFT_RESET_SPI1_BIT 24
#elif CONFIG_CHIP_SG2260
#define SPI0_INTR 37
#define SPI1_INTR 38
#define SOFT_RESET_SPI0_BIT 27
#define SOFT_RESET_SPI1_BIT 28
#endif

#define SOFT_RESET_SPI_BIT SOFT_RESET_SPI0_BIT

#define CLK_EN_SPI_BIT      9

#define SPI_INTR    SPI0_INTR
#define SPI_BASE    SPI0_BASE

void writel_fence(u64 addr, u32 val);
void writew_fence(u64 addr, u16 val);
void writeb_fence(u64 addr, u8 val);

u8 spi_reg_status(u64 spi_base, u8 cmd);
u64 spi_flash_map_enable(u8 enable);

void spi_flash_init(u64 spi_base);
void spi_flash_set_dmmr_mode(u64 spi_base, u32 en);
// int spi_flash_id_check(u64 spi_base);
int do_sector_erase(u64 spi_base, u32 addr, u32 sector_num);
int do_full_chip_erase(u64 spi_base);
//int spi_flash_program(u64 spi_base, u32 sector_addr);
u32 spi_flash_read_id(u64 spi_base);

int spi_flash_write_by_page(u64 spi_base, u32 fa, u8 *data, u32 size);
void spi_flash_read_by_page(u64 spi_base, u32 fa, u8 *data, u32 size);

int spi_flash_program(u64 spi_base, u32 sector_addr, u32 len);
int do_page_prog(u64 spi_base, u8 *src_buf, u32 addr, u32 size);
u8 spi_data_read(u64 spi_base, u8* dst_buf, u32 addr, u32 size);
#endif
