#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include "mmio.h"
#include "reg_soc.h"
#include "ddr_sys.h"
#include "bitwise_ops.h"
#include "regconfig.h"
#include "system_common.h"

#define info printf

uint32_t  freq_in;
uint32_t  tar_freq;
uint32_t  mod_freq;
uint32_t  dev_freq;
uint64_t  reg_set;
uint64_t  reg_span;
uint64_t  reg_step;

uint32_t  int_sts_keep;
uint32_t  lpddr4_calvl_done;
uint32_t  CUR_PLL_SPEED;
uint32_t  NEXT_PLL_SPEED;
uint32_t  EN_PLL_SPEED_CHG;
// uint32_t  lpddr3_bit0_pass_save;
// uint32_t  lpddr3_bit1_pass_save;
// uint32_t  lpddr3_bit2_pass_save;
// uint32_t  lpddr3_bit3_pass_save;
// uint32_t  lpddr3_bit4_pass_save;
// uint32_t  lpddr3_bit5_pass_save;
// uint32_t  lpddr3_bit6_pass_save;
// uint32_t  lpddr3_bit7_pass_save;
// uint32_t  lpddr3_bit8_pass_save;
// uint32_t  lpddr3_bit9_pass_save;
// uint32_t  lpddr3_bit0_min_pass_save;
// uint32_t  lpddr3_bit1_min_pass_save;
// uint32_t  lpddr3_bit2_min_pass_save;
// uint32_t  lpddr3_bit3_min_pass_save;
// uint32_t  lpddr3_bit4_min_pass_save;

// uint32_t  lpddr3_bit4_min_pass_save;
// uint32_t  lpddr3_bit5_min_pass_save;
// uint32_t  lpddr3_bit6_min_pass_save;
// uint32_t  lpddr3_bit7_min_pass_save;
// uint32_t  lpddr3_bit8_min_pass_save;
// uint32_t  lpddr3_bit9_min_pass_save;
// uint32_t  lpddr3_bit0_max_pass_save;
// uint32_t  lpddr3_bit1_max_pass_save;
// uint32_t  lpddr3_bit2_max_pass_save;
// uint32_t  lpddr3_bit3_max_pass_save;
// uint32_t  lpddr3_bit4_max_pass_save;
// uint32_t  lpddr3_bit5_max_pass_save;
// uint32_t  lpddr3_bit6_max_pass_save;
// uint32_t  lpddr3_bit7_max_pass_save;
// uint32_t  lpddr3_bit8_max_pass_save;
// uint32_t  lpddr3_bit9_max_pass_save;
uint32_t  lpddr4_bit0_pass_save;
uint32_t  lpddr4_bit1_pass_save;
uint32_t  lpddr4_bit2_pass_save;
uint32_t  lpddr4_bit3_pass_save;
uint32_t  lpddr4_bit4_pass_save;

uint32_t  lpddr4_bit5_pass_save;
uint32_t  lpddr4_bit6_pass_save;
uint32_t  lpddr4_bit7_pass_save;
uint32_t  lpddr4_bit8_pass_save;
uint32_t  lpddr4_bit9_pass_save;
uint32_t  lpddr4_bit0_min_pass_save;
uint32_t  lpddr4_bit1_min_pass_save;
uint32_t  lpddr4_bit2_min_pass_save;
uint32_t  lpddr4_bit3_min_pass_save;
uint32_t  lpddr4_bit4_min_pass_save;
uint32_t  lpddr4_bit5_min_pass_save;
uint32_t  lpddr4_bit6_min_pass_save;
uint32_t  lpddr4_bit7_min_pass_save;
uint32_t  lpddr4_bit8_min_pass_save;
uint32_t  lpddr4_bit9_min_pass_save;
uint32_t  lpddr4_bit0_max_pass_save;
uint32_t  lpddr4_bit1_max_pass_save;
uint32_t  lpddr4_bit2_max_pass_save;
uint32_t  lpddr4_bit3_max_pass_save;
uint32_t  lpddr4_bit4_max_pass_save;
uint32_t  lpddr4_bit5_max_pass_save;
uint32_t  lpddr4_bit6_max_pass_save;
uint32_t  lpddr4_bit7_max_pass_save;
uint32_t  lpddr4_bit8_max_pass_save;
uint32_t  lpddr4_bit9_max_pass_save;
uint32_t  read_temp;

// uint32_t  rddata_ctrl;
// uint32_t  rddata_addr;
uint32_t  dram_class;
// uint32_t  pi_init_work_freq;
uint32_t  apb_rddata;
// uint32_t  axi_rddata;
uint32_t  axi_rddata_;
uint32_t  rddata;
uint32_t  ken_rddata;
uint32_t  read_data;
uint32_t  axi_rddata;
uint32_t  axi_rddata1;
uint32_t  axi_rddata2;
uint32_t  axi_rddata3;
uint32_t  axi_rddata4;
uint32_t  axi_rddata6;
uint32_t  axi_rddata7;
uint32_t  axi_rddata8;
uint32_t  bist_result;
uint64_t  err_data_odd;
uint64_t  err_data_even;


void ddrc_init(void)
{
	for (int i = 0; i < ddrc_regs_count; i++)
		mmio_wr32(ddrc_regs[i].addr, ddrc_regs[i].val);
}

void pi_init(void)
{
	for (int i = 0; i < pi_regs_count; i++)
		mmio_wr32(PI_BASE + i * 4, pi_regs[i]);
}

void phy_init(void)
{
	for (int i = 0; i < phy_regs_count; i++)
		mmio_wr32(phy_regs[i].addr, phy_regs[i].val);
}

void ctrl_init_high_patch(void)
{
	for (int i = 0; i < ddrc_patch_regs_count; i++)
		mmio_wr32(ddrc_patch_regs[i].addr, ddrc_patch_regs[i].val);
}

