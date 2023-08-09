#include <common.h>
#include <malloc.h>
#include "timer.h"

#include "system_common.h"
#include "sysdma.h"

#define PAGE_SIZE	4096
#define LLI_SIZE	0x40

#define TIMEOUT	50000 //us

uint64_t *tx_lli_start_address = NULL;
uint64_t *rx_lli_start_address = NULL;

#define min_t(type, x, y) ({				\
		type __min1 = (x);                      \
		type __min2 = (y);                      \
		__min1 < __min2 ? __min1 : __min2; })

# define do_div(n,base) ({					\
	uint32_t __base = (base);				\
	uint32_t __rem;						\
	__rem = ((uint64_t)(n)) % __base;			\
	(n) = ((uint64_t)(n)) / __base;				\
	__rem;							\
 })

static uint64_t *llp_addr_first;
static uint64_t *llp_addr_prev;

static uint64_t cal_kbs(uint64_t total_ticks, uint64_t len)
{
	//calculate KBs
	uint64_t kbs;
	unsigned long long per_sec = CONFIG_TIMER_FREQ;

	per_sec *= len;
	kbs = do_div(per_sec, total_ticks);
	printf("DMA memcpy %ld KBs \n", kbs);

	return kbs;
}

static void dump_lli_content(uint64_t * llp_first, int count)
{
	int i;
	uint64_t * llp = llp_first;
	for(i = 0;i < count;i++,llp += 4) {
		printf("lli			: %d, %p\n", i, llp);
		printf("sar			: 0x%lx\n", *llp++);
		printf("dar			: 0x%lx\n", *llp++);
		printf("block ts		: 0x%lx\n", *llp++);
		printf("llp			: 0x%lx\n", *llp++);
		printf("ctl			: 0x%lx\n", *llp);
		printf("\n");
	}
}

static void *llp_desc_get(void)
{
	static uint64_t *addr = NULL;

	if(!addr) {
		addr = (uint64_t *)0x101000000;
		if((uint64_t)addr & (LLI_SIZE - 1)) {
			addr = (uint64_t *)(((uint64_t)addr + LLI_SIZE) & (~(LLI_SIZE - 1)));
		}
	}
	return addr;
}

static void llp_desc_put(void * addr)
{
	//free(addr);
}

void bm_dma_turn_off(void)
{
	DMAC_WRITE(DMAC_CFGREG, 0x0);
}

void bm_dma_turn_on(void)
{
	DMAC_WRITE(DMAC_CFGREG, DMAC_CFG_DMA_EN | DMAC_CFG_DMA_INT_EN);
}

int device_alloc_chan(struct dma_chan *chan)
{
	u64 chan_en;

	if(chan->chan_id > DMAC_DMA_MAX_NR_CHANNELS) {
		printf("%s invalid channel id\n", __func__);
		return -1;
	}

	chan_en = DMAC_READ(DMAC_CHENREG);
	printf("%s, chan_en=0x%lx\n",__func__, chan_en);
	if(chan_en & (1 << chan->chan_id)) {
		printf("%s channel busy\n", __func__);
		return -1;
	}

	return 0;
}

void device_free_chan(struct dma_chan *chan)
{
}

static void prepare_memory_to_memory(struct dma_chan *chan)
{
	uint64_t ctl;
	static unsigned int first_lli = 1;
	uint64_t *llp_addr;
	size_t offset;
	size_t xfer_count;
	unsigned int trans_width;
	struct slave_config *config = chan->config;
	printf("prepare_memory_to_memory\n");
	bm_dma_turn_off();

	ctl = DWC_CTL_SRC_MSIZE(config->src_maxburst)
		| DWC_CTL_DST_MSIZE(config->dst_maxburst)
		| DWC_CTL_SMS(0)
		| DWC_CTL_DMS(0)
		| DWC_CTL_SRC_WIDTH(2)
		| DWC_CTL_DST_WIDTH(2)
		| DWC_CTL_SRC_INC
		| DWC_CTL_DST_INC;

	config->llp_phy_start = (u64 *)llp_desc_get();
	llp_addr = config->llp_phy_start;
	trans_width = 0x2;
	config->lli_count = 0;
	
	for (offset = 0; offset < config->len; offset = (offset + (xfer_count << trans_width))) {
		xfer_count = min_t(size_t, (config->len - offset) >> trans_width, MAX_BLOCK_TX);
		LLI_WRITE(llp_addr, LLI_OFFSET_SAR, config->src_addr + offset);
		LLI_WRITE(llp_addr, LLI_OFFSET_DAR, config->dst_addr + offset);
		ctl |= DWC_CTL_SHADOWREG_OR_LLI_VALID;
		LLI_WRITE(llp_addr, LLI_OFFSET_CTL, ctl);
		LLI_WRITE(llp_addr, LLI_OFFSET_BTS, xfer_count - 1 );

		if (first_lli){
			llp_addr_first = llp_addr;
		} else {
			LLI_WRITE(llp_addr_prev, LLI_OFFSET_LLP, (uint64_t)llp_addr | DMA0);
		}

		llp_addr_prev = llp_addr;
		first_lli = 0;
		llp_addr += (LLI_SIZE >> 3);
		config->lli_count++;
	}

	LLI_WRITE(llp_addr_prev, LLI_OFFSET_CTL, ctl | DWC_CTL_SHADOWREG_OR_LLI_LAST );
}

