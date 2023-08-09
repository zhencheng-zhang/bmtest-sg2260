/*
 * Copyright (C) 2018 bitmain
 */
#include <common.h>
#include <malloc.h>
#include "system_common.h"
//#include "bm_i2s_tdm.h"
//#include "bm_dw_i2c.h"
#include "timer.h"
#include "dma.h"
#include "mmio.h"

//uint64_t *dma_addr_first;
uint64_t *dma_addr_prev;

uint64_t *dma_addr_prev_1;
uint64_t *dma_addr_prev_2;

uint64_t *tx_lli_start_addr = NULL;
uint64_t *rx_lli_start_addr = NULL;

uint64_t *rx1_lli_start_addr = NULL;
uint64_t *rx2_lli_start_addr = NULL;


u32 txrx_count = 0;
/* dmac and dma channel init setting:
* dma turn off
*/

void dma_turn_off()
{
	DMAC_WRITE(0x10, 0x0);
}

void dma_turn_on()
{
	DMAC_WRITE(0x10, 0x3);
}

void dma_reset()
{
	printf("%s\n", __func__);
	DMAC_WRITE(0x58, 0x1);

}

void dma_mem2dev_setting(unsigned int *src_addr, unsigned int len, unsigned int *reg, unsigned int p_dev)
{
	uint64_t ctl;
	unsigned int first_lli = 1;
	uint64_t *llp_addr;
	size_t offset;
	size_t xfer_count;
	unsigned int trans_width = 0x2;
	unsigned int i = 0;

/*
2.1 prepare work, default ctl setting
	prepare ctl reg
	source and dest data width, tr_width
	data width = 0x2 AXI fix read or write one DWORD 32bits
	reg_width 0x0:bits,0x1:16bits
	src and dst mszie 0x4 32 data item, 32bits per item data
	SMS and DMS ,select M1 as dma master
	src inc and dst no change when dma do mem2dev copy
	bm1880_v2/bm1882 only dma master 1 support mem2dev or dev2mem
	i2s max fifo deepth is 16 * 32bits, so src and dst msize should be set <= 0x3
	(0x3 : 16 items,32bits per item)
*/
//	DMAC_WRITE(0x10, 0x0);	//turn off dma first

	switch (p_dev) {
		case DMA_I2S0:
		case DMA_I2S1:
		case DMA_I2S2:
		case DMA_I2S3:
		case DMA_DW_I2S:
		    //trans_width=0x2;
			ctl = 0x3 << 14 //SRC MSIZE
				| 0x3 << 18 //DST MSIZE
				| 0x2 << 8  //src_tr_width
				| 0x2 << 11 //dst_tr_width
				| 0x0UL << 48
				| 0x1UL << 47
				| 0xFUL << 39
				| 0x1UL << 38;
			break;
		case DMA_I2C0:
		case DMA_I2C1:
		case DMA_I2C2:
		case DMA_I2C3:
		case DMA_I2C4:
			//trans_width=0x1;
			ctl = 0x3 << 14 //SRC MSIZE
				| 0x3 << 18 //DST MSIZE
				| 0x2 << 8  //src_tr_width
				| 0x2 << 11; //dst_tr_width
			break;
		case DMA_SPI0:
			ctl = 0x1 << 14 //SRC MSIZE
				| 0x1 << 18 //DST MSIZE
				| 0x2 << 8  //src_tr_width
				| 0x2 << 11; //dst_tr_width
			break;
		default:
			return;
	}

	ctl |= 0x0 << 0 //SMS
		| 0x0 << 2 //DMS
		| 0X0 << 4 //SRC INC
		| 0X1 << 6; //DST no change


/*
2.2 prepare link list setting
	len: to send data size
	xfer_count: the count of link list items
	trans_width: data_width,32bits fixed (for I2C ?? 16 bits?)
	src_addr, dst_addr: memory addr
	max block ts  becase i2s fifo deepth == 16 * 32bits,so max_block_ts <= 116
	trans_width = 0x2
	reg:i2s fifo reg base addr
*/
	for (offset = 0; offset < len; offset = (offset+(xfer_count << trans_width))) {
		xfer_count = min_t(size_t, (len - offset) >> trans_width, 16);
		if (xfer_count == 0)
			xfer_count = 1;
		//Be make sure the allocated address with mask ~0x3f shouldn't re-write some meaningful memory
		if (!tx_lli_start_addr) {
			llp_addr = (uint64_t *)((uint64_t)malloc(LLI_SIZE)&(~0x3f));
			tx_lli_start_addr = llp_addr;
		} else {
			if (first_lli)
				llp_addr = tx_lli_start_addr;
			else {
			//	llp_addr = (uint64_t *)*(dma_addr_prev + 3);
			//	llp_addr = (uint64_t *)((uint64_t)llp_addr -1);
				llp_addr += (LLI_SIZE/8);
			}
		}

		LLI_WRITE(llp_addr, 0x0, src_addr + (offset / 4)); //sar, offset is 64 bytes
		LLI_WRITE(llp_addr, 0x8, reg); //dar

		LLI_WRITE(llp_addr, 0x20, ctl | 1UL << 63 ); //ctl, set lli valid bit
		LLI_WRITE(llp_addr, 0x10, xfer_count - 1 ); //block ts - 1
		//if (first_lli) {
		//	dma_addr_first = llp_addr;
		//} else {
		if (!first_lli)
			LLI_WRITE(dma_addr_prev, 0x18, (uint64_t)llp_addr | 1);//set llp mem addr and select m1
		//}

		dma_addr_prev = llp_addr;
		first_lli = 0;
		i++;
	}
	LLI_WRITE(dma_addr_prev, 0x20, ctl | 3LL << 62);//set llp the last block
	debug("TX alocate %d LLI\n", i);
}

