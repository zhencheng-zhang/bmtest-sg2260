#ifndef __BM_SPINOR_H__
#define __BM_SPINOR_H__

/*
SPINOR IP switch:
    - SPI_NOR1 is False: SPI NOR 0
    - SPI_NOR1 is True : SPI NOR 1
*/
// #define SPI_NOR1

#ifdef SPI_NOR1
#define BASE_SPINOR	0x05400000
#define SPINOR_INT	SF1_SPI_INT
#else
#define BASE_SPINOR	0x57000000
#define SPINOR_INT	SF_SPI_INT
#endif

#define SPINOR_CMD_WREN             0x06
#define SPINOR_CMD_WRDI             0x04
#define SPINOR_CMD_RDID             0x9F
#define SPINOR_CMD_RDSR             0x05
#define SPINOR_CMD_WRSR             0x01

#define SPINOR_CMD_READ             0x03
#define SPINOR_CMD_FAST_READ        0x0B
#define SPINOR_CMD_READ_4B          0x13
#define SPINOR_CMD_READ_FRD4B       0x0C
#define SPINOR_CMD_READ_FRQIO       0xEB

#define SPINOR_CMD_SE               0x20
#define SPINOR_CMD_SE_4B            0x21
#define SPINOR_CMD_BE_32K           0x52
#define SPINOR_CMD_BE_64K           0xD8
#define SPINOR_CMD_BE_64K_4B        0xDC

#define SPINOR_CMD_PP               0x02
#define SPINOR_CMD_PP_4B            0x12
#define SPINOR_CMD_PP_4B_QIO        0x34
#define SPINOR_CMD_PP_4B_QIEF       0x3E

#define SPINOR_CMD_SFDP             0x5A

#define SPINOR_CMD_EN4B             0xB7
#define SPINOR_CMD_EX4B             0xE9

#define SPINOR_CMD_ENQD             0x35
#define SPINOR_CMD_EXQD             0xF5

#define SPINOR_STATUS_WIP          (0x01 << 0)
#define SPINOR_STATUS_WEL          (0x01 << 1)
#define SPINOR_STATUS_BP0          (0x01 << 2)
#define SPINOR_STATUS_BP1          (0x01 << 3)
#define SPINOR_STATUS_BP2          (0x01 << 4)
#define SPINOR_STATUS_SRWD         (0x01 << 7)

#define SPINOR_FLASH_BLOCK_SIZE             256
#define SPINOR_TRAN_CSR_ADDR_BYTES_SHIFT    8
#define SPINOR_MAX_FIFO_DEPTH               8

/* register definitions */
#define REG_SPINOR_CTRL                     0x00
#define REG_SPINOR_CE_CTRL                  0x04
#define REG_SPINOR_DLY_CTRL                 0x08
#define REG_SPINOR_DMMR                     0x0C
#define REG_SPINOR_TRAN_CSR                 0x10
#define REG_SPINOR_TRAN_NUM                 0x14
#define REG_SPINOR_FIFO_PORT                0x18
#define REG_SPINOR_FIFO_PT                  0x20
#define REG_SPINOR_INT_STS                  0x28
#define REG_SPINOR_INT_EN                   0x2C
#define REG_SPINOR_OPT                      0x30

/* bit definition */
#define BIT_SPINOR_CTRL_CPHA                    (0x01 << 12)
#define BIT_SPINOR_CTRL_CPOL                    (0x01 << 13)
#define BIT_SPINOR_CTRL_HOLD_OL                 (0x01 << 14)
#define BIT_SPINOR_CTRL_WP_OL                   (0x01 << 15)
#define BIT_SPINOR_CTRL_LSBF                    (0x01 << 20)
#define BIT_SPINOR_CTRL_SRST                    (0x01 << 21)
#define BIT_SPINOR_CTRL_SCK_DIV_SHIFT           0
#define BIT_SPINOR_CTRL_FRAME_LEN_SHIFT         16

#define BIT_SPINOR_CE_CTRL_CEMANUAL             (0x01 << 0)
#define BIT_SPINOR_CE_CTRL_CEMANUAL_EN          (0x01 << 1)

#define BIT_SPINOR_CTRL_FM_INTVL_SHIFT          0
#define BIT_SPINOR_CTRL_CET_SHIFT               8

#define BIT_SPINOR_DMMR_EN                      (0x01 << 0)