#if 0
void sys_pll_init(void)
{
	uint32_t read_val = 0;
	uint32_t write_val = 0;

	info("BLD:  sys_pll_init ");
	mmio_wr32(PLL_CTRL_G2_BASE + PLL_G2_SSC_SYN_CTRL, 0x3E); // enable synthesizer clock enable

	mmio_wr32(PLL_CTRL_G2_BASE + APLL_SSC_SYN_SET, 385505882); // set apll synthesizer  104.4480001 M
	read_val = mmio_rd32(PLL_CTRL_G2_BASE + APLL_SSC_SYN_CTRL);
	write_val = read_val ^ 0x1;
	mmio_wr32(PLL_CTRL_G2_BASE + APLL_SSC_SYN_CTRL, write_val); // bit 0 toggle
	mmio_wr32(PLL_CTRL_G2_BASE + APLL0_CSR, 0x01108201); // set apll *8/2

	mmio_wr32(PLL_CTRL_G2_BASE + DISPPLL_SSC_SYN_SET, 406720388); // set disp synthesizer  98.99999997 M
	read_val = mmio_rd32(PLL_CTRL_G2_BASE + DISPPLL_SSC_SYN_CTRL);
	write_val = read_val ^ 0x1;
	mmio_wr32(PLL_CTRL_G2_BASE + DISPPLL_SSC_SYN_CTRL, write_val); // bit 0 toggle
	mmio_wr32(PLL_CTRL_G2_BASE + DISPPLL_CSR, 0x00188101); // set disp *12/1

	mmio_wr32(PLL_CTRL_G2_BASE + CAM0PLL_SSC_SYN_SET, 393705325); // set cam0 synthesizer  102.27273 M
	read_val = mmio_rd32(PLL_CTRL_G2_BASE + CAM0PLL_SSC_SYN_CTRL);
	write_val = read_val ^ 0x1;
	mmio_wr32(PLL_CTRL_G2_BASE + CAM0PLL_SSC_SYN_CTRL, write_val); // bit 0 toggle
	mmio_wr32(PLL_CTRL_G2_BASE + CAM0PLL_CSR, 0x00168000); // set cam0 *11/1

	mmio_wr32(PLL_CTRL_G2_BASE + CAM1PLL_SSC_SYN_SET, 421827145); // set cam1 synthesizer  95.45454549 M
	read_val = mmio_rd32(PLL_CTRL_G2_BASE + CAM1PLL_SSC_SYN_CTRL);
	write_val = read_val ^ 0x1;
	mmio_wr32(PLL_CTRL_G2_BASE + CAM1PLL_SSC_SYN_CTRL, write_val); // bit 0 toggle
	mmio_wr32(PLL_CTRL_G2_BASE + CAM1PLL_CSR, 0x00168000); // set cam1 *11/1

	read_val = mmio_rd32(PLL_CTRL_G2_BASE + PLL_G2_CTRL);
	write_val = read_val & (~0x00011111);
	mmio_wr32(PLL_CTRL_G2_BASE + PLL_G2_CTRL, write_val); //clear all pll PD

	info("done\n");
}
#endif

void DDR_reset(void)
{
	uint32_t read_val;

	//Read out 0x0300_3000 get read_data[31:0]
	read_val = mmio_rd32(0x03003000);

	//Change read_data[2] to 1b0
	//Write 0x0300_3000 with modified read_data
	mmio_wr32(0x03003000, read_val & (~(BIT(2))));

	//Wait about 1us
	// udelay(10);
	opdelay(100);

	//Change read_data[2] to 1b1
	//Write 0x0300_3000 with modified read_data
	read_val = mmio_rd32(0x03003000);
	mmio_wr32(0x03003000, read_val | (BIT(2)));
}

#ifdef SUBTYPE_FPGA
static void write_to_mem(uintptr_t addr, uint32_t value)
{
	mmio_wr32(addr, value);
}
#endif /* SUBTYPE_FPGA */


#define RTC_SRAM_FLAG_ADDR		0x03005800
#define RTC_SRAM_TRUSTED_MAILBOX_BASE	(RTC_SRAM_FLAG_ADDR + 8)
#define PLAT_BM_TRUSTED_MAILBOX_BASE	0x0E00F000
#define PLAT_BM_TRUSTED_MAILBOX_SIZE	0x28
#define REG_RTC_ST_ON_REASON		0x030050F8
static uint64_t get_warmboot_entry(void)
{
	/* Check if RTC state changed from ST_SUSP */
	if ((mmio_rd32(REG_RTC_ST_ON_REASON) & 0xF) == 0x9)
		return *(volatile uint64_t *)RTC_SRAM_FLAG_ADDR;
	else
		return 0;
}

static void restore_trusted_mailbox(void)
{
	uint64_t src, dst, end;

	src = RTC_SRAM_FLAG_ADDR + 8;
	dst = PLAT_BM_TRUSTED_MAILBOX_BASE;
	end = dst + PLAT_BM_TRUSTED_MAILBOX_SIZE;
	do {
		*(volatile uint64_t *)dst = *(volatile uint64_t *)src;
		dst += 8;
		src += 8;
	} while (dst < end);
}