/*
2.3 do start the dma transfer
	start initialize
	channel cfg setting
	Rx_dma_req_i2s1: handshake interface 0
	Tx_dma_req_i2s1: handshake interface 1
	Rx_dma_req_i2s0: handshake interface 2
	Tx_dma_req_i2s0: handshake interface 3
	dma on
*/
void dma_start_transfer(unsigned int p_dev)
{
	uint64_t cfg;
	unsigned int channel;
	uint64_t chan_en;
	uint64_t val=0;

	DMAC_WRITE(0x10, 0x3); //enable dma

	cfg = 15UL << 55
		| 15UL << 59 //src and dst request limit
		| 7UL << 49 // highest priority,0 is lowest priority
		| 0x3 << 2 //Linked List type of destination
		| 0x3 << 0 //0b11 to enable link list mode of source
		| 1UL << 32 // mem2dev dmac is the flow control,i2s has't fc
		| 0UL << 36 //Destination Software or Hardware Handshaking select
		| 0UL << 35; //Source Software or Hardware Handshaking Select

	// do start dma,

	switch (p_dev) {
		case DMA_I2S0:
			channel =  DMA_CH1;
			cfg |= 1UL << 39 // i2s0 tx src hw handshake interface
				| 1UL << 44; // i2s0 tx dst hw handshake interface
			break;
		case DMA_I2S1:
			channel = DMA_CH3;
			cfg |= 3UL << 39 // i2s1 tx src hw handshake interface
				| 3UL << 44; // i2s1 tx dst hw handshake interface
			break;
		case DMA_I2S2:
			cfg |= 5UL << 39 // i2s2 tx src hw handshake interface
				| 5UL << 44; // i2s2 tx dst hw handshake interface
			break;
		case DMA_I2S3:
			cfg |= 7UL << 39 // i2s3 tx src hw handshake interface
				| 7UL << 44; // i2s3 tx dst hw handshake interface
			break;
		case DMA_I2C0:
			channel = DMA_CH5;
			cfg |= 5UL << 39 // i2c0 tx src hw handshake interface
				| 5UL << 44; // i2c0 tx dst hw handshake interface
			break;
		case DMA_I2C1:
		case DMA_I2C2:
		case DMA_I2C3:
		case DMA_I2C4:
			break;
		case DMA_SPI0:
			channel = DMA_CH3;
			cfg |= 3UL << 39 // spi0 tx src hw handshake interface
				| 3UL << 44; // spi0 tx dst hw handshake interface
			break;
		default:
			return;
	}

	chan_en = DMAC_READ(0x18); // CH_EN
	//printf("%s, chan_en=0x%lx, channel=%d\n",__func__, chan_en, channel);
//	for (channel =0;channel < 7;channel++) {
	if(!((chan_en >> channel) & 0x1)) { //check if channel is inactive
		DMA_CHAN_WRITE(channel, 0x120, cfg);//cfg
		DMA_CHAN_WRITE(channel, 0x128, tx_lli_start_addr);//set the first llp mem addr to CHANNEL LLP reg

		val = (1 << channel ) << 8 | (1 << channel);
		DMAC_WRITE(0x18, val); //chan enable bit and chan enable enable bit
		timer_meter_start();
		chan_en = 1 << channel;
	    debug("TX channel_en=0x%lx\n", chan_en);

		do {
			mdelay(1);
			if (timer_meter_get_ms() > 20000) { //set timeout to 30s
				printf("M2P DMA timeout\n");
				break; //timeout
			}
		} while(chan_en & DMAC_READ(0x18));

		debug("%s, AFTER. chan_en=0x%llx, total time=%d ms\n",__func__, DMAC_READ(0x18), timer_meter_get_ms());
	} else
		printf("DMA channel %d is busy\n", channel);

	//}


/*3, stop dma */
	DMAC_WRITE(0x10, 0);
}

