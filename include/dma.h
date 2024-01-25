/*
 * Copyright (C) 2018 bitmain
 */
#define DMAC_BASE 0x7020000000
#define DMA_CHAN_BASE DMAC_BASE
#define DMA_INT_BASE 0x5801900C

#define DMA_I2S0_I2S1_ENABLE_MASK 0x000001FF

#define DMA_LLI_VALID_MASK 		0x8000000000000000
#define DMA_LLI_LAST_MASK 		0x4000000000000000
#define DMA_LLI_CTL_OFFSET		0x20
#define DMA_LLI_NEXT_LLP_OFFSET	0x18

#define TOP_DMA_CH_REMAP0	(TOP_BASE+0x154)
#define TOP_DMA_CH_REMAP1	(TOP_BASE+0x158)
#define TOP_DMA_CH_REMAP2	(TOP_BASE+0x15c)
#define TOP_DMA_CH_REMAP3	(TOP_BASE+0x160)

#define DMA_REMAP_CH0_OFFSET	0
#define DMA_REMAP_CH1_OFFSET	8
#define DMA_REMAP_CH2_OFFSET	16
#define DMA_REMAP_CH3_OFFSET	24
#define DMA_REMAP_CH4_OFFSET	0
#define DMA_REMAP_CH5_OFFSET	8
#define DMA_REMAP_CH6_OFFSET	16
#define DMA_REMAP_CH7_OFFSET	24
#define DMA_REMAP_UPDATE_OFFSET	31
#define DMA_REMAP_CH8_OFFSET	0
#define DMA_REMAP_CH9_OFFSET	8
#define DMA_REMAP_CH10_OFFSET	16
#define DMA_REMAP_CH11_OFFSET	24
#define DMA_REMAP_CH12_OFFSET	0
#define DMA_REMAP_CH13_OFFSET	8
#define DMA_REMAP_CH14_OFFSET	16
#define DMA_REMAP_CH15_OFFSET	24

#define DMA_CH0	0
#define DMA_CH1	1
#define DMA_CH2	2
#define DMA_CH3	3
#define DMA_CH4	4
#define DMA_CH5	5
#define DMA_CH6	6
#define DMA_CH7	7

/* refer to dma_remap.docx to define id to each channel */
#define DMA_RX_REQ_I2S0 		0
#define DMA_TX_REQ_I2S0 		1
#define DMA_RX_REQ_I2S1 		2
#define DMA_TX_REQ_I2S1 		3
#define DMA_RX_REQ_I2S2 		4
#define DMA_TX_REQ_I2S2 		5
#define DMA_RX_REQ_I2S3 		6
#define DMA_TX_REQ_I2S3 		7
#define DMA_RX_REQ_I2S4 		8
#define DMA_TX_REQ_I2S4 		9
#define DMA_RX_REQ_I2S5 		10
#define DMA_TX_REQ_I2S5 		11
#define DMA_DW_RX_REQ_I2S		12
#define DMA_DW_TX_REQ_I2S		13
#define DMA_RX_REQ_N_UART0 		14
#define DMA_TX_REQ_N_UART0 		15
#define DMA_RX_REQ_N_UART1 		16
#define DMA_TX_REQ_N_UART1 		17
#define DMA_RX_REQ_N_UART2 		18
#define DMA_TX_REQ_N_UART2 		19
#define DMA_RX_REQ_N_UART3 		20
#define DMA_TX_REQ_N_UART3 		21
#define DMA_RX_REQ_N_UART4 		22
#define DMA_TX_REQ_N_UART4 		23
#define DMA_RX_REQ_N_UART5 		24
#define DMA_TX_REQ_N_UART5 		25
#define DMA_RX_REQ_N_UART6 		26
#define DMA_TX_REQ_N_UART6 		27
#define DMA_RX_REQ_N_UART7 		28
#define DMA_TX_REQ_N_UART7 		29
#define DMA_RX_REQ_SPI0 		30
#define DMA_TX_REQ_SPI0 		31
#define DMA_RX_REQ_SPI1 		32
#define DMA_TX_REQ_SPI1 		33
#define DMA_RX_REQ_SPI2 		34
#define DMA_TX_REQ_SPI2 		35
#define DMA_RX_REQ_SPI3 		36
#define DMA_TX_REQ_SPI3 		37
#define DMA_RX_REQ_I2C0 		38
#define DMA_TX_REQ_I2C0 		39
#define DMA_RX_REQ_I2C1 		40
#define DMA_TX_REQ_I2C1 		41
#define DMA_RX_REQ_I2C2 		42
#define DMA_TX_REQ_I2C2 		43
#define DMA_RX_REQ_I2C3 		44
#define DMA_TX_REQ_I2C3 		45
#define DMA_RX_REQ_I2C4 		46
#define DMA_TX_REQ_I2C4 		47
#define DMA_RX_REQ_I2C5 		48
#define DMA_TX_REQ_I2C5 		49
#define DMA_RX_REQ_I2C6 		50
#define DMA_TX_REQ_I2C6 		51
#define DMA_RX_REQ_I2C7 		52
#define DMA_TX_REQ_I2C7 		53
#define DMA_RX_REQ_I2C8 		54
#define DMA_TX_REQ_I2C8 		55
#define DMA_RX_REQ_I2C9 		56
#define DMA_TX_REQ_I2C9 		57
#define DMA_RX_REQ_TDM0 		58
#define DMA_TX_REQ_TDM0 		59
#define DMA_RX_REQ_TDM1 		60
#define DMA_REQ_AUDSRC 			61
#define DMA_SPI_NAND			62
#define DMA_SPI_NOR				63

/* define peripheral id */
#define DMA_I2S0		0
#define DMA_I2S1		1
#define DMA_I2S2		2
#define DMA_I2S3		3
#define DMA_I2S4		4
#define DMA_I2S5		5
#define DMA_DW_I2S		6
#define DMA_I2C0		7
#define DMA_I2C1		8
#define DMA_I2C2		9
#define DMA_I2C3		10
#define DMA_I2C4		11
#define DMA_I2C5		12
#define DMA_I2C6		13
#define DMA_I2C7		14
#define DMA_I2C8		15
#define DMA_I2C9		15
#define DMA_SPI0		17
#define DMA_AUDSRC		18

#define LLI_SIZE		0x40

typedef	uintptr_t	size_t;


#define min_t(type, x, y) ({				\
		type __min1 = (x);                      \
		type __min2 = (y);                      \
		__min1 < __min2 ? __min1 : __min2; })

void dma_turn_on(void);
void dma_turn_off(void);
void dma_reset(void);

void dma_interrupt_init(void);

void dma_dev2mem_setting(unsigned int *src_addr, unsigned int len, unsigned int *reg, unsigned int p_dev);

void dma_mem2dev_setting(unsigned int *src_addr, unsigned int len, unsigned int *reg, unsigned int p_dev);

void dma_start_transfer(unsigned int p_dev);

void dma_start_receive(unsigned int p_dev);

void dma_start_txrx(void);

void dma_start_txrx_from_pdm(void);

void dma_start_src(void);

void concurrent_RX_dma_dev2mem_setting(unsigned int *src_addr1, unsigned int *src_addr2, unsigned int len,
									   unsigned int *reg1, unsigned int *reg2);

void dma_start_conurrent_rx(void);