static void prepare_memory_to_device(struct dma_chan *chan)
{
	uint64_t ctl;
	static unsigned int first_lli = 1;
	uint64_t *llp_addr;
	size_t offset;
	size_t xfer_count;
	unsigned int trans_width;
	struct slave_config *config = chan->config;

	printf("%s **\n", __func__);

	bm_dma_turn_off();

	ctl = DWC_CTL_SRC_MSIZE(config->src_msize)
		| DWC_CTL_DST_MSIZE(config->dst_msize)
		| DWC_CTL_SMS(config->src_master)
		| DWC_CTL_DMS(config->dst_master)
		| DWC_CTL_SRC_WIDTH(config->src_addr_width)
		| DWC_CTL_DST_WIDTH(config->dst_addr_width)
		| DWC_CTL_SRC_INC
		| DWC_CTL_DST_FIX;

	trans_width = 0x2;
	for (offset = 0; offset < config->len; offset = (offset + (xfer_count << trans_width))) {
		xfer_count = min_t(size_t, (config->len - offset) >> trans_width, MAX_BLOCK_TX);
		if (xfer_count == 0)
			xfer_count = 1;

		if (!tx_lli_start_address) {
			llp_addr = (uint64_t *)((uint64_t)malloc(LLI_SIZE)&(~0x3f));
			tx_lli_start_address = llp_addr;
		} else {
			if (first_lli){
				llp_addr = tx_lli_start_address;
			} else {
				llp_addr += (LLI_SIZE/8);
			}
		}

		LLI_WRITE(llp_addr, LLI_OFFSET_SAR, config->src_addr + (offset / 4));
		LLI_WRITE(llp_addr, LLI_OFFSET_DAR, config->reg);
		LLI_WRITE(llp_addr, LLI_OFFSET_CTL, ctl | DWC_CTL_SHADOWREG_OR_LLI_VALID);
		LLI_WRITE(llp_addr, LLI_OFFSET_BTS, xfer_count - 1 );

		if (!first_lli)
			LLI_WRITE(llp_addr_prev, LLI_OFFSET_LLP, (uint64_t)llp_addr | 1);//set llp mem addr and select m1

		llp_addr_prev = llp_addr;
		first_lli = 0;
	}

	LLI_WRITE(llp_addr_prev, LLI_OFFSET_CTL, ctl | DWC_CTL_SHADOWREG_OR_LLI_LAST | DWC_CTL_SHADOWREG_OR_LLI_VALID);
}

static void prepare_device_to_memory(struct dma_chan *chan)
{
	uint64_t ctl;
	static unsigned int first_lli = 1;
	uint64_t *llp_addr;
	size_t offset;
	size_t xfer_count;
	unsigned int trans_width;
	struct slave_config *config = chan->config;

	printf("%s **\n", __func__);

	bm_dma_turn_off();

	ctl = DWC_CTL_SRC_MSIZE(config->src_msize)
		| DWC_CTL_DST_MSIZE(config->dst_msize)
		| DWC_CTL_SMS(config->src_master)
		| DWC_CTL_DMS(config->dst_master)
		| DWC_CTL_SRC_WIDTH(config->src_addr_width)
		| DWC_CTL_DST_WIDTH(config->dst_addr_width)
		| DWC_CTL_SRC_FIX
		| DWC_CTL_DST_INC;

	trans_width = 0x2;
	for (offset = 0; offset < config->len; offset = (offset + (xfer_count << trans_width))) {
		xfer_count = min_t(size_t, (config->len - offset) >> trans_width, MAX_BLOCK_TX);
		if (xfer_count == 0)
			xfer_count = 1;

		if (!rx_lli_start_address) {
			llp_addr = (uint64_t *)((uint64_t)malloc(0x40)&(~0x3f)); //link list item take 0x40 bytes
			rx_lli_start_address = llp_addr;
		} else {
			if (first_lli){
				llp_addr = rx_lli_start_address;
			} else {
				llp_addr += (LLI_SIZE/8);
			}
		}
		LLI_WRITE(llp_addr, LLI_OFFSET_SAR, config->reg);
		LLI_WRITE(llp_addr, LLI_OFFSET_DAR, config->dst_addr + (offset / 4));
		LLI_WRITE(llp_addr, LLI_OFFSET_CTL, ctl | DWC_CTL_SHADOWREG_OR_LLI_VALID);
		LLI_WRITE(llp_addr, LLI_OFFSET_BTS, xfer_count - 1 );

		if (!first_lli)
			LLI_WRITE(llp_addr_prev, LLI_OFFSET_LLP, (uint64_t)llp_addr | 1);//set llp mem addr and select m1

		llp_addr_prev = llp_addr;
		first_lli = 0;
	}

	LLI_WRITE(llp_addr_prev, LLI_OFFSET_CTL, ctl | DWC_CTL_SHADOWREG_OR_LLI_LAST | DWC_CTL_SHADOWREG_OR_LLI_VALID);
	tx_lli_start_address = rx_lli_start_address + ((4000000 + 64) / 8); //rx lli total numbers + 0x40 bytes offset
}

