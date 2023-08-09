#ifndef BITMAIN_SYSDMA_H
#define BITMAIN_SYSDMA_H

#define DMAC_DMA_MAX_NR_MASTERS		2
#define DMAC_DMA_MAX_NR_CHANNELS	8
#define MAX_BLOCK_TX			16

#define DMAC_BASE			0x29340000
//#define DMA_INT_BASE 			0x5801900C
#define DMA_UART_ENABLE_MASK 		0xE01 //UART0

#define DMAC_CFGREG	0x10
#define DMAC_CHENREG	0x18

#define DMAC_CFG_DMA_EN		(1 << 0)
#define DMAC_CFG_DMA_INT_EN	(1 << 1)

#define DWC_CTL_SMS(n)		((n & 0x1)<<0)	/* src master select */
#define DWC_CTL_DMS(n)		((n & 0x1)<<2)	/* dst master select */
#define DWC_CTL_SRC_INC		(0<<4)	/* Source Address Increment update*/
#define DWC_CTL_SRC_FIX		(1<<4)	/* Source Address Increment not*/
#define DWC_CTL_DST_INC		(0<<6)	/* Destination Address Increment update*/
#define DWC_CTL_DST_FIX		(1<<6)	/* Destination Address Increment not*/
#define DWC_CTL_SRC_WIDTH(n)	((n & 0x7)<<8)	/* Source Transfer Width */
#define DWC_CTL_DST_WIDTH(n)	((n & 0x7)<<11)	/* Destination Transfer Width */
#define DWC_CTL_SRC_MSIZE(n)	((n & 0xf)<<14)	/* SRC Burst Transaction Length, data items */
#define DWC_CTL_DST_MSIZE(n)	((n & 0xf)<<18)	/* DST Burst Transaction Length, data items */
#define DWC_CTL_AR_CACHE(n)	((n & 0xf)<<22)
#define DWC_CTL_AW_CACHE(n)	((n & 0xf)<<26)
#define DWC_CTL_N_LAST_W_EN	(1<<30)	/* last write posted write enable/disable*/
#define DWC_CTL_N_LAST_W_DIS	(0<<30)	/* last write posted wrtie enable/disable*/
#define DWC_CTL_ARLEN_DIS	(0UL<<38) /* Source Burst Length Disable */
#define DWC_CTL_ARLEN_EN	(1UL<<38) /* Source Burst Length Enable */
#define DWC_CTL_ARLEN(n)	((n & 0xff)<<39)
#define DWC_CTL_AWLEN_DIS	(0UL<<47) /* DST Burst Length Enable */
#define DWC_CTL_AWLEN_EN	(1UL<<47)
#define DWC_CTL_AWLEN(n)	((n & 0xff)<<48)
#define DWC_CTL_SRC_STA_DIS	(0UL<<56)
#define DWC_CTL_SRC_STA_EN	(1UL<<56)
#define DWC_CTL_DST_STA_DIS	(0UL<<57)
#define DWC_CTL_DST_STA_EN	(1UL<<57)
#define DWC_CTL_IOC_BLT_DIS	(0UL<<58)	/* Interrupt On completion of Block Transfer */
#define DWC_CTL_IOC_BLT_EN	(1UL<<58)
#define DWC_CTL_SHADOWREG_OR_LLI_LAST	(1UL<<62)	/* Last Shadow Register/Linked List Item */
#define DWC_CTL_SHADOWREG_OR_LLI_VALID	(1UL<<63)	/* Shadow Register content/Linked List Item valid */

#define DWC_CFG_SRC_MULTBLK_TYPE(x)	((x & 0x7) << 0)
#define DWC_CFG_DST_MULTBLK_TYPE(x)	((x & 0x7) << 2)
#define DWC_CFG_TT_FC(x)		((x & 0x7) << 32)
#define DWC_CFG_HS_SEL_SRC_HW		(0UL<<35)
#define DWC_CFG_HS_SEL_SRC_SW		(1UL<<35)
#define DWC_CFG_HS_SEL_DST_HW		(0UL<<36)
#define DWC_CFG_HS_SEL_DST_SW		(1UL<<36)
#define DWC_CFG_SRC_HWHS_POL_H		(0UL << 37)
#define DWC_CFG_SRC_HWHS_POL_L		(1UL << 37)
#define DWC_CFG_DST_HWHS_POL_H		(0UL << 38)
#define DWC_CFG_DST_HWHS_POL_L		(1UL << 38)
#define DWC_CFG_SRC_PER(x)		((x & 0xff) << 39)
#define DWC_CFG_DST_PER(x)		((x & 0xff) << 44)

