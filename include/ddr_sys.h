#ifndef __DDR_SYS_H__
#define __DDR_SYS_H__

#include "mmio.h"

extern uint32_t  freq_in;
extern uint32_t  tar_freq;
extern uint32_t  mod_freq;
extern uint32_t  dev_freq;
extern uint64_t  reg_set;
extern uint64_t  reg_span;
extern uint64_t  reg_step;

#define ddr_mmio_rd32(a, b) b = mmio_rd32(a)

#define PHY_BASE_ADDR		2048
#define PI_BASE_ADDR		0
#define CADENCE_PHYD		0x08000000
#define CADENCE_PHYD_APB	0x08006000
#define cfg_base			0x08004000

#define DDR_SYS_BASE	0x08000000
#define PI_BASE			(DDR_SYS_BASE + 0x0000)
#define PHY_BASE		(DDR_SYS_BASE + 0x2000)
#define DDRC_BASE		(DDR_SYS_BASE + 0x4000)
#define PHYD_BASE		(DDR_SYS_BASE + 0x6000)
#define AXI_MON_BASE	(DDR_SYS_BASE + 0x8000)
#define DDR_TOP_BASE		(DDR_SYS_BASE + 0xa000)
#define DDR_BIST_BASE	0x08010000

#ifdef _mem_freq_4266
    #define mem_freq_2133 1
    #define mem_freq_1600 0
    #define mem_freq_1066 0
    #define ssc_out_freg 266
#endif
#ifdef _mem_freq_3200
    #define mem_freq_2133 0
    #define mem_freq_1600 1
    #define mem_freq_1066 0
    #define ssc_out_freg 200
#endif
#ifdef _mem_freq_2666
    #define mem_freq_2133 0
    #define mem_freq_1600 0
    #define mem_freq_1066 1
    #define ssc_out_freg 166
#endif
#ifdef _mem_freq_2400
    #define mem_freq_2133 0
    #define mem_freq_1600 1
    #define mem_freq_1066 0
    #define ssc_out_freg 150
#endif
#ifdef _mem_freq_2133
    #define mem_freq_2133 0
    #define mem_freq_1600 0
    #define mem_freq_1066 1
    #define ssc_out_freg 133
#endif
#ifdef _mem_freq_1866
    #define mem_freq_2133 0
    #define mem_freq_1600 1
    #define mem_freq_1066 0
    #define ssc_out_freg 116
#endif
#if mem_freq_2133 == 1
#define clkdiv_clksel 0b00
#elif mem_freq_1600 == 1
#define clkdiv_clksel 0b10
#elif mem_freq_1066 == 1
#define clkdiv_clksel 0b11
#else
#define clkdiv_clksel 0b00
#endif

extern uint32_t  int_sts_keep;
extern uint32_t  lpddr4_calvl_done;
extern uint32_t  CUR_PLL_SPEED;
extern uint32_t  NEXT_PLL_SPEED;
extern uint32_t  EN_PLL_SPEED_CHG;
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
extern uint32_t  lpddr4_bit0_pass_save;
extern uint32_t  lpddr4_bit1_pass_save;
extern uint32_t  lpddr4_bit2_pass_save;
extern uint32_t  lpddr4_bit3_pass_save;
extern uint32_t  lpddr4_bit4_pass_save;
extern uint32_t  lpddr4_bit5_pass_save;
extern uint32_t  lpddr4_bit6_pass_save;
extern uint32_t  lpddr4_bit7_pass_save;
extern uint32_t  lpddr4_bit8_pass_save;
extern uint32_t  lpddr4_bit9_pass_save;
extern uint32_t  lpddr4_bit0_min_pass_save;
extern uint32_t  lpddr4_bit1_min_pass_save;
extern uint32_t  lpddr4_bit2_min_pass_save;
extern uint32_t  lpddr4_bit3_min_pass_save;
extern uint32_t  lpddr4_bit4_min_pass_save;
extern uint32_t  lpddr4_bit5_min_pass_save;
extern uint32_t  lpddr4_bit6_min_pass_save;
extern uint32_t  lpddr4_bit7_min_pass_save;
extern uint32_t  lpddr4_bit8_min_pass_save;
extern uint32_t  lpddr4_bit9_min_pass_save;
extern uint32_t  lpddr4_bit0_max_pass_save;
extern uint32_t  lpddr4_bit1_max_pass_save;
extern uint32_t  lpddr4_bit2_max_pass_save;
extern uint32_t  lpddr4_bit3_max_pass_save;
extern uint32_t  lpddr4_bit4_max_pass_save;
extern uint32_t  lpddr4_bit5_max_pass_save;
extern uint32_t  lpddr4_bit6_max_pass_save;
extern uint32_t  lpddr4_bit7_max_pass_save;
extern uint32_t  lpddr4_bit8_max_pass_save;
extern uint32_t  lpddr4_bit9_max_pass_save;
extern uint32_t  read_temp;
// uint32_t  rddata_ctrl;
// uint32_t  rddata_addr;
extern uint32_t  dram_class;
// uint32_t  pi_init_work_freq;
extern uint32_t  apb_rddata;
// uint32_t  axi_rddata;
extern uint32_t  axi_rddata_;
extern uint32_t  rddata;
extern uint32_t  ken_rddata;
extern uint32_t  read_data;
extern uint32_t  axi_rddata;
extern uint32_t  axi_rddata1;
extern uint32_t  axi_rddata2;
extern uint32_t  axi_rddata3;
extern uint32_t  axi_rddata4;
extern uint32_t  axi_rddata6;
extern uint32_t  axi_rddata7;
extern uint32_t  axi_rddata8;
extern uint32_t  bist_result;
extern uint64_t  err_data_odd;
extern uint64_t  err_data_even;