void dma_dev2mem_setting(unsigned int *src_addr, unsigned int len, unsigned int *reg, unsigned int p_dev)
{
	uint64_t ctl;
	unsigned int first_lli = 1;
	uint64_t *llp_addr;
	size_t offset;
	size_t xfer_count;
	unsigned int trans_width=0x2;
	unsigned int i = 0;

/*
2.1 prepare work, default ctl setting
	prepare ctl reg
	source and dest data width, tr_width
	data width = 0x2 AXI fix read or write one DWORD 32bits
	reg_width 0x0:bits,0x1:16bits
	src and dst mszie 0x4 32 data item, 32bits per item data
	SMS and DMS ,select M1 as dma master
	src inc and dst no change when dma do mem2dev copy
	only dma master 2 support mem2dev or dev2mem
	i2s max fifo deepth is 16 * 32bits, so src and dst msize should be set <= 0x3
	(0x3 : 16 items,32bits per item)
*/

	switch (p_dev) {
		case DMA_I2S0:
		case DMA_I2S1:
		case DMA_I2S2:
		case DMA_I2S3:
		case DMA_DW_I2S:
			ctl = 0x3 << 14 //SRC MSIZE
				| 0x3 << 18 //DST MSIZE
				| 0x2 << 8  //src_tr_width
				| 0x2 << 11 //dst_tr_width
				| 0xFUL << 48
				| 0x1UL << 47
				| 0x0UL << 39
				| 0x1UL << 38;
			break;
		case DMA_I2C0:
		case DMA_I2C1:
		case DMA_I2C2:
		case DMA_I2C3:
		case DMA_I2C4:
			ctl = 0x3 << 14 //SRC MSIZE
				| 0x3 << 18 //DST MSIZE
				| 0x2 << 8  //src_tr_width
				| 0x2 << 11; //dst_tr_width
			break;
		case DMA_SPI0:
			ctl = 0x3 << 14 //SRC MSIZE
				| 0x3 << 18 //DST MSIZE
				| 0x2 << 8  //src_tr_width
				| 0x2 << 11; //dst_tr_width
			break;
		case DMA_AUDSRC:
			ctl = 0x1 << 14 //SRC MSIZE
				| 0x1 << 18 //DST MSIZE
				| 0x2 << 8  //src_tr_width
				| 0x2 << 11 //dst_tr_width
				| 0xFUL << 48
				| 0x1UL << 47
				| 0x0UL << 39
				| 0x1UL << 38;
			break;
		default:
			return;
	}

//	DMAC_WRITE(0x10, 0x0);	//turn off dma first

	ctl |= 0x1 << 0 //SMS
		| 0x1 << 2 //DMS
		| 0X1 << 4 //SRC no change
		| 0X0 << 6; //DST inc

/* 2.2 prepare link list setting
	len: to send data size
	xfer_count: the count of link list items
	trans_width: data_width,32bits fixed
	src_addr, dst_addr: memory addr
	max block ts  becase i2s fifo deepth == 16 * 32bits,so max_block_ts <= 116
	trans_width = 0x2
	reg:i2s fifo reg base addr
*/
	for (offset = 0; offset < len; offset = (offset+(xfer_count << trans_width))) {
		if (xfer_count == 0)
			xfer_count = 1;
		xfer_count = min_t(size_t, (len - offset) >> trans_width, 16);
		if (!rx_lli_start_addr) {
			llp_addr = (uint64_t *)((uint64_t)malloc(0x40)&(~0x3f)); //link list item take 0x40 bytes
			rx_lli_start_addr = llp_addr;
		} else {
			if (first_lli)
				llp_addr = rx_lli_start_addr;
			else {
				llp_addr += (LLI_SIZE/8);
			}
		}
		LLI_WRITE(llp_addr, 0x0, reg); //sar
		LLI_WRITE(llp_addr, 0x8, src_addr + (offset / 4)); //dar, offset is 64 bytes

		LLI_WRITE(llp_addr, 0x20, ctl | 1UL << 63 ); //ctl, set lli valid bit

		LLI_WRITE(llp_addr, 0x10, xfer_count - 1 ); //block ts - 1

		if (!first_lli)
			LLI_WRITE(dma_addr_prev, 0x18, (uint64_t)llp_addr | 1);//set llp mem addr and select m1

		dma_addr_prev = llp_addr;
		first_lli = 0;
		i++;
	}
	LLI_WRITE(dma_addr_prev, 0x20, ctl | 3LL << 62);//set llp the last block
#if defined(BUILD_TEST_CASE_audio) && defined(RUN_IN_DDR)
	tx_lli_start_addr = rx_lli_start_addr + ((4000000 + 64) / 8); //rx lli total numbers + 0x40 bytes offset
#endif
	debug("RX alocate %d LLI\n", i);
}