int device_prep_dma_slave(struct dma_chan *chan)
{
	int direction = chan->direction;

	switch (direction) {
	case DMA_MEM_TO_MEM:
		prepare_memory_to_memory(chan);
		break;
	case DMA_MEM_TO_DEV:
		prepare_memory_to_device(chan);
		break;
	case DMA_DEV_TO_MEM:
		prepare_device_to_memory(chan);
		break;
	default:
		printf("%s direction error\n",__func__);
		return -1;
	}

	return 0;
}

int device_issue_pending(struct dma_chan *chan)
{
	int direction;
	uint64_t cfg = 0, val;
	int timeout = 0;
	struct slave_config *config;
	u64 start_tick;
	u64 end_tick;

	bm_dma_turn_on();

	config = chan->config;
	direction = chan->direction;
	if(direction == DMA_MEM_TO_DEV) {
		cfg = DWC_CFG_SRC_OSR_LMT(15UL)
			| DWC_CFG_DST_OSR_LMT(15UL)
			| DWC_CFG_CH_PRIOR(7UL)
			| DWC_CFG_DST_MULTBLK_TYPE(3)
			| DWC_CFG_SRC_MULTBLK_TYPE(3)
			| DWC_CFG_TT_FC(1UL)
			| DWC_CFG_HS_SEL_SRC_HW
			| DWC_CFG_HS_SEL_DST_HW
			| DWC_CFG_SRC_PER((uint64_t)config->src_handshake_id)
			| DWC_CFG_DST_PER((uint64_t)config->dst_handshake_id);

		config->llp_phy_start = tx_lli_start_address;

	} else if(direction == DMA_DEV_TO_MEM) {
		cfg = DWC_CFG_SRC_OSR_LMT(15UL)
			| DWC_CFG_DST_OSR_LMT(15UL)
			| DWC_CFG_CH_PRIOR(7UL)
			| DWC_CFG_DST_MULTBLK_TYPE(3)
			| DWC_CFG_SRC_MULTBLK_TYPE(3)
			| DWC_CFG_TT_FC(2UL)
			| DWC_CFG_HS_SEL_SRC_HW
			| DWC_CFG_HS_SEL_DST_HW
			| DWC_CFG_SRC_PER((uint64_t)config->src_handshake_id)
			| DWC_CFG_DST_PER((uint64_t)config->dst_handshake_id);

		config->llp_phy_start = rx_lli_start_address;

	} else if(direction == DMA_MEM_TO_MEM) {
		cfg = DWC_CFG_SRC_OSR_LMT(15UL)
			| DWC_CFG_DST_OSR_LMT(15UL)
			| DWC_CFG_CH_PRIOR(7UL)
			| DWC_CFG_DST_MULTBLK_TYPE(3)
			| DWC_CFG_SRC_MULTBLK_TYPE(3)
			| DWC_CFG_TT_FC(0UL);
	}

	DMA_CHAN_WRITE(chan->chan_id, DWC_CHAN_CFG_OFFSET, cfg);
	DMA_CHAN_WRITE(chan->chan_id, DWC_CHAN_LLP_OFFSET, (uint64_t)config->llp_phy_start);
	val = ((1 << chan->chan_id ) << 8 | (1 << chan->chan_id));
	start_tick = timer_get_tick();
	DMAC_WRITE(DMAC_CHENREG, val);
	do {
		udelay(1);
		timeout += 1;
	}	while(((1 << chan->chan_id) & DMAC_READ(DMAC_CHENREG)) && timeout < TIMEOUT);
	end_tick = timer_get_tick();

	if(end_tick > start_tick)
		cal_kbs(end_tick - start_tick, config->len);

	if(timeout >= TIMEOUT) {
		printf("%s, DMA transfer failed chan_en = 0x%llx\n",
			__func__, DMAC_READ(DMAC_CHENREG));
		dump_lli_content(config->llp_phy_start, config->lli_count);
		return -1;
	} else
		printf("%s, DMA transfer done chan_en = 0x%llx\n",
		__func__, DMAC_READ(DMAC_CHENREG));

	bm_dma_turn_off();

	llp_desc_put(llp_addr_first);

	printf("enter %s done\n",__func__);

	return 0;
}

int sysdma_init(void)
{
//	u64 val;

//	val = readl(DMA_INT_BASE);
//	writel(DMA_INT_BASE, val|DMA_UART_ENABLE_MASK);

	return 0;
}