#define DWC_CFG_CH_PRIOR_MASK		(0x7UL << 49)	/* priority mask */
#define DWC_CFG_CH_PRIOR(x)		((x & 0x7) << 49)	/* priority */
#define DWC_CFG_SRC_OSR_LMT(x)		(((x) & 0xf) << 55) /* max request x + 1 <= 16 */
#define DWC_CFG_DST_OSR_LMT(x)		(((x) & 0xf) << 59)

#define DWC_CHAN_CFG_OFFSET	0x120
#define DWC_CHAN_LLP_OFFSET	0x128

#define CH_EN_WE_OFFSET		8

#define LLI_OFFSET_SAR		0x0
#define LLI_OFFSET_DAR		0x8
#define LLI_OFFSET_BTS		0x10
#define LLI_OFFSET_LLP		0x18
#define LLI_OFFSET_CTL		0x20

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


#define DMAC_WRITE(offset, value) \
	writeq(DMAC_BASE + offset , (uint64_t)value)

#define DMAC_READ(offset) \
	readq(DMAC_BASE + offset)

#define DMA_CHAN_WRITE(chan, offset, value) \
	writeq((uint64_t)(DMAC_BASE + ((chan * 0x100) +offset)), (uint64_t)(value));

#define LLI_WRITE(addr, offset, value) \
	*(addr + (offset >> 3)) = (uint64_t)(value)

#define LLI_READ(addr, offset) \
	*(addr + (offset >> 3))

typedef uint64_t dma_addr_t;
typedef uint64_t phys_addr_t;

enum dma_master {
	DMA0,
	DMA1
};

enum transfer_direction {
	DMA_MEM_TO_MEM,
	DMA_MEM_TO_DEV,
	DMA_DEV_TO_MEM,
	DMA_TRANS_NONE,
};

enum slave_buswidth {
	SLAVE_BUSWIDTH_1_BYTE,
	SLAVE_BUSWIDTH_2_BYTES,
	SLAVE_BUSWIDTH_4_BYTES,
};

enum dma_msize {
	DW_DMA_MSIZE_1,
	DW_DMA_MSIZE_4,
	DW_DMA_MSIZE_8,
	DW_DMA_MSIZE_16,
	DW_DMA_MSIZE_32,
	DW_DMA_MSIZE_64,
	DW_DMA_MSIZE_128,
	DW_DMA_MSIZE_256,
};

/* flow controller */
enum dw_dma_fc {
	DW_DMA_FC_D_M2M, /* FlowControl is DMAC, mem to mem */
	DW_DMA_FC_D_M2P, /* FlowControl is DMAC, mem to perip */
	DW_DMA_FC_D_P2M,
	DW_DMA_FC_D_P2P,
	DW_DMA_FC_SP_P2M, /* FlowControl is Source periph, periph to mem */
	DW_DMA_FC_SP_P2P,
	DW_DMA_FC_DP_M2P, /* FlowControl is Dst periph, periph to mem */
	DW_DMA_FC_DP_P2P,
};

enum dma_handshake_id{
	I2S1_RX_HS_ID,
	I2S1_TX_HS_ID,
	I2S0_RX_HS_ID,
	I2S0_TX_HS_ID,
	UART3_RX_HS_ID,
	UART3_TX_HS_ID,
	UART2_RX_HS_ID,
	UART2_TX_HS_ID,
	UART1_RX_HS_ID,
	UART1_TX_HS_ID,
	UART0_RX_HS_ID,
	UART0_TX_HS_ID,
};

struct slave_config {
	enum transfer_direction direction;
	dma_addr_t src_addr;
	dma_addr_t dst_addr;
	phys_addr_t reg;
	int data_width;
	enum slave_buswidth src_addr_width;
	enum slave_buswidth dst_addr_width;
	u32 src_maxburst;
	u32 dst_maxburst;
	enum dma_msize src_msize;
	enum dma_msize dst_msize;
	int device_fc;
	unsigned int src_master;
	unsigned int dst_master;
	u64 *llp_phy_start;
	u64 len;
	int src_handshake_id;
	int dst_handshake_id;
	int lli_count;
};

struct dma_chan {
	int chan_id;
	enum transfer_direction direction;
	struct slave_config *config;
};

void dma_turn_off(void);
void dma_turn_on(void);
int sysdma_init(void);

int device_alloc_chan(struct dma_chan *chan);
void device_free_chan(struct dma_chan *chan);
int device_prep_dma_slave(struct dma_chan *chan);
int device_pause(struct dma_chan *chan);
int device_resume(struct dma_chan *chan);
int device_terminate_all(struct dma_chan *chan);
int device_issue_pending(struct dma_chan *chan);

#endif