void dma_start_receive(unsigned int p_dev)
{
	uint64_t cfg;
	unsigned int channel;
	uint64_t chan_en;
	uint64_t val=0;

	//debug("%s\n", __func__);
	DMAC_WRITE(0x10, 0x3); //enable dma

	cfg = 15UL << 55
		| 15UL << 59 //src and dst request limit
		| 7UL << 49 // highest priority,0 is lowest priority
		| 0x3 << 2 //Linked List type of destination
		| 0x3 << 0 //0b11 to enable link list mode of source
		| 2UL << 32 // dev2mem dmac is the flow control,i2s has't fc
		| 0UL << 36 //Destination Software or Hardware Handshaking select
		| 0UL << 35; //Source Software or Hardware Handshaking Select

	switch (p_dev) {
		case DMA_I2S0:
			channel =  DMA_CH0;
			cfg |= 0UL << 39 // i2s0 rx src hw handshake interface
				| 0UL << 44; // i2s0 rx dst hw handshake interface
			break;
		case DMA_I2S1:
			channel =  DMA_CH2;
			cfg |= 2UL << 39 // i2s1 rx src hw handshake interface
				| 2UL << 44; // i2s1 rx dst hw handshake interface
			break;
		case DMA_I2S2:
			channel =  DMA_CH4;
			cfg |= 4UL << 39 // i2s2 rx src hw handshake interface
				| 4UL << 44; // i2s2 rx dst hw handshake interface
			break;
		case DMA_I2S3:
			cfg |= 6UL << 39 // i2s3 rx src hw handshake interface
				| 6UL << 44; // i2s3 rx dst hw handshake interface
			break;
		case DMA_I2C0:
			channel =  DMA_CH4;
			cfg |= 4UL << 39 // i2c0 rx src hw handshake interface
				| 4UL << 44; // i2c0 rx dst hw handshake interface
			break;
		case DMA_I2C1:
		case DMA_I2C2:
		case DMA_I2C3:
		case DMA_I2C4:
			break;
		case DMA_SPI0:
			channel =  DMA_CH2;
			cfg |= 2UL << 39 // spi0 rx src hw handshake interface
				| 2UL << 44; // spi0 rx dst hw handshake interface
			break;
		default:
			return;
	}
	// do start dma,

	chan_en = DMAC_READ(0x18); // CH_EN

	if(!((chan_en >> channel) & 0x1)) { //check if channel is inactive
		DMA_CHAN_WRITE(channel, 0x120, cfg);//cfg
		DMA_CHAN_WRITE(channel, 0x128, rx_lli_start_addr);//set the first llp mem addr to CHANNEL LLP reg
		val = (1 << channel ) << 8 | (1 << channel );
		DMAC_WRITE(0x18, val); //chan enable bit and chan enable enable bit
		chan_en = 1 << channel;
		debug("RX channel_en=0x%lx\n", chan_en);
	// wait dma trans complete
		timer_meter_start();
		do {
			mdelay(10);
			val = DMAC_READ(0x18);
		//	debug("%s, AFTER. chan_en=0x%lx\n",__func__, val);

		//	if (timer_meter_get_ms() > 20000) { //set timeout to 30s
		//		printf("P2M DMA timeout\n");
		//		break; //timeout
		//	}
		} while(chan_en & val);

		printf("%s, AFTER. chan_en=0x%llx, total time=%d ms\n",__func__, DMAC_READ(0x18), timer_meter_get_ms());
	} else
		printf("DMA channel %d is busy\n", channel);

/* 3, stop dma */
	// dma turn off
	DMAC_WRITE(0x10, 0);

}


