#include "mmio.h"
#include "ddr_sys.h"
#include "system_common.h"
#include "bitwise_ops.h"
#include "regconfig.h"

#define REAL_LOCK
// #define SSC_EN
// #define SSC_BYPASS

void phy_pll_init(void)
{
	// uint32_t apb_rddata;
	// uint32_t freq_in;
	// uint32_t tar_freq;
	// uint32_t mod_freq;
	// uint32_t dev_freq;
	// uint64_t reg_set;
	// uint64_t reg_span;
	// uint64_t reg_step;

	freq_in = 1501;
	mod_freq = 100;
	dev_freq = 15;
#if SSC_EN
	tar_freq = (ddr_data_rate >> 4) * 0.985;
#else
	tar_freq = (ddr_data_rate >> 4);
#endif
	reg_set = (uint64_t)freq_in * 67108864 / tar_freq;
	reg_span = ((tar_freq * 250) / mod_freq);
	reg_step = reg_set * dev_freq / (reg_span * 1000);
	uartlog("freq_in = %d\n", freq_in);
	uartlog("reg_set = %lx\n", reg_set);
	uartlog("tar_freq = %x\n", tar_freq);
	uartlog("reg_span = %lx\n", reg_span);
	uartlog("reg_step = %lx\n", reg_step);

	apb_rddata = 0x1;
	mmio_wr32(0x14 + CADENCE_PHYD_APB, apb_rddata);
	uartlog("TOP_REG_RX_LDO_EN\n");
#ifdef SSC_EN
	apb_rddata = reg_set; //TOP_REG_SSC_SET
	mmio_wr32(0x34 + CADENCE_PHYD_APB, apb_rddata);
	apb_rddata = get_bits_from_value(reg_span, 15, 0); //TOP_REG_SSC_SPAN
	mmio_wr32(0x38 + CADENCE_PHYD_APB, apb_rddata);
	apb_rddata = get_bits_from_value(reg_step, 23, 0); //TOP_REG_SSC_STEP
	mmio_wr32(0x3C + CADENCE_PHYD_APB, apb_rddata);
	ddr_mmio_rd32(0x30 + CADENCE_PHYD_APB, apb_rddata);
	apb_rddata = modified_bits_by_value(apb_rddata, ~get_bits_from_value(apb_rddata, 0, 0), 0, 0);
	apb_rddata = modified_bits_by_value(apb_rddata, 1, 1, 1); //
	apb_rddata = modified_bits_by_value(apb_rddata, 0, 3, 2); //TOP_REG_SSC_SSC_MODE
	apb_rddata = modified_bits_by_value(apb_rddata, 1, 4, 4); //
	apb_rddata = modified_bits_by_value(apb_rddata, 0, 8, 8); //
	apb_rddata = modified_bits_by_value(apb_rddata, 0, 9, 9); //
	apb_rddata = modified_bits_by_value(apb_rddata, 0, 10, 10); //
	apb_rddata = modified_bits_by_value(apb_rddata, 0, 11, 11); //
	apb_rddata = modified_bits_by_value(apb_rddata, 0, 13, 12); //TOP_REG_CLKDIV_CLKSEL
	mmio_wr32(0x30 + CADENCE_PHYD_APB, apb_rddata);
	uartlog("SSC_EN\n");
#else
#ifdef SSC_BYPASS
	ddr_mmio_rd32(0x30 + CADENCE_PHYD_APB, apb_rddata);
	apb_rddata = modified_bits_by_value(apb_rddata, 0, 1, 1); //
	apb_rddata = modified_bits_by_value(apb_rddata, 0, 3, 2); //TOP_REG_SSC_SSC_MODE
	apb_rddata = modified_bits_by_value(apb_rddata, 0, 4, 4); //
	apb_rddata = modified_bits_by_value(apb_rddata, mem_freq_2133, 8, 8); //
	apb_rddata = modified_bits_by_value(apb_rddata, 0, 9, 9); //
	apb_rddata = modified_bits_by_value(apb_rddata, mem_freq_1600, 10, 10); //
	apb_rddata = modified_bits_by_value(apb_rddata, mem_freq_1066, 11, 11); //
	apb_rddata = modified_bits_by_value(apb_rddata, clkdiv_clksel, 13, 12); //TOP_REG_CLKDIV_CLKSEL
	mmio_wr32(0x30 + CADENCE_PHYD_APB, apb_rddata);
	uartlog("SSC_BYPASS\n");
#else
	uartlog("SSC_EN =0\n");
	apb_rddata = reg_set; //TOP_REG_SSC_SET
	mmio_wr32(0x34 + CADENCE_PHYD_APB, apb_rddata);
	apb_rddata = get_bits_from_value(reg_span, 15, 0); //TOP_REG_SSC_SPAN
	mmio_wr32(0x38 + CADENCE_PHYD_APB, apb_rddata);
	apb_rddata = get_bits_from_value(reg_step, 23, 0); //TOP_REG_SSC_STEP
	mmio_wr32(0x3C + CADENCE_PHYD_APB, apb_rddata);
	ddr_mmio_rd32(0x30 + CADENCE_PHYD_APB, apb_rddata);
	apb_rddata = modified_bits_by_value(apb_rddata, ~get_bits_from_value(apb_rddata, 0, 0), 0, 0); //
	apb_rddata = modified_bits_by_value(apb_rddata, 0, 1, 1); //
	apb_rddata = modified_bits_by_value(apb_rddata, 0, 3, 2); //TOP_REG_SSC_SSC_MODE
	apb_rddata = modified_bits_by_value(apb_rddata, 1, 4, 4); //
	mmio_wr32(0x30 + CADENCE_PHYD_APB, apb_rddata);
	uartlog("SSC_OFF\n");
#endif //SSC_BYPASS
#endif //SSC_EN
	opdelay(1000);
	ddr_mmio_rd32(0x0C + CADENCE_PHYD_APB, apb_rddata);
	apb_rddata = modified_bits_by_value(apb_rddata, 1, 0, 0); //
	apb_rddata = modified_bits_by_value(apb_rddata, 1, 1, 1); //
	apb_rddata = modified_bits_by_value(apb_rddata, 0, 2, 2); //
	apb_rddata = modified_bits_by_value(apb_rddata, 1, 5, 3); //TOP_REG_DDRPLL_ICTRL
	apb_rddata = modified_bits_by_value(apb_rddata, 0, 6, 6); //
	apb_rddata = modified_bits_by_value(apb_rddata, 1, 8, 8); //
	apb_rddata = modified_bits_by_value(apb_rddata, 1, 10, 9); //TOP_REG_DDRPLL_SEL_MODE
	apb_rddata = modified_bits_by_value(apb_rddata, 0, 15, 15); //
	mmio_wr32(0x0C + CADENCE_PHYD_APB, apb_rddata);
	ddr_mmio_rd32(0x10 + CADENCE_PHYD_APB, apb_rddata);
	apb_rddata = modified_bits_by_value(apb_rddata, 0, 7, 0); //TOP_REG_DDRPLL_TEST
	mmio_wr32(0x10 + CADENCE_PHYD_APB, apb_rddata);
	apb_rddata = 0x1;
	mmio_wr32(0x04 + CADENCE_PHYD_APB, apb_rddata);
	ddr_mmio_rd32(0x0C + CADENCE_PHYD_APB, apb_rddata);
	apb_rddata = modified_bits_by_value(apb_rddata, 1, 7, 7); //
	mmio_wr32(0x0C + CADENCE_PHYD_APB, apb_rddata);
	uartlog("RSTZ_DIV=1\n");
	uartlog("Start DRRPLL LOCK pll init\n");
#ifdef REAL_LOCK
	//original   apb_rddata[15]=0;
	apb_rddata = modified_bits_by_value(apb_rddata, 0, 15, 15);
	while (get_bits_from_value(apb_rddata, 15, 15) == 0) {
		//original     apb_read(0x10, apb_rddata);
		ddr_mmio_rd32(0x10 + CADENCE_PHYD_APB, apb_rddata);
	}
#else
#endif
	uartlog("End DRRPLL LOCK=1... pll init\n");
}