#define BIT_SPINOR_TRAN_CSR_TRAN_MODE_RX        (0x01 << 0)
#define BIT_SPINOR_TRAN_CSR_TRAN_MODE_TX        (0x01 << 1)
#define BIT_SPINOR_TRAN_CSR_CNTNS_READ          (0x01 << 2)
#define BIT_SPINOR_TRAN_CSR_FAST_MODE           (0x01 << 3)
#define BIT_SPINOR_TRAN_CSR_BUS_WIDTH_1_BIT     (0x0 << 4)
#define BIT_SPINOR_TRAN_CSR_BUS_WIDTH_2_BIT     (0x01 << 4)
#define BIT_SPINOR_TRAN_CSR_BUS_WIDTH_4_BIT     (0x02 << 4)
#define BIT_SPINOR_TRAN_CSR_DMA_EN              (0x01 << 6)
#define BIT_SPINOR_TRAN_CSR_MISO_LEVEL          (0x01 << 7)
#define BIT_SPINOR_TRAN_CSR_ADDR_BYTES_NO_ADDR  (0x0 << 8)
#define BIT_SPINOR_TRAN_CSR_WITH_CMD            (0x01 << 11)
#define BIT_SPINOR_TRAN_CSR_FIFO_TRG_LVL_1_BYTE (0x0 << 12)
#define BIT_SPINOR_TRAN_CSR_FIFO_TRG_LVL_2_BYTE (0x01 << 12)
#define BIT_SPINOR_TRAN_CSR_FIFO_TRG_LVL_4_BYTE (0x02 << 12)
#define BIT_SPINOR_TRAN_CSR_FIFO_TRG_LVL_8_BYTE (0x03 << 12)
#define BIT_SPINOR_TRAN_CSR_GO_BUSY             (0x01 << 15)
#define BIT_SPINOR_TRAN_CSR_DUMMY_SHIFT         (16)

#define BIT_SPINOR_TRAN_CSR_TRAN_MODE_MASK      0x3
#define BIT_SPINOR_TRAN_CSR_BUSWIDTH_MASK       0x30
#define BIT_SPINOR_TRAN_CSR_ADDR_BYTES_MASK     0x700
#define BIT_SPINOR_TRAN_CSR_FIFO_TRG_LVL_MASK   0x3000
#define BIT_SPINOR_TRAN_CSR_DUMMY_MASK          0xF0000
#define BIT_SPINOR_TRAN_CSR_4BADDR_MASK         0xF00000


#define BIT_SPINOR_INT_TRAN_DONE                (0x01 << 0)
#define BIT_SPINOR_INT_RD_FIFO                  (0x01 << 2)
#define BIT_SPINOR_INT_WR_FIFO                  (0x01 << 3)
#define BIT_SPINOR_INT_RX_FRAME                 (0x01 << 4)
#define BIT_SPINOR_INT_TX_FRAME                 (0x01 << 5)

#define BIT_SPINOR_INT_TRAN_DONE_EN             (0x01 << 0)
#define BIT_SPINOR_INT_RD_FIFO_EN               (0x01 << 2)
#define BIT_SPINOR_INT_WR_FIFO_EN               (0x01 << 3)
#define BIT_SPINOR_INT_RX_FRAME_EN              (0x01 << 4)
#define BIT_SPINOR_INT_TX_FRAME_EN              (0x01 << 5)

#define ADDR_3B             3
#define ADDR_4B             4
#define SIZE_4KB            (4*1024)
#define SIZE_64KB           (64*1024)
#define SIZE_1MB            (1*1024*1024)
#define SPI_BLOCK_SIZE      (64*1024)
#define SPI_SECTOR_SIZE     256

#define SPINOR_ID_M25P128          0x00182020
#define SPINOR_ID_N25Q128          0x0018ba20
#define SPINOR_ID_GD25LQ128        0x001860c8
#define SPINOR_ID_GD25Q128         0x001840c8
#define SPINOR_ID_MT25Q256         0x0019ba20
#define SPINOR_ID_W25Q256JV        0x001940ef
#define SPINOR_ID_W25Q512JVEIQ     0x002040ef

size_t spi_flash_read_blocks(int lba, uintptr_t buf, size_t size);
size_t spi_flash_write_blocks(int lba, const uintptr_t buf, size_t size);

void bm_spi_init(uint32_t base, uint32_t clk_div, uint32_t cpha, uint32_t cpol, uint32_t edge, uint32_t int_en);
int bm_spi_flash_program(uint8_t *src_buf, uint32_t base, uint32_t size);
uint32_t bm_spi_read_id(unsigned long spi_base);
uint32_t bm_spi_nor_sfdp(unsigned long spi_base);

int bm_spi_flash_erase_sector(unsigned long spi_base, uint32_t addr, uint32_t addr_mode, uint32_t sector_size);
int bm_spi_flash_program_sector(unsigned long spi_base, uint32_t addr, uint32_t len, uint32_t addr_mode, uint8_t *buf);
void bm_spi_flash_read_sector(unsigned long spi_base, uint32_t addr, uint32_t addr_mode, uint8_t *buf);
void bm_spi_byte4en(unsigned long spi_base, uint8_t read_cmd);
uint32_t bm_spi_nor_enter_4byte(unsigned long spi_base, uint32_t enter);
int bm_read_int_flag();
void bm_write_int_flag(int val);

uint32_t bm_spi_reset_nor(unsigned long spi_base);
uint8_t spi_read_status(unsigned long spi_base);

#endif	/* __BM_SPINOR_H__ */