void dma_start_txrx(void)
{
	uint64_t cfg1;
	uint64_t cfg2;
	unsigned int channel_rx;
	unsigned int channel_tx;
	uint64_t chan_en;
	uint64_t val=0;


	cfg1 = 15UL << 55
		| 15UL << 59 //src and dst request limit
		| 7UL << 49 // highest priority,0 is lowest priority
		| 0x3 << 2 //Linked List type of destination
		| 0x3 << 0 //0b11 to enable link list mode of source
		| 2UL << 32 // dev2mem dmac is the flow control,i2s has't fc
		| 0UL << 36 //Destination Software or Hardware Handshaking select
		| 0UL << 35; //Source Software or Hardware Handshaking Select

	cfg2 = 15UL << 55
		| 15UL << 59 //src and dst request limit
		| 7UL << 49 // highest priority,0 is lowest priority
		| 0x3 << 2 //Linked List type of destination
		| 0x3 << 0 //0b11 to enable link list mode of source
		| 1UL << 32 // mem2dev dmac is the flow control,i2s has't fc
		| 0UL << 36 //Destination Software or Hardware Handshaking select
		| 0UL << 35; //Source Software or Hardware Handshaking Select


	// do start dma,

	channel_rx = DMA_CH2;
	cfg1 |= 2UL << 39 // i2s1 rx src hw handshake interface
		| 2UL << 44; // i2s1 rx dst hw handshake interface
	channel_tx = DMA_CH1;
	cfg2 |= 1UL << 39 // i2s0 tx src hw handshake interface
		| 1UL << 44; // i2s0 tx dst hw handshake interface

	chan_en = DMAC_READ(0x18); // CH_EN

	if(!((chan_en >> channel_rx) & 0x1) && !((chan_en >> channel_tx) & 0x1)) { //check if channel is inactive
		DMA_CHAN_WRITE(channel_rx, 0x120, cfg1);//cfg
		DMA_CHAN_WRITE(channel_rx, 0x128, rx_lli_start_addr);//set the first llp mem addr to CHANNEL LLP reg
		if (txrx_count > 0) {
			DMA_CHAN_WRITE(channel_tx, 0x120, cfg2);//cfg
			DMA_CHAN_WRITE(channel_tx, 0x128, tx_lli_start_addr);//set the first llp mem addr to CHANNEL LLP reg
		}

		if (txrx_count == 0){
			val = ((1 << channel_rx ) << 8) | (1 << channel_rx);
			chan_en = 1 << channel_rx;
		} else {
			val = (((1 << channel_rx) | (1 << channel_tx)) << 8) | ((1 << channel_rx) | (1 << channel_tx));
			chan_en = ((1 << channel_rx) | (1 << channel_tx));
		}
	    //printf("channel_en=0x%lx, val=0x%lx\n", chan_en, val);
		DMAC_WRITE(0x18, val); //chan enable bit and chan write enable bit

		txrx_count++;


		do {
#if 0
			unsigned int i2s_int=readl((unsigned int *)0x4100024);
			if ((i2s_int & 0x200) == 0x200)
				printf("I2S RXOF\n");
			if ((i2s_int & 0x400) == 0x400)
				printf("I2S RXUF\n");
			if ((i2s_int & 0x400) == 0x2000)
				printf("I2S TXOF\n");
			if ((i2s_int & 0x4000) == 0x4000)
				printf("I2S TXUF\n");
#endif
		} while(chan_en & DMAC_READ(0x18));

		debug("%s, AFTER. chan_en=0x%llx, total time=%d ms\n",__func__, DMAC_READ(0x18), timer_meter_get_ms());
	} else
		printf("DMA channel is busy\n");

}

void dma_start_txrx_from_pdm(void)
{
	uint64_t cfg1;
	uint64_t cfg2;
	unsigned int channel_rx;
	unsigned int channel_tx;
	uint64_t chan_en;
	uint64_t val=0;


	cfg1 = 15UL << 55
		| 15UL << 59 //src and dst request limit
		| 7UL << 49 // highest priority,0 is lowest priority
		| 0x3 << 2 //Linked List type of destination
		| 0x3 << 0 //0b11 to enable link list mode of source
		| 2UL << 32 // dev2mem dmac is the flow control,i2s has't fc
		| 0UL << 36 //Destination Software or Hardware Handshaking select
		| 0UL << 35; //Source Software or Hardware Handshaking Select

	cfg2 = 15UL << 55
		| 15UL << 59 //src and dst request limit
		| 7UL << 49 // highest priority,0 is lowest priority
		| 0x3 << 2 //Linked List type of destination
		| 0x3 << 0 //0b11 to enable link list mode of source
		| 1UL << 32 // mem2dev dmac is the flow control,i2s has't fc
		| 0UL << 36 //Destination Software or Hardware Handshaking select
		| 0UL << 35; //Source Software or Hardware Handshaking Select


	// do start dma,

	channel_rx = DMA_CH2;
	cfg1 |= 2UL << 39 // i2s1 rx src hw handshake interface
		| 2UL << 44; // i2s1 rx dst hw handshake interface
	channel_tx = DMA_CH5;
	cfg2 |= 5UL << 39 // i2s2 tx src hw handshake interface
		| 5UL << 44; // i2s2 tx dst hw handshake interface

	chan_en = DMAC_READ(0x18); // CH_EN

	if(!((chan_en >> channel_rx) & 0x1) && !((chan_en >> channel_tx) & 0x1)) { //check if channel is inactive
		DMA_CHAN_WRITE(channel_rx, 0x120, cfg1);//cfg
		DMA_CHAN_WRITE(channel_rx, 0x128, rx_lli_start_addr);//set the first llp mem addr to CHANNEL LLP reg
		if (txrx_count > 0) {
			DMA_CHAN_WRITE(channel_tx, 0x120, cfg2);//cfg
			DMA_CHAN_WRITE(channel_tx, 0x128, tx_lli_start_addr);//set the first llp mem addr to CHANNEL LLP reg
		}

		if (txrx_count == 0){
			val = ((1 << channel_rx ) << 8) | (1 << channel_rx);
			chan_en = 1 << channel_rx;
		} else {
			val = (((1 << channel_rx) | (1 << channel_tx)) << 8) | ((1 << channel_rx) | (1 << channel_tx));
			chan_en = ((1 << channel_rx) | (1 << channel_tx));
		}
	    //printf("channel_en=0x%lx, val=0x%lx\n", chan_en, val);
		DMAC_WRITE(0x18, val); //chan enable bit and chan write enable bit

		txrx_count++;


		do {
#if 0
			unsigned int i2s_int=readl((unsigned int *)0x4100024);
			if ((i2s_int & 0x200) == 0x200)
				printf("I2S RXOF\n");
			if ((i2s_int & 0x400) == 0x400)
				printf("I2S RXUF\n");
			if ((i2s_int & 0x400) == 0x2000)
				printf("I2S TXOF\n");
			if ((i2s_int & 0x4000) == 0x4000)
				printf("I2S TXUF\n");
#endif
		} while(chan_en & DMAC_READ(0x18));

		debug("%s, AFTER. chan_en=0x%llx, total time=%d ms\n",__func__, DMAC_READ(0x18), timer_meter_get_ms());
	} else
		printf("DMA channel is busy\n");

}

