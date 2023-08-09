#include "bm_types.h"
#include "boot_test.h"
#include "mmio.h"
#include "system_common.h"

#define AXIMON_BASE_1 0x08008000

uint32_t axi_cycle_cnt[7][2]={0}; //[x][0]: write, [x][1]: read
uint32_t axi_byte_cnt[7][2]={0};

void axi_monitor_start(void)
{
	for (uint32_t i=0; i<6; i++) {
		for (uint32_t j=0; j<2; j++) {
			mmio_write_32(AXIMON_BASE_1 + i*0x100 + j*0x80 + 0x00, 0x10000); //func_en[0]
			// mmio_write_32(AXIMON_BASE_1 + i*0x100 + j*0x80 + 0x04, j+1);     //input_sel[5:0]: 1: Write, 2: Read
			mmio_write_32(AXIMON_BASE_1 + i*0x100 + j*0x80 + 0x04, j+3);     //input_sel[5:0]: 3: , 4:
			mmio_write_32(AXIMON_BASE_1 + i*0x100 + j*0x80 + 0x00, 0x70005); //snapshot[2], func_en[0]
			mmio_write_32(AXIMON_BASE_1 + i*0x100 + j*0x80 + 0x00, 0x40000); //snapshot[2]
		}
	}
}

void axi_monitor_snapshot(void)
{
	for (uint32_t i=0; i<6; i++) {
		for (uint32_t j=0; j<2; j++) {
			mmio_write_32(AXIMON_BASE_1 + i*0x100 + j*0x80 + 0x00, 0x40004); //snapshot[2]
			mmio_write_32(AXIMON_BASE_1 + i*0x100 + j*0x80 + 0x00, 0x40000);
		}
	}
}

void axi_monitor_end(void)
{
	for (uint32_t i=0; i<6; i++)
		for (uint32_t j=0; j<2; j++)
			mmio_write_32(AXIMON_BASE_1 + i*0x100 + j*0x80 + 0x00, 0x10000); //func_en[0]
}

void axi_monitor_read_count(void)
{
	for (uint32_t i=0; i<6; i++) {
		for(uint32_t j=0; j<2; j++) {
			axi_cycle_cnt[i][j]   = mmio_read_32(AXIMON_BASE_1 + i*0x100 + j*0x80 + 0x40);
			axi_byte_cnt[i][j]    = mmio_read_32(AXIMON_BASE_1 + i*0x100 + j*0x80 + 0x48);
		}
	}
}

void axi_monitor_print_measured_count(void)
{
	uartlog("m1: VIP real time\n");
	uartlog("m2: VIP off line\n");
	uartlog("m3: CPU\n");
	uartlog("m4: TPU\n");
	uartlog("m5: Video\n");
	uartlog("m6: Hsperi\n");

	for (uint32_t i=0; i<6; i++) {
		for(uint32_t j=0; j<2; j++) {
			uartlog("axi_cycle_cnt[%u][%u] = %u\n", i, j, axi_cycle_cnt[i][j]);
			uartlog("axi_byte_cnt[%u][%u] = %u\n", i, j, axi_byte_cnt[i][j]);
		}
	}
}

void axi_monitor_calculate_bandwidth(void)
{
	uint64_t fully_byte_cnt = axi_cycle_cnt[0][0] * 16;
	uint32_t total_byte_cnt = 0;

	axi_monitor_print_measured_count();

	for (uint32_t i=0; i<6; i++) {
		for(uint32_t j=0; j<2; j++) {
			total_byte_cnt += axi_byte_cnt[i][j];
		}
	}

	uartlog("total_byte_cnt = %u\n", total_byte_cnt);
	uartlog("fully_byte_cnt = %lu\n", fully_byte_cnt);
}