void bm_mrw(uint32_t mr_num, uint32_t mr_wrdata, uint32_t mr_cs, uint32_t mr_mode);
void bm_pi_reinit(void);
void bm_synp_mrr_lp4(uint32_t addr);
void bm_bist_rw_prbs(void);
void bm_bist_wr_prbs_init(void);
void bm_bist_start_check(uint32_t *bist_result, uint64_t *err_data_odd, uint64_t *err_data_even);
void bm_bist_rd_prbs_init(void);
void bm_bist_rx_delay(uint32_t delay);
void bm_bist_rx_deskew_delay(uint32_t delay);
void bm_dqsosc_upd(uint32_t dqsosc_offset);
void bm_synp_mrw_lp4(uint32_t addr, uint32_t data);
void bm_synp_mrw(uint32_t addr, uint32_t data);
void bm_sync_mr_to_pi(void);
void bm_synp_mrs_ddr3_ddr4(uint32_t addr, uint32_t data);
void bm_soft_zq(void);
void bm_soft_mr(void);
void chg_pll_freq(void);
void dll_cal_status(void);
void dll_cal(void);
void ddr_zqcal_hw_isr8(void);
void ddr_zqcal_isr8(void);
void clk_normal(void);
void clk_div2(void);
void INT_ISR_02(void);
void INT_ISR_08(void);
void INT_ISR_00(void);
void INT_ISR_01(void);
void isr_process(void);
void clk_div40(void);
void ddr_phy_sync_dfi_timing(void);
void ddr_phy_power_on_seq1(void);
void ddr_phy_power_on_seq2(void);
void ddr_phy_power_on_seq3(void);
void wait_for_dfi_init_complete(void);
void ctrlupd_short(void);
void wait_for_pi_training_done(void);
void polling_dfi_init_start(void);
void set_dfi_init_complete(void);
void polling_synp_normal_mode(void);
void set_pi_start(void);
void pi_reg_adj(void);
void pi_wrlvl_req(void);
void pi_rdlvl_gate_req(void);
void pi_rdlvl_req(void);
void pi_wdqlvl_req(void);
void pi_wrlvl_status(void);
void pi_rdlvl_gate_status(void);
void pi_rdlvl_status(void);
void pi_wdqlvl_status(void);
void dll_cal_status(void);
void zqcal_status(void);
void setting_check(void);
void change_to_calvl_freq(void);
void check_rd32(uintptr_t addr, uint32_t expected);
void ddr_sys_init(void);
void pll_init(void);
void ddr_sys_bring_up(void);
void pll_init(void);
void change_to_calvl_freq(void);
void zq_cal_var(void);
void set_dfi_init_start(void);
void bm_bist_wr_sram_init(void);
void rdgtlvl(void);
void ddr_freq_change_htol(void);
void ddr_freq_change_ltoh(void);

#endif /* __DDR_SYS_H__ */