void dma_start_src(void)
{
	uint64_t cfg1;
	uint64_t cfg2;
	unsigned int channel_rx;
	unsigned int channel_tx;
	uint64_t chan_en;
	uint64_t val=0;


	cfg1 = 15UL << 55
		| 15UL << 59 //src and dst request limit
		| 7UL << 49 // highest priority,0 is lowest priority
		| 0x3 << 2 //Linked List type of destination
		| 0x3 << 0 //0b11 to enable link list mode of source
		| 2UL << 32 // dev2mem dmac is the flow control,i2s has't fc
		| 0UL << 36 //Destination Software or Hardware Handshaking select
		| 0UL << 35; //Source Software or Hardware Handshaking Select

	cfg2 = 15UL << 55
		| 15UL << 59 //src and dst request limit
		| 7UL << 49 // highest priority,0 is lowest priority
		| 0x3 << 2 //Linked List type of destination
		| 0x3 << 0 //0b11 to enable link list mode of source
		| 1UL << 32 // mem2dev dmac is the flow control,i2s has't fc
		| 0UL << 36 //Destination Software or Hardware Handshaking select
		| 0UL << 35; //Source Software or Hardware Handshaking Select


	// do start dma,

	channel_rx = DMA_CH6;
	cfg1 |= 6UL << 39 // audsrc src hw handshake interface
		| 6UL << 44; // audsrc dst hw handshake interface
	channel_tx = DMA_CH3;
	cfg2 |= 3UL << 39 // i2s1 tx src hw handshake interface
		| 3UL << 44; // i2s1 tx dst hw handshake interface

	chan_en = DMAC_READ(0x18); // CH_EN

	if(!((chan_en >> channel_rx) & 0x1) && !((chan_en >> channel_tx) & 0x1)) { //check if channel is inactive
		DMA_CHAN_WRITE(channel_tx, 0x120, cfg2);//cfg
		DMA_CHAN_WRITE(channel_tx, 0x128, tx_lli_start_addr);//set the first llp mem addr to CHANNEL LLP reg
//		if (txrx_count > 0) {
			DMA_CHAN_WRITE(channel_rx, 0x120, cfg1);//cfg
			DMA_CHAN_WRITE(channel_rx, 0x128, rx_lli_start_addr);//set the first llp mem addr to CHANNEL LLP reg
//		}

//		if (txrx_count == 0){
//			val = ((1 << channel_rx ) << 8) | (1 << channel_rx);
//			chan_en = 1 << channel_rx;
//		} else {
			val = (((1 << channel_rx) | (1 << channel_tx)) << 8) | ((1 << channel_rx) | (1 << channel_tx));
			chan_en = ((1 << channel_rx) | (1 << channel_tx));
//		}
	    //printf("channel_en=0x%lx, val=0x%lx\n", chan_en, val);
		DMAC_WRITE(0x18, val); //chan enable bit and chan write enable bit

		//txrx_count++;


		do {
#if 0
			unsigned int i2s_int=readl((unsigned int *)0x4100024);
			if ((i2s_int & 0x200) == 0x200)
				printf("I2S RXOF\n");
			if ((i2s_int & 0x400) == 0x400)
				printf("I2S RXUF\n");
			if ((i2s_int & 0x400) == 0x2000)
				printf("I2S TXOF\n");
			if ((i2s_int & 0x4000) == 0x4000)
				printf("I2S TXUF\n");
#endif
		} while(chan_en & DMAC_READ(0x18));

		debug("%s, AFTER. chan_en=0x%llx, total time=%d ms\n",__func__, DMAC_READ(0x18), timer_meter_get_ms());
	} else
		printf("DMA channel is busy\n");

}