static void ddr_patch_set(void)
{
	mmio_wr32(0x08006014, 0x00000015);  //RX_LDO

#if defined(DDR3) && defined(ODT)
	mmio_wr32(0x08006018, 0x04040404);  //ODT

	//force reg write =1
	rddata = mmio_rd32(0x0800A204);
	rddata = modified_bits_by_value(rddata, 0x1, 0, 0);
	mmio_wr32(0x0800A204,  rddata);

	//write mr1 to 0x42
	rddata = mmio_rd32(0x080040dc);
	rddata = modified_bits_by_value(rddata, 0x42, 15, 0);
	mmio_wr32(0x080040dc,  rddata);
#elif defined(LP4) && defined(ODT)
	mmio_wr32(0x08006018, 0x42424242);  //ODT

	// mr4 disable: [0] derate_enable = 0
	rddata = mmio_rd32(0x08004020);
	rddata = modified_bits_by_value(rddata, 0, 0, 0);
	mmio_wr32(0x08004020, rddata);
	mmio_wr32(0x08004030, 0x00000100); //Temperoay setting: disable self refresh
	mmio_wr32(0x080040e8, 0x00020062);
	mmio_wr32(0x08002028, 0x00000823);

	// SOC ODT 120 ohm --> 40 ohm
	rddata = mmio_rd32(cfg_base + 0xec);
	rddata = modified_bits_by_value(rddata, 0x26, 31, 16);
	mmio_wr32(cfg_base + 0xec, rddata);

	//ODT CENTER TAP [3]    trigger level =5'h6 [20:16] [28:24]
	mmio_wr32(0x08002020, 0x00000008);
	mmio_wr32(0x08002030, 0x00000008);
	mmio_wr32(0x08002040, 0x00000008);
	mmio_wr32(0x08002050, 0x00000008);
	//Disable en_lsmode
	mmio_wr32(0x08002014, 0x10101010);

	// #define trig_lvl_start 0x0
	// #define trig_lvl_end 0x4
	rddata = mmio_rd32(4 * (220 + PHY_BASE_ADDR) + CADENCE_PHYD);
	rddata = modified_bits_by_value(rddata, 0, 4, 0);
	rddata = modified_bits_by_value(rddata, 4, 9, 5);
	mmio_wr32(4 * (220 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
#if 0 //for tj
	//all bank refresh
	// 0x08004064[31] = 0x0
	// 0x08004064[27:16] = 0x51
	// 0x08004064[9:0] = 0x78
	// 0x08004050[2] = 0x0
	rddata = 0;
	rddata = modified_bits_by_value(rddata, 0x0, 31, 31);
	rddata = modified_bits_by_value(rddata, 0x51, 27, 16);
	rddata = modified_bits_by_value(rddata, 0x78, 9, 0);
	mmio_wr32(0x08004064, rddata);

	rddata = mmio_rd32(0x08004050);
	rddata = modified_bits_by_value(rddata, 0x0, 9, 4);
	rddata = modified_bits_by_value(rddata, 0x0, 2, 2);
	mmio_wr32(0x08004050, rddata);
#endif //for tj

#if defined(DBI_OFF)
	//dbi off
	rddata = mmio_rd32(0x080040e0);
	rddata = modified_bits_by_value(rddata, 0x20, 31, 16);
	mmio_wr32(0x080040e0, rddata);
	mmio_wr32(0x080041c0, 0x1);
	// bm_synp_mrw_lp4(3, 0x20);
#endif //DBI_OFF
#endif //LP4 && ODT
}

static void ddr_pinmux(void)
{
#if defined(LP4)
	uartlog("[DBG] KC, pin mux setting\n");
	//------------------------------
	//  pin mux base on PHYA
	//------------------------------
	//param_pi_data_byte_swap_en[0:0] <= `PI_SD int_regin[0:0];
	//param_pi_data_byte_swap_slice0[1:0] <= `PI_SD int_regin[9:8];
	//param_pi_data_byte_swap_slice1[1:0] <= `PI_SD int_regin[17:16];
	//param_pi_data_byte_swap_slice2[1:0] <= `PI_SD int_regin[25:24];
	////original      REGREAD(89 + PI_BASE_ADDR, rddata);
	ddr_mmio_rd32(4 * (89 + PI_BASE_ADDR) + CADENCE_PHYD, rddata);
	//original      rddata[0]     = 0b1;     //param_pi_data_byte_swap_en
	rddata = modified_bits_by_value(rddata, 0b1, 0, 0); //
	//original      rddata[9:8]   = 0b10;    //param_pi_data_byte_swap_slice0
	rddata = modified_bits_by_value(rddata, 0b10, 9, 8); //param_pi_data_byte_swap_slice0
	//original      rddata[17:16] = 0b11;    //param_pi_data_byte_swap_slice1
	rddata = modified_bits_by_value(rddata, 0b11, 17, 16); //param_pi_data_byte_swap_slice1
	//original      rddata[25:24] = 0b00;    //param_pi_data_byte_swap_slice2
	rddata = modified_bits_by_value(rddata, 0b00, 25, 24); //param_pi_data_byte_swap_slice2
	//original      REGWR  (89 + PI_BASE_ADDR, rddata, 0);
	mmio_wr32(4 * (89 + PI_BASE_ADDR) + CADENCE_PHYD, rddata);
	//param_pi_data_byte_swap_slice3[1:0] <= `PI_SD int_regin[1:0];
	////original      REGREAD(90 + PI_BASE_ADDR, rddata);
	ddr_mmio_rd32(4 * (90 + PI_BASE_ADDR) + CADENCE_PHYD, rddata);
	//original      rddata[1:0]   = 0b01;    //param_pi_data_byte_swap_slice3
	rddata = modified_bits_by_value(rddata, 0b01, 1, 0); //param_pi_data_byte_swap_slice3
	//original      REGWR  (90 + PI_BASE_ADDR, rddata, 0);
	mmio_wr32(4 * (90 + PI_BASE_ADDR) + CADENCE_PHYD, rddata);
	////original      REGREAD(106 + PHY_BASE_ADDR, rddata);
	ddr_mmio_rd32(4 * (106 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	//original      rddata[3:0]   = 45;   //param_phyd_swap_byte0_dm_mux[3:0]
	rddata = modified_bits_by_value(rddata, 5, 3, 0); //param_phyd_swap_byte0_dm_mux[3:0]
	//original      rddata[7:4]   = 48;   //param_phyd_swap_byte0_dq0_mux[3:0]
	rddata = modified_bits_by_value(rddata, 8, 7, 4); //param_phyd_swap_byte0_dq0_mux[3:0]
	//original      rddata[11:8]  = 47;   //param_phyd_swap_byte0_dq1_mux[3:0]
	rddata = modified_bits_by_value(rddata, 7, 11, 8); //param_phyd_swap_byte0_dq1_mux[3:0]
	//original      rddata[15:12] = 40;   //param_phyd_swap_byte0_dq2_mux[3:0]
	rddata = modified_bits_by_value(rddata, 0, 15, 12); //param_phyd_swap_byte0_dq2_mux[3:0]
	//original      rddata[19:16] = 42;   //param_phyd_swap_byte0_dq3_mux[3:0]
	rddata = modified_bits_by_value(rddata, 2, 19, 16); //param_phyd_swap_byte0_dq3_mux[3:0]
	//original      rddata[23:20] = 46;   //param_phyd_swap_byte0_dq4_mux[3:0]
	rddata = modified_bits_by_value(rddata, 6, 23, 20); //param_phyd_swap_byte0_dq4_mux[3:0]
	//original      rddata[27:24] = 41;   //param_phyd_swap_byte0_dq5_mux[3:0]
	rddata = modified_bits_by_value(rddata, 1, 27, 24); //param_phyd_swap_byte0_dq5_mux[3:0]
	//original      rddata[31:28] = 43;   //param_phyd_swap_byte0_dq6_mux[3:0]
	rddata = modified_bits_by_value(rddata, 3, 31, 28); //param_phyd_swap_byte0_dq6_mux[3:0]
	//original      REGWR  (106 + PHY_BASE_ADDR, rddata, 0);
	mmio_wr32(4 * (106 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	////original      REGREAD(107 + PHY_BASE_ADDR, rddata);
	ddr_mmio_rd32(4 * (107 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	//original      rddata[3:0]   = 44;   //param_phyd_swap_byte0_dq7_mux[3:0]
	rddata = modified_bits_by_value(rddata, 4, 3, 0); //param_phyd_swap_byte0_dq7_mux[3:0]
	//original      REGWR  (107 + PHY_BASE_ADDR, rddata, 0);
	mmio_wr32(4 * (107 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	////original      REGREAD(108 + PHY_BASE_ADDR, rddata);
	ddr_mmio_rd32(4 * (108 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	//original      rddata[3:0]   = 41;   //param_phyd_swap_byte1_dm_mux[3:0]
	rddata = modified_bits_by_value(rddata, 1, 3, 0); //param_phyd_swap_byte1_dm_mux[3:0]
	//original      rddata[7:4]   = 48;   //param_phyd_swap_byte1_dq0_mux[3:0]
	rddata = modified_bits_by_value(rddata, 8, 7, 4); //param_phyd_swap_byte1_dq0_mux[3:0]
	//original      rddata[11:8]  = 45;   //param_phyd_swap_byte1_dq1_mux[3:0]
	rddata = modified_bits_by_value(rddata, 5, 11, 8); //param_phyd_swap_byte1_dq1_mux[3:0]
	//original      rddata[15:12] = 47;   //param_phyd_swap_byte1_dq2_mux[3:0]
	rddata = modified_bits_by_value(rddata, 7, 15, 12); //param_phyd_swap_byte1_dq2_mux[3:0]
	//original      rddata[19:16] = 46;   //param_phyd_swap_byte1_dq3_mux[3:0]
	rddata = modified_bits_by_value(rddata, 6, 19, 16); //param_phyd_swap_byte1_dq3_mux[3:0]
	//original      rddata[23:20] = 40;   //param_phyd_swap_byte1_dq4_mux[3:0]
	rddata = modified_bits_by_value(rddata, 0, 23, 20); //param_phyd_swap_byte1_dq4_mux[3:0]
	//original      rddata[27:24] = 44;   //param_phyd_swap_byte1_dq5_mux[3:0]
	rddata = modified_bits_by_value(rddata, 4, 27, 24); //param_phyd_swap_byte1_dq5_mux[3:0]
	//original      rddata[31:28] = 43;   //param_phyd_swap_byte1_dq6_mux[3:0]
	rddata = modified_bits_by_value(rddata, 3, 31, 28); //param_phyd_swap_byte1_dq6_mux[3:0]
	//original      REGWR  (108 + PHY_BASE_ADDR, rddata, 0);
	mmio_wr32(4 * (108 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	////original      REGREAD(109 + PHY_BASE_ADDR, rddata);
	ddr_mmio_rd32(4 * (109 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	//original      rddata[3:0]   = 42;   //param_phyd_swap_byte1_dq7_mux[3:0]
	rddata = modified_bits_by_value(rddata, 2, 3, 0); //param_phyd_swap_byte1_dq7_mux[3:0]
	//original      REGWR  (109 + PHY_BASE_ADDR, rddata, 0);
	mmio_wr32(4 * (109 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	////original      REGREAD(110 + PHY_BASE_ADDR, rddata);
	ddr_mmio_rd32(4 * (110 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	//original      rddata[3:0]   = 46;   //param_phyd_swap_byte2_dm_mux[3:0]
	rddata = modified_bits_by_value(rddata, 6, 3, 0); //param_phyd_swap_byte2_dm_mux[3:0]
	//original      rddata[7:4]   = 45;   //param_phyd_swap_byte2_dq0_mux[3:0]
	rddata = modified_bits_by_value(rddata, 5, 7, 4); //param_phyd_swap_byte2_dq0_mux[3:0]
	//original      rddata[11:8]  = 41;   //param_phyd_swap_byte2_dq1_mux[3:0]
	rddata = modified_bits_by_value(rddata, 1, 11, 8); //param_phyd_swap_byte2_dq1_mux[3:0]
	//original      rddata[15:12] = 42;   //param_phyd_swap_byte2_dq2_mux[3:0]
	rddata = modified_bits_by_value(rddata, 2, 15, 12); //param_phyd_swap_byte2_dq2_mux[3:0]
	//original      rddata[19:16] = 48;   //param_phyd_swap_byte2_dq3_mux[3:0]
	rddata = modified_bits_by_value(rddata, 8, 19, 16); //param_phyd_swap_byte2_dq3_mux[3:0]
	//original      rddata[23:20] = 47;   //param_phyd_swap_byte2_dq4_mux[3:0]
	rddata = modified_bits_by_value(rddata, 7, 23, 20); //param_phyd_swap_byte2_dq4_mux[3:0]
	//original      rddata[27:24] = 43;   //param_phyd_swap_byte2_dq5_mux[3:0]
	rddata = modified_bits_by_value(rddata, 3, 27, 24); //param_phyd_swap_byte2_dq5_mux[3:0]
	//original      rddata[31:28] = 40;   //param_phyd_swap_byte2_dq6_mux[3:0]
	rddata = modified_bits_by_value(rddata, 0, 31, 28); //param_phyd_swap_byte2_dq6_mux[3:0]
	//original      REGWR  (110 + PHY_BASE_ADDR, rddata, 0);
	mmio_wr32(4 * (110 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	////original      REGREAD(111 + PHY_BASE_ADDR, rddata);
	ddr_mmio_rd32(4 * (111 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	//original      rddata[3:0]   = 44;   //param_phyd_swap_byte2_dq7_mux[3:0]
	rddata = modified_bits_by_value(rddata, 4, 3, 0); //param_phyd_swap_byte2_dq7_mux[3:0]
	//original      REGWR  (111 + PHY_BASE_ADDR, rddata, 0);
	mmio_wr32(4 * (111 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	////original      REGREAD(112 + PHY_BASE_ADDR, rddata);
	ddr_mmio_rd32(4 * (112 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	//original      rddata[3:0]   = 47;   //param_phyd_swap_byte3_dm_mux[3:0]
	rddata = modified_bits_by_value(rddata, 7, 3, 0); //param_phyd_swap_byte3_dm_mux[3:0]
	//original      rddata[7:4]   = 43;   //param_phyd_swap_byte3_dq0_mux[3:0]
	rddata = modified_bits_by_value(rddata, 3, 7, 4); //param_phyd_swap_byte3_dq0_mux[3:0]
	//original      rddata[11:8]  = 45;   //param_phyd_swap_byte3_dq1_mux[3:0]
	rddata = modified_bits_by_value(rddata, 5, 11, 8); //param_phyd_swap_byte3_dq1_mux[3:0]
	//original      rddata[15:12] = 40;   //param_phyd_swap_byte3_dq2_mux[3:0]
	rddata = modified_bits_by_value(rddata, 0, 15, 12); //param_phyd_swap_byte3_dq2_mux[3:0]
	//original      rddata[19:16] = 46;   //param_phyd_swap_byte3_dq3_mux[3:0]
	rddata = modified_bits_by_value(rddata, 6, 19, 16); //param_phyd_swap_byte3_dq3_mux[3:0]
	//original      rddata[23:20] = 44;   //param_phyd_swap_byte3_dq4_mux[3:0]
	rddata = modified_bits_by_value(rddata, 4, 23, 20); //param_phyd_swap_byte3_dq4_mux[3:0]
	//original      rddata[27:24] = 41;   //param_phyd_swap_byte3_dq5_mux[3:0]
	rddata = modified_bits_by_value(rddata, 1, 27, 24); //param_phyd_swap_byte3_dq5_mux[3:0]
	//original      rddata[31:28] = 48;   //param_phyd_swap_byte3_dq6_mux[3:0]
	rddata = modified_bits_by_value(rddata, 8, 31, 28); //param_phyd_swap_byte3_dq6_mux[3:0]
	//original      REGWR  (112 + PHY_BASE_ADDR, rddata, 0);
	mmio_wr32(4 * (112 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	////original      REGREAD(113 + PHY_BASE_ADDR, rddata);
	ddr_mmio_rd32(4 * (113 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	//original      rddata[3:0]   = 42;   //param_phyd_swap_byte3_dq7_mux[3:0]
	rddata = modified_bits_by_value(rddata, 2, 3, 0); //param_phyd_swap_byte3_dq7_mux[3:0]
	//original      REGWR  (113 + PHY_BASE_ADDR, rddata, 0);
	mmio_wr32(4 * (113 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	////original      REGREAD(126 + PHY_BASE_ADDR, rddata);
	ddr_mmio_rd32(4 * (126 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	//original      rddata[4:0]   = 52;   //param_phyd_swap_ca0[4:0]
	rddata = modified_bits_by_value(rddata, 2, 4, 0); //param_phyd_swap_ca0[4:0]
	//original      rddata[12:8]  = 54;   //param_phyd_swap_ca1[4:0]
	rddata = modified_bits_by_value(rddata, 4, 12, 8); //param_phyd_swap_ca1[4:0]
	//original      rddata[20:16] = 54;   //param_phyd_swap_ca2[4:0]
	rddata = modified_bits_by_value(rddata, 4, 20, 16); //param_phyd_swap_ca2[4:0]
	//original      rddata[28:24] = 53;   //param_phyd_swap_ca3[4:0]
	rddata = modified_bits_by_value(rddata, 3, 28, 24); //param_phyd_swap_ca3[4:0]
	//original      REGWR  (126 + PHY_BASE_ADDR, rddata, 0);
	mmio_wr32(4 * (126 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	////original      REGREAD(127 + PHY_BASE_ADDR, rddata);
	ddr_mmio_rd32(4 * (127 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	//original      rddata[4:0]   = 512;   //param_phyd_swap_ca4[4:0]
	rddata = modified_bits_by_value(rddata, 12, 4, 0); //param_phyd_swap_ca4[4:0]
	//original      rddata[12:8]  = 511;   //param_phyd_swap_ca5[4:0]
	rddata = modified_bits_by_value(rddata, 11, 12, 8); //param_phyd_swap_ca5[4:0]
	//original      rddata[20:16] = 55;   //param_phyd_swap_ca6[4:0]
	rddata = modified_bits_by_value(rddata, 5, 20, 16); //param_phyd_swap_ca6[4:0]
	//original      rddata[28:24] = 55;   //param_phyd_swap_ca7[4:0]
	rddata = modified_bits_by_value(rddata, 5, 28, 24); //param_phyd_swap_ca7[4:0]
	//original      REGWR  (127 + PHY_BASE_ADDR, rddata, 0);
	mmio_wr32(4 * (127 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	////original      REGREAD(128 + PHY_BASE_ADDR, rddata);
	ddr_mmio_rd32(4 * (128 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	//original      rddata[4:0]   = 51;   //param_phyd_swap_ca8[4:0]
	rddata = modified_bits_by_value(rddata, 1, 4, 0); //param_phyd_swap_ca8[4:0]
	//original      rddata[12:8]  = 512;   //param_phyd_swap_ca9[4:0]
	rddata = modified_bits_by_value(rddata, 12, 12, 8); //param_phyd_swap_ca9[4:0]
	//original      rddata[20:16] = 510;   //param_phyd_swap_ca10[4:0]
	rddata = modified_bits_by_value(rddata, 10, 20, 16); //param_phyd_swap_ca10[4:0]
	//original      rddata[28:24] = 512;   //param_phyd_swap_ca11[4:0]
	rddata = modified_bits_by_value(rddata, 12, 28, 24); //param_phyd_swap_ca11[4:0]
	//original      REGWR  (128 + PHY_BASE_ADDR, rddata, 0);
	mmio_wr32(4 * (128 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	////original      REGREAD(129 + PHY_BASE_ADDR, rddata);
	ddr_mmio_rd32(4 * (129 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	//original      rddata[4:0]   = 510;   //param_phyd_swap_ca12[4:0]
	rddata = modified_bits_by_value(rddata, 10, 4, 0); //param_phyd_swap_ca12[4:0]
	//original      rddata[12:8]  = 58;   //param_phyd_swap_ca13[4:0]
	rddata = modified_bits_by_value(rddata, 8, 12, 8); //param_phyd_swap_ca13[4:0]
	//original      rddata[20:16] = 512;   //param_phyd_swap_ca14[4:0]
	rddata = modified_bits_by_value(rddata, 12, 20, 16); //param_phyd_swap_ca14[4:0]
	//original      rddata[28:24] = 512;   //param_phyd_swap_ca15[4:0]
	rddata = modified_bits_by_value(rddata, 12, 28, 24); //param_phyd_swap_ca15[4:0]
	//original      REGWR  (129 + PHY_BASE_ADDR, rddata, 0);
	mmio_wr32(4 * (129 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	////original      REGREAD(130 + PHY_BASE_ADDR, rddata);
	ddr_mmio_rd32(4 * (130 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	//original      rddata[4:0]   = 512;   //param_phyd_swap_ca16[4:0]
	rddata = modified_bits_by_value(rddata, 12, 4, 0); //param_phyd_swap_ca16[4:0]
	//original      rddata[12:8]  = 59;   //param_phyd_swap_ca17[4:0]
	rddata = modified_bits_by_value(rddata, 9, 12, 8); //param_phyd_swap_ca17[4:0]
	//original      rddata[20:16] = 57;   //param_phyd_swap_ca18[4:0]
	rddata = modified_bits_by_value(rddata, 7, 20, 16); //param_phyd_swap_ca18[4:0]
	//original      rddata[28:24] = 50;   //param_phyd_swap_ca19[4:0]
	rddata = modified_bits_by_value(rddata, 0, 28, 24); //param_phyd_swap_ca19[4:0]
	//original      REGWR  (130 + PHY_BASE_ADDR, rddata, 0);
	mmio_wr32(4 * (130 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	////original      REGREAD(131 + PHY_BASE_ADDR, rddata);
	ddr_mmio_rd32(4 * (131 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	//original      rddata[4:0]   = 51;   //param_phyd_swap_ca20[4:0]
	rddata = modified_bits_by_value(rddata, 1, 4, 0); //param_phyd_swap_ca20[4:0]
	//original      rddata[12:8]  = 56;   //param_phyd_swap_ca21[4:0]
	rddata = modified_bits_by_value(rddata, 6, 12, 8); //param_phyd_swap_ca21[4:0]
	//original      rddata[20:16] = 512;   //param_phyd_swap_ca22[4:0]
	rddata = modified_bits_by_value(rddata, 12, 20, 16); //param_phyd_swap_ca22[4:0]
	//original      rddata[28:24] = 56;   //param_phyd_swap_ca23[4:0]
	rddata = modified_bits_by_value(rddata, 6, 28, 24); //param_phyd_swap_ca23[4:0]
	//original      REGWR  (131 + PHY_BASE_ADDR, rddata, 0);
	mmio_wr32(4 * (131 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	////original      REGREAD(141 + PHY_BASE_ADDR, rddata);
	ddr_mmio_rd32(4 * (141 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	//original      rddata[15]    = 0b0;   //param_phyd_swap_cke0
	rddata = modified_bits_by_value(rddata, 0b0, 15, 15); //
	//original      rddata[16]    = 0b1;   //param_phyd_swap_cke1
	rddata = modified_bits_by_value(rddata, 0b1, 16, 16); //
	//original      rddata[17]    = 0b0;   //param_phyd_swap_cs0
	rddata = modified_bits_by_value(rddata, 0b0, 17, 17); //
	//original      rddata[19]    = 0b1;   //param_phyd_swap_cs1
	rddata = modified_bits_by_value(rddata, 0b1, 19, 19); //
	//original      REGWR  (141 + PHY_BASE_ADDR, rddata, 0);
	mmio_wr32(4 * (141 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	uartlog("[DBG] KC, pin mux setting }\n");
	//LP4
#elif defined(DDR3)
	uartlog("[DBG] KC, DDR3_CS pin mux setting \n");
	ddr_mmio_rd32(4 * (89 + PI_BASE_ADDR) + CADENCE_PHYD, rddata);
	rddata = modified_bits_by_value(rddata, 1, 0, 0); //
	rddata = modified_bits_by_value(rddata, 0, 9, 8); //param_pi_data_byte_swap_slice0
	rddata = modified_bits_by_value(rddata, 2, 17, 16); //param_pi_data_byte_swap_slice1
	rddata = modified_bits_by_value(rddata, 3, 25, 24); //param_pi_data_byte_swap_slice2
	mmio_wr32(4 * (89 + PI_BASE_ADDR) + CADENCE_PHYD, rddata);
	ddr_mmio_rd32(4 * (90 + PI_BASE_ADDR) + CADENCE_PHYD, rddata);
	rddata = modified_bits_by_value(rddata, 1, 1, 0); //param_pi_data_byte_swap_slice3
	mmio_wr32(4 * (90 + PI_BASE_ADDR) + CADENCE_PHYD, rddata);
	ddr_mmio_rd32(4 * (106 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	rddata = modified_bits_by_value(rddata, 8, 3, 0); //param_phyd_swap_byte0_dm_mux[3:0]
	rddata = modified_bits_by_value(rddata, 4, 7, 4); //param_phyd_swap_byte0_dq0_mux[3:0]
	rddata = modified_bits_by_value(rddata, 1, 11, 8); //param_phyd_swap_byte0_dq1_mux[3:0]
	rddata = modified_bits_by_value(rddata, 3, 15, 12); //param_phyd_swap_byte0_dq2_mux[3:0]
	rddata = modified_bits_by_value(rddata, 7, 19, 16); //param_phyd_swap_byte0_dq3_mux[3:0]
	rddata = modified_bits_by_value(rddata, 5, 23, 20); //param_phyd_swap_byte0_dq4_mux[3:0]
	rddata = modified_bits_by_value(rddata, 6, 27, 24); //param_phyd_swap_byte0_dq5_mux[3:0]
	rddata = modified_bits_by_value(rddata, 2, 31, 28); //param_phyd_swap_byte0_dq6_mux[3:0]
	mmio_wr32(4 * (106 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	ddr_mmio_rd32(4 * (107 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	rddata = modified_bits_by_value(rddata, 0, 3, 0); //param_phyd_swap_byte0_dq7_mux[3:0]
	mmio_wr32(4 * (107 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	ddr_mmio_rd32(4 * (108 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	rddata = modified_bits_by_value(rddata, 7, 3, 0); //param_phyd_swap_byte1_dm_mux[3:0]
	rddata = modified_bits_by_value(rddata, 3, 7, 4); //param_phyd_swap_byte1_dq0_mux[3:0]
	rddata = modified_bits_by_value(rddata, 1, 11, 8); //param_phyd_swap_byte1_dq1_mux[3:0]
	rddata = modified_bits_by_value(rddata, 4, 15, 12); //param_phyd_swap_byte1_dq2_mux[3:0]
	rddata = modified_bits_by_value(rddata, 0, 19, 16); //param_phyd_swap_byte1_dq3_mux[3:0]
	rddata = modified_bits_by_value(rddata, 5, 23, 20); //param_phyd_swap_byte1_dq4_mux[3:0]
	rddata = modified_bits_by_value(rddata, 2, 27, 24); //param_phyd_swap_byte1_dq5_mux[3:0]
	rddata = modified_bits_by_value(rddata, 6, 31, 28); //param_phyd_swap_byte1_dq6_mux[3:0]
	mmio_wr32(4 * (108 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	ddr_mmio_rd32(4 * (109 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	rddata = modified_bits_by_value(rddata, 8, 3, 0); //param_phyd_swap_byte1_dq7_mux[3:0]
	mmio_wr32(4 * (109 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	ddr_mmio_rd32(4 * (110 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	rddata = modified_bits_by_value(rddata, 5, 3, 0); //param_phyd_swap_byte2_dm_mux[3:0]
	rddata = modified_bits_by_value(rddata, 8, 7, 4); //param_phyd_swap_byte2_dq0_mux[3:0]
	rddata = modified_bits_by_value(rddata, 1, 11, 8); //param_phyd_swap_byte2_dq1_mux[3:0]
	rddata = modified_bits_by_value(rddata, 4, 15, 12); //param_phyd_swap_byte2_dq2_mux[3:0]
	rddata = modified_bits_by_value(rddata, 3, 19, 16); //param_phyd_swap_byte2_dq3_mux[3:0]
	rddata = modified_bits_by_value(rddata, 7, 23, 20); //param_phyd_swap_byte2_dq4_mux[3:0]
	rddata = modified_bits_by_value(rddata, 2, 27, 24); //param_phyd_swap_byte2_dq5_mux[3:0]
	rddata = modified_bits_by_value(rddata, 0, 31, 28); //param_phyd_swap_byte2_dq6_mux[3:0]
	mmio_wr32(4 * (110 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	ddr_mmio_rd32(4 * (111 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	rddata = modified_bits_by_value(rddata, 6, 3, 0); //param_phyd_swap_byte2_dq7_mux[3:0]
	mmio_wr32(4 * (111 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	ddr_mmio_rd32(4 * (112 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	rddata = modified_bits_by_value(rddata, 0, 3, 0); //param_phyd_swap_byte3_dm_mux[3:0]
	rddata = modified_bits_by_value(rddata, 6, 7, 4); //param_phyd_swap_byte3_dq0_mux[3:0]
	rddata = modified_bits_by_value(rddata, 1, 11, 8); //param_phyd_swap_byte3_dq1_mux[3:0]
	rddata = modified_bits_by_value(rddata, 4, 15, 12); //param_phyd_swap_byte3_dq2_mux[3:0]
	rddata = modified_bits_by_value(rddata, 7, 19, 16); //param_phyd_swap_byte3_dq3_mux[3:0]
	rddata = modified_bits_by_value(rddata, 5, 23, 20); //param_phyd_swap_byte3_dq4_mux[3:0]
	rddata = modified_bits_by_value(rddata, 8, 27, 24); //param_phyd_swap_byte3_dq5_mux[3:0]
	rddata = modified_bits_by_value(rddata, 3, 31, 28); //param_phyd_swap_byte3_dq6_mux[3:0]
	mmio_wr32(4 * (112 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	ddr_mmio_rd32(4 * (113 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	rddata = modified_bits_by_value(rddata, 2, 3, 0); //param_phyd_swap_byte3_dq7_mux[3:0]
	mmio_wr32(4 * (113 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	ddr_mmio_rd32(4 * (126 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	rddata = modified_bits_by_value(rddata, 8, 4, 0); //param_phyd_swap_ca0[4:0]
	rddata = modified_bits_by_value(rddata, 5, 12, 8); //param_phyd_swap_ca1[4:0]
	rddata = modified_bits_by_value(rddata, 23, 20, 16); //param_phyd_swap_ca2[4:0]
	rddata = modified_bits_by_value(rddata, 7, 28, 24); //param_phyd_swap_ca3[4:0]
	mmio_wr32(4 * (126 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	ddr_mmio_rd32(4 * (127 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	rddata = modified_bits_by_value(rddata, 9, 4, 0); //param_phyd_swap_ca4[4:0]
	rddata = modified_bits_by_value(rddata, 14, 12, 8); //param_phyd_swap_ca5[4:0]
	rddata = modified_bits_by_value(rddata, 2, 20, 16); //param_phyd_swap_ca6[4:0]
	rddata = modified_bits_by_value(rddata, 11, 28, 24); //param_phyd_swap_ca7[4:0]
	mmio_wr32(4 * (127 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	ddr_mmio_rd32(4 * (128 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	rddata = modified_bits_by_value(rddata, 19, 4, 0); //param_phyd_swap_ca8[4:0]
	rddata = modified_bits_by_value(rddata, 13, 12, 8); //param_phyd_swap_ca9[4:0]
	rddata = modified_bits_by_value(rddata, 3, 20, 16); //param_phyd_swap_ca10[4:0]
	rddata = modified_bits_by_value(rddata, 0, 28, 24); //param_phyd_swap_ca11[4:0]
	mmio_wr32(4 * (128 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	ddr_mmio_rd32(4 * (129 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	rddata = modified_bits_by_value(rddata, 6, 4, 0); //param_phyd_swap_ca12[4:0]
	rddata = modified_bits_by_value(rddata, 1, 12, 8); //param_phyd_swap_ca13[4:0]
	rddata = modified_bits_by_value(rddata, 15, 20, 16); //param_phyd_swap_ca14[4:0]
	rddata = modified_bits_by_value(rddata, 4, 28, 24); //param_phyd_swap_ca15[4:0]
	mmio_wr32(4 * (129 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	ddr_mmio_rd32(4 * (130 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	rddata = modified_bits_by_value(rddata, 10, 4, 0); //param_phyd_swap_ca16[4:0]
	rddata = modified_bits_by_value(rddata, 12, 12, 8); //param_phyd_swap_ca17[4:0]
	rddata = modified_bits_by_value(rddata, 20, 20, 16); //param_phyd_swap_ca18[4:0]
	rddata = modified_bits_by_value(rddata, 18, 28, 24); //param_phyd_swap_ca19[4:0]
	mmio_wr32(4 * (130 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	ddr_mmio_rd32(4 * (131 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	rddata = modified_bits_by_value(rddata, 17, 4, 0); //param_phyd_swap_ca20[4:0]
	rddata = modified_bits_by_value(rddata, 21, 12, 8); //param_phyd_swap_ca21[4:0]
	rddata = modified_bits_by_value(rddata, 22, 20, 16); //param_phyd_swap_ca22[4:0]
	rddata = modified_bits_by_value(rddata, 16, 28, 24); //param_phyd_swap_ca23[4:0]
	mmio_wr32(4 * (131 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	ddr_mmio_rd32(4 * (141 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	rddata = modified_bits_by_value(rddata, 0b0, 15, 15); //
	rddata = modified_bits_by_value(rddata, 0b1, 16, 16); //
	rddata = modified_bits_by_value(rddata, 0b0, 17, 17); //
	rddata = modified_bits_by_value(rddata, 0b1, 19, 19); //
	mmio_wr32(4 * (141 + PHY_BASE_ADDR) + CADENCE_PHYD, rddata);
	uartlog("[DBG] KC, pin mux setting } \n");
#endif //DDR3
}

void show_ddr_info(void)
{
	extern const char fw_version_info[];
	// extern const char built_time_info[];

	info("BLD:  ver: %s\n", fw_version_info);
	// info("BLD:  built time:%s\n", built_time_info);

	info("BLD:  ");

#ifdef DDR3
	info("DDR3");
#elif defined(LP4)
	info("LPDDR4");
#elif defined(DDR4)
	info("DDR4");
#endif

	info("-%d", ddr_data_rate);

#ifdef ODT
	info("-ODT");
#endif

#ifdef DBI_OFF
	info("-DBI_OFF");
#endif

	info("\n");

}

typedef void func(void);
int plat_bm_ddr_init(void)
{
	func *warmboot_entry;

	// from blp
	mmio_wr32(0x03002040, 0x00010009); // set cpu clock to PLL(MPLL) div by 1 = 1HGz
	mmio_wr32(0x03002048, 0x00020009); // set ap_fab clock to PLL(MIPIPLL) div 2 = 600MHz
	mmio_wr32(0x03002054, 0x00010009); // set tpu clock to PLL(TPLL) div by 1 = 800MHz
	mmio_wr32(0x0300205C, 0x00020009); // set tpu_fab clock to PLL (MIPIPLL) div by 2 = 600MHz
	mmio_wr32(0x03002030, 0x00000000); // switch clock to PLL from xtal
	mmio_wr32(0x03002034, 0x00000000); // switch clock to PLL from xtal

	show_ddr_info();

	// sys_pll_init();

	warmboot_entry = (func *)get_warmboot_entry();

	if (warmboot_entry) {
		/* TODO: restore ddr training data */
		/* TODO: leave ddr self refresh mode */

		restore_trusted_mailbox();
		warmboot_entry();
	}

#ifdef SUBTYPE_FPGA
#define MSG_BASE_ADDR 0x0C000000
	// #define MSG_BASE_ADDR 0x0E01F000
	write_to_mem(MSG_BASE_ADDR + 0x00, 'B');
	write_to_mem(MSG_BASE_ADDR + 0x04, 'I');
	write_to_mem(MSG_BASE_ADDR + 0x08, 'T');
	write_to_mem(MSG_BASE_ADDR + 0x0C, 'M');
	write_to_mem(MSG_BASE_ADDR + 0x10, 'A');
	write_to_mem(MSG_BASE_ADDR + 0x14, 'I');
	write_to_mem(MSG_BASE_ADDR + 0x18, 'N');
	return 0;
#endif

	DDR_reset();

	mmio_wr32(PLLG6_BASE + top_pll_g6_reg_ddr_ssc_syn_src_en,
		  (mmio_rd32(PLLG6_BASE + top_pll_g6_reg_ddr_ssc_syn_src_en)
		   & (~top_pll_g6_reg_ddr_ssc_syn_src_en_MASK))
		  | (0x1 << top_pll_g6_reg_ddr_ssc_syn_src_en_OFFSET));

	uartlog("PLL INIT !\n");
	extern void phy_pll_init(void);
	phy_pll_init();

	uartlog("DDRC_INIT !\n");
	ddrc_init();

	// turn off (auto self refresh, auto power down)
	read_data = mmio_rd32(cfg_base + 0x30);
	read_data = modified_bits_by_value(read_data, 0, 3, 3);
	read_data = modified_bits_by_value(read_data, 0, 1, 1);
	read_data = modified_bits_by_value(read_data, 0, 0, 0);
	mmio_wr32(cfg_base + 0x30, read_data);

	// ctrlupd_short();

	// release ddrc soft reset
	uartlog("releast reset  !\n");
	mmio_wr32(DDR_TOP_BASE + 0x20, 0x0);

	// set m1 m2 QOS = 8
	mmio_wr32(0x030001D8, 0x44008888);

#ifndef SUBTYPE_PALLADIUM
	uartlog(" pi_init !\n");
	pi_init();
	uartlog(" phy_init !\n");
	phy_init();
#endif
	ddr_pinmux();
	ddr_patch_set();

	uartlog(" bm_sync_mr_to_pi !\n");
	bm_sync_mr_to_pi();
	uartlog(" ddr_phy_sync_dfi_timing !\n");
	ddr_phy_sync_dfi_timing();

	setting_check();
	uartlog("setting_check  finish\n");
	pi_reg_adj();
	uartlog("pi_reg_adj finish\n");

	//set_dfi_init_start
	set_dfi_init_start();
	uartlog("set_dfi_init_start finish\n");

	uartlog(" ddr_phy_power_on_seq1 !\n");
	ddr_phy_power_on_seq1();
	uartlog("ddr_phy_power_on_seq1 finish\n");
#ifdef LP4
	change_to_calvl_freq();
	uartlog("change_to_calvl_freq finish\n");
#endif
	uartlog(" polling_dfi_init_start !\n");
	polling_dfi_init_start();
	uartlog(" INT_ISR_08 !\n");
	INT_ISR_08();
	uartlog(" ddr_phy_power_on_seq3 !\n");
	ddr_phy_power_on_seq3();
	uartlog(" wait_for_dfi_init_complete !\n");
	wait_for_dfi_init_complete();
	uartlog(" polling_synp_normal_mode !\n");
	polling_synp_normal_mode();

	uartlog("polling_synp_normal_mode finish\n");
	bm_soft_mr();
	//bm_soft_zq
	bm_soft_zq();
	uartlog("bm_soft_zq finish\n");
	// Training request
	set_pi_start();
	uartlog("set_pi_start finish\n");
	// wait_for_pi_training_done
	wait_for_pi_training_done();
	uartlog("wait_for_pi_training_done  finish\n");
	// if ((dram_class == {`PI_DDR4_CLASS,1'b0}) || (dram_class == {`PI_DDR5_CLASS,1'b0})) begin
	//     success = $mmtcleval("mmsetvar RefreshChecks 1");
	//     uartlog("Enable RefreshChecks...");
	// end
	uartlog("PI Initial Training Finished\n");
	ctrl_init_high_patch();

#if defined(LP4)
	// pi_wrlvl_status();
	pi_wrlvl_req();
	rdgtlvl();
	// pi_rdlvl_gate_status();
	// pi_rdlvl_gate_req();
	// pi_rdlvl_status();
	pi_rdlvl_req();
	// pi_wdqlvl_status();
	//pi_wdqlvl_req();
#endif //

#if 0
	//pi_wrlvl_req
	pi_wrlvl_req();
	uartlog("pi_wrlvl_req finish\n");
	bm_bist_wr_prbs_init();
	bm_bist_start_check(&bist_result, &err_data_odd, &err_data_even);
	uartlog("[KC_DBG] , bist_result = %x, err_data_odd = %x, err_data_even = %x\n", bist_result, err_data_odd, err_data_even);
	if (bist_result == 0)
		uartlog("[KC_DBG] ERROR bist_fail\n");
	//pi_rdlvl_gate_req
	pi_rdlvl_gate_req();
	uartlog("pi_rdlvl_gate_req finish\n");
	bm_bist_wr_prbs_init();
	bm_bist_start_check(&bist_result, &err_data_odd, &err_data_even);
	uartlog("[KC_DBG] , bist_result = %x, err_data_odd = %x, err_data_even = %x\n", bist_result, err_data_odd, err_data_even);
	if (bist_result == 0)
		uartlog("[KC_DBG] ERROR bist_fail\n");
	//pi_rdlvl_req
	pi_rdlvl_req();
	uartlog("pi_rdlvl_req finish\n");
	bm_bist_wr_prbs_init();
	bm_bist_start_check(&bist_result, &err_data_odd, &err_data_even);
	uartlog("[KC_DBG] , bist_result = %x, err_data_odd = %x, err_data_even = %x\n", bist_result, err_data_odd, err_data_even);
	if (bist_result == 0)
		uartlog("[KC_DBG] ERROR bist_fail\n");
	//pi_wdqlvl_req
	pi_wdqlvl_req();
	uartlog("pi_wdqlvl_req finish\n");
#endif //0

#if 1
	bm_bist_wr_prbs_init();
	for (int i = 0; i < 10; i++) {
	#if defined(LP4)
		pi_wdqlvl_req();
	#endif
		bm_bist_start_check(&bist_result, &err_data_odd, &err_data_even);
		info("BLD:  bist_result/odd/even=%x/%lx/%lx\n", bist_result, err_data_odd, err_data_even);
		if (bist_result)
			break;
	}
	if (bist_result == 0) {
		info("BLD:  DDR BIST fail\n");
		while (1);
	} else {
		info("BLD:  DDR BIST pass\n");
	}
#endif

	// uartlog("End of ddr_main\n");
	// bm_bist_wr_sram_init();
	// while(1);

	return 0;
}
