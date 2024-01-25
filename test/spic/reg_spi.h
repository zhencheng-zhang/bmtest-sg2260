#ifndef REG_SPI_H
#define REG_SPI_H


// verified
/* Full Register definitions */
#define REG_BM1680_SPI_CTRL                     0x000
#define REG_BM1680_SPI_CE_CTRL                  0x004
#define REG_BM1680_SPI_DLY_CTRL                 0x008
#define REG_BM1680_SPI_DMMR                     0x00C
#define REG_BM1680_SPI_TRAN_CSR                 0x010
#define REG_BM1680_SPI_TRAN_NUM                 0x014
#define REG_BM1680_SPI_FIFO_PORT                0x018

#define REG_BM1680_SPI_FIFO_PT                  0x020

#define REG_BM1680_SPI_INT_STS                  0x028
#define REG_BM1680_SPI_INT_EN                   0x02C

#define REG_BM1680_SPI_OPT						0x030


/* bit definition */
#define BM1680_SPI_CTRL_CPHA                    (0x01 << 12)
#define BM1680_SPI_CTRL_CPOL                    (0x01 << 13)
#define BM1680_SPI_CTRL_HOLD_OL                 (0x01 << 14)
#define BM1680_SPI_CTRL_WP_OL                   (0x01 << 15)
#define BM1680_SPI_CTRL_LSBF                    (0x01 << 20)
#define BM1680_SPI_CTRL_SRST                    (0x01 << 21)
#define BM1680_SPI_CTRL_SCK_DIV_SHIFT           0
#define BM1680_SPI_CTRL_FRAME_LEN_SHIFT         16

#define BM1680_SPI_CE_CTRL_CEMANUAL             (0x01 << 0)
#define BM1680_SPI_CE_CTRL_CEMANUAL_EN          (0x01 << 1)

#define BM1680_SPI_CTRL_FM_INTVL_SHIFT          0
#define BM1680_SPI_CTRL_CET_SHIFT               8

#define BM1680_SPI_DMMR_EN                      (0x01 << 0)

#define BM1680_SPI_TRAN_CSR_TRAN_MODE_RX        (0x01 << 0)
#define BM1680_SPI_TRAN_CSR_TRAN_MODE_TX        (0x01 << 1)
#define BM1680_SPI_TRAN_CSR_CNTNS_READ          (0x01 << 2)
#define BM1680_SPI_TRAN_CSR_FAST_MODE           (0x01 << 3)
#define BM1680_SPI_TRAN_CSR_BUS_WIDTH_1_BIT     (0x0 << 4)
#define BM1680_SPI_TRAN_CSR_BUS_WIDTH_2_BIT     (0x01 << 4)
#define BM1680_SPI_TRAN_CSR_BUS_WIDTH_4_BIT     (0x02 << 4)
#define BM1680_SPI_TRAN_CSR_DMA_EN              (0x01 << 6)
#define BM1680_SPI_TRAN_CSR_MISO_LEVEL          (0x01 << 7)
#define BM1680_SPI_TRAN_CSR_ADDR_BYTES_NO_ADDR  (0x0 << 8)
#define BM1680_SPI_TRAN_CSR_WITH_CMD            (0x01 << 11)
#define BM1680_SPI_TRAN_CSR_FIFO_TRG_LVL_1_BYTE (0x0 << 12)
#define BM1680_SPI_TRAN_CSR_FIFO_TRG_LVL_2_BYTE (0x01 << 12)
#define BM1680_SPI_TRAN_CSR_FIFO_TRG_LVL_4_BYTE (0x02 << 12)
#define BM1680_SPI_TRAN_CSR_FIFO_TRG_LVL_8_BYTE (0x03 << 12)
#define BM1680_SPI_TRAN_CSR_GO_BUSY             (0x01 << 15)

#define BM1680_SPI_TRAN_CSR_ADDR_BYTES_SHIFT    8

#define BM1680_SPI_TRAN_CSR_TRAN_MODE_MASK      0x0003
#define BM1680_SPI_TRAN_CSR_ADDR_BYTES_MASK     0x0700
#define BM1680_SPI_TRAN_CSR_FIFO_TRG_LVL_MASK   0x3000


#define BM1680_SPI_INT_TRAN_DONE                (0x01 << 0)
#define BM1680_SPI_INT_RD_FIFO                  (0x01 << 2)
#define BM1680_SPI_INT_WR_FIFO                  (0x01 << 3)
#define BM1680_SPI_INT_RX_FRAME                 (0x01 << 4)
#define BM1680_SPI_INT_TX_FRAME                 (0x01 << 5)

#define BM1680_SPI_INT_TRAN_DONE_EN             (0x01 << 0)
#define BM1680_SPI_INT_RD_FIFO_EN               (0x01 << 2)
#define BM1680_SPI_INT_WR_FIFO_EN               (0x01 << 3)
#define BM1680_SPI_INT_RX_FRAME_EN              (0x01 << 4)
#define BM1680_SPI_INT_TX_FRAME_EN              (0x01 << 5)


#define BM1680_SPI_MAX_FIFO_DEPTH               8

#endif /* REG_SPI_H */