void concurrent_RX_dma_dev2mem_setting(unsigned int *src_addr1, unsigned int *src_addr2, unsigned int len,
									   unsigned int *reg1, unsigned int *reg2)
{
	uint64_t ctl;
	unsigned int first_lli = 1;
	uint64_t *llp_addr;
	uint64_t *llp_addr_2;
	size_t offset;
	size_t xfer_count;
	unsigned int trans_width=0x2;
	unsigned int i = 0;
#if 1
	uint64_t cfg1;
	uint64_t cfg2;
	unsigned int channel1;
	unsigned int channel2;
	uint64_t chan_en;
	uint64_t val=0;
#endif
	printf("%s\n", __func__);
/*
2.1 prepare work, default ctl setting
	prepare ctl reg
	source and dest data width, tr_width
	data width = 0x2 AXI fix read or write one DWORD 32bits
	reg_width 0x0:bits,0x1:16bits
	src and dst mszie 0x4 32 data item, 32bits per item data
	SMS and DMS ,select M1 as dma master
	src inc and dst no change when dma do mem2dev copy
	only dma master 2 support mem2dev or dev2mem
	i2s max fifo deepth is 16 * 32bits, so src and dst msize should be set <= 0x3
	(0x3 : 16 items,32bits per item)
*/

	ctl = 0x3 << 14 //SRC MSIZE
		| 0x3 << 18 //DST MSIZE
		| 0x2 << 8  //src_tr_width
		| 0x2 << 11 //dst_tr_width
		| 0xFUL << 48
		| 0x1UL << 47
		| 0xFUL << 39
		| 0x1UL << 38
		| 0x1 << 0 //SMS
		| 0x1 << 2 //DMS
		| 0X1 << 4 //SRC no change
		| 0X0 << 6; //DST inc

//	DMAC_WRITE(0x10, 0x0);	//turn off dma first

/* 2.2 prepare link list setting
	len: to send data size
	xfer_count: the count of link list items
	trans_width: data_width,32bits fixed
	src_addr, dst_addr: memory addr
	max block ts  becase i2s fifo deepth == 16 * 32bits,so max_block_ts <= 116
	trans_width = 0x2
	reg:i2s fifo reg base addr
*/
	for (offset = 0; offset < len; offset = (offset+(xfer_count << trans_width))) {
		if (xfer_count == 0)
			xfer_count = 1;
		xfer_count = min_t(size_t, (len - offset) >> trans_width, 16);
		if (!rx1_lli_start_addr) {
			llp_addr = (uint64_t *)((uint64_t)malloc(0x40)&(~0x3f)); //link list item take 0x40 bytes
			rx1_lli_start_addr = llp_addr;
			llp_addr_2 = rx1_lli_start_addr+0x1000;
			rx2_lli_start_addr = llp_addr_2;
		} else {
			if (first_lli){
				llp_addr = rx1_lli_start_addr;
				llp_addr_2 = rx2_lli_start_addr;
			}
			else {
				llp_addr += (LLI_SIZE/8);
				llp_addr_2 += (LLI_SIZE/8);
			}
		}
		LLI_WRITE(llp_addr, 0x0, reg1); //sar
		LLI_WRITE(llp_addr, 0x8, src_addr1 + (offset / 4)); //dar, offset is 64 bytes

		LLI_WRITE(llp_addr, 0x20, ctl | 1UL << 63 ); //ctl, set lli valid bit

		LLI_WRITE(llp_addr, 0x10, xfer_count - 1 ); //block ts - 1

		LLI_WRITE(llp_addr_2, 0x0, reg2); //sar
		LLI_WRITE(llp_addr_2, 0x8, src_addr2 + (offset / 4)); //dar, offset is 64 bytes

		LLI_WRITE(llp_addr_2, 0x20, ctl | 1UL << 63 ); //ctl, set lli valid bit

		LLI_WRITE(llp_addr_2, 0x10, xfer_count - 1 ); //block ts - 1

		if (!first_lli){
			LLI_WRITE(dma_addr_prev_1, 0x18, (uint64_t)llp_addr | 1);//set llp mem addr and select m1
			LLI_WRITE(dma_addr_prev_2, 0x18, (uint64_t)llp_addr_2 | 1);//set llp mem addr and select m1
		}

		dma_addr_prev_1 = llp_addr;
		dma_addr_prev_2 = llp_addr_2;
		first_lli = 0;
		i++;
	}
	LLI_WRITE(dma_addr_prev_1, 0x20, ctl | 3LL << 62);//set llp the last block
	LLI_WRITE(dma_addr_prev_2, 0x20, ctl | 3LL << 62);//set llp the last block
	debug("Concurrent RX alocate %d LLI\n", i);

#if 1
	cfg1 = 15UL << 55
		| 15UL << 59 //src and dst request limit
		| 7UL << 49 // highest priority,0 is lowest priority
		| 0x3 << 2 //Linked List type of destination
		| 0x3 << 0 //0b11 to enable link list mode of source
		| 2UL << 32 // dev2mem dmac is the flow control,i2s has't fc
		| 0UL << 36 //Destination Software or Hardware Handshaking select
		| 0UL << 35; //Source Software or Hardware Handshaking Select

	cfg2 = 15UL << 55
		| 15UL << 59 //src and dst request limit
		| 7UL << 49 // highest priority,0 is lowest priority
		| 0x3 << 2 //Linked List type of destination
		| 0x3 << 0 //0b11 to enable link list mode of source
		| 2UL << 32 // dev2mem dmac is the flow control,i2s has't fc
		| 0UL << 36 //Destination Software or Hardware Handshaking select
		| 0UL << 35; //Source Software or Hardware Handshaking Select


		channel1 = DMA_CH2;
			cfg1 |= 2UL << 39 // i2s1 rx src hw handshake interface
				| 2UL << 44; // i2s1 rx dst hw handshake interface
		channel2 = DMA_CH4;
			cfg2 |= 4UL << 39 // i2s2 rx src hw handshake interface
				| 4UL << 44; // i2s2 rx dst hw handshake interface
	chan_en = DMAC_READ(0x18); // CH_EN

	if(!((chan_en >> channel1) & 0x1) && !((chan_en >> channel2) & 0x1)) { //check if channel is inactive
		DMA_CHAN_WRITE(channel1, 0x120, cfg1);//cfg
		DMA_CHAN_WRITE(channel1, 0x128, rx1_lli_start_addr);//set the first llp mem addr to CHANNEL LLP reg

		DMA_CHAN_WRITE(channel2, 0x120, cfg2);//cfg
		DMA_CHAN_WRITE(channel2, 0x128, rx2_lli_start_addr);//set the first llp mem addr to CHANNEL LLP reg



		val = (((1 << channel1) | (1 << channel2)) << 8) | ((1 << channel1) | (1 << channel2));
		chan_en = ((1 << channel1) | (1 << channel2));

	    printf("channel_en=0x%lx, val=0x%lx\n", chan_en, val);
		dma_turn_on();
		DMAC_WRITE(0x18, val); //chan enable bit and chan enable enable bit
		//timer_meter_start();

		//printf("%s, AFTER. chan_en=0x%llx, total time=%d ms\n",__func__, DMAC_READ(0x18), timer_meter_get_ms());
	} else
		printf("DMA channel is busy\n");
#endif


}

void dma_start_conurrent_rx(void)
{
#if 0
	uint64_t cfg1;
	uint64_t cfg2;
	unsigned int channel1;
	unsigned int channel2;
	uint64_t chan_en;
	uint64_t val=0;

	printf("%s\n", __func__);

	cfg1 = 15UL << 55
		| 15UL << 59 //src and dst request limit
		| 7UL << 49 // highest priority,0 is lowest priority
		| 0x3 << 2 //Linked List type of destination
		| 0x3 << 0 //0b11 to enable link list mode of source
		| 2UL << 32 // dev2mem dmac is the flow control,i2s has't fc
		| 0UL << 36 //Destination Software or Hardware Handshaking select
		| 0UL << 35; //Source Software or Hardware Handshaking Select

	cfg2 = 15UL << 55
		| 15UL << 59 //src and dst request limit
		| 7UL << 49 // highest priority,0 is lowest priority
		| 0x3 << 2 //Linked List type of destination
		| 0x3 << 0 //0b11 to enable link list mode of source
		| 2UL << 32 // dev2mem dmac is the flow control,i2s has't fc
		| 0UL << 36 //Destination Software or Hardware Handshaking select
		| 0UL << 35; //Source Software or Hardware Handshaking Select


		channel1 = DMA_CH2;
			cfg1 |= 2UL << 39 // i2s1 rx src hw handshake interface
				| 2UL << 44; // i2s1 rx dst hw handshake interface
		channel2 = DMA_CH4;
			cfg2 |= 4UL << 39 // i2s2 rx src hw handshake interface
				| 4UL << 44; // i2s2 rx dst hw handshake interface
	chan_en = DMAC_READ(0x18); // CH_EN

	if(!((chan_en >> channel1) & 0x1) && !((chan_en >> channel2) & 0x1)) { //check if channel is inactive
		DMA_CHAN_WRITE(channel1, 0x120, cfg1);//cfg
		DMA_CHAN_WRITE(channel1, 0x128, rx1_lli_start_addr);//set the first llp mem addr to CHANNEL LLP reg

		DMA_CHAN_WRITE(channel2, 0x120, cfg2);//cfg
		DMA_CHAN_WRITE(channel2, 0x128, rx2_lli_start_addr);//set the first llp mem addr to CHANNEL LLP reg



		val = (((1 << channel1) | (1 << channel2)) << 8) | ((1 << channel1) | (1 << channel2));
		chan_en = ((1 << channel1) | (1 << channel2));

	    printf("channel_en=0x%lx, val=0x%lx\n", chan_en, val);
		DMAC_WRITE(0x18, val); //chan enable bit and chan enable enable bit
		timer_meter_start();
		do {
		} while(chan_en & DMAC_READ(0x18));

		printf("%s, AFTER. chan_en=0x%llx, total time=%d ms\n",__func__, DMAC_READ(0x18), timer_meter_get_ms());
	} else
		printf("DMA channel is busy\n");
#else
		do {
		} while(DMAC_READ(0x18));
#endif

}