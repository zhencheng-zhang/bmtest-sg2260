#include "system_common.h"
#include "efuse.h"

/*
 * Organization of EFUSE_ADR register:
 *   [  i:   0]: i <= 7, address of 32-bit cell
 *   [i+6: i+1]: 5-bit bit index within the cell
 */

#define NUM_ADDRESS_BITS 8

// verified
static const u64 EFUSE_MODE = EFUSE_BASE;
static const u64 EFUSE_ADR = EFUSE_BASE + 0x4;
static const u64 EFUSE_RD_DATA = EFUSE_BASE + 0xc;
static const u64 EFUSE_ECCSRAM_ADR = EFUSE_BASE + 0x10;
static const u64 EFUSE_ECCSRAM_RDPORT = EFUSE_BASE + 0x14;


/**
 * EFUSE_MD
  2’b00:IDLE, switch to ECC mode when ECC_READ enable.
  2’b01:DIR
  2’b10:Embedded read
  2’b11:Embedded write
*/
static void efuse_mode_md_write(u32 val)
{
  u32 mode = cpu_read32(EFUSE_MODE);
  u32 new = (mode & 0xfffffffc) | (val & 0b11);
  cpu_write32(EFUSE_MODE, new);
}

static void efuse_mode_wait_ready()
{
  // bit[7]: efuse_rdy. after reset this bit is 0
  // while (cpu_read32(EFUSE_MODE) == 0x80)
  //   ;
  while ((cpu_read32(EFUSE_MODE) & 0b11) != 0)
  ;
}

static void efuse_mode_reset()
{
  cpu_write32(EFUSE_MODE, 0);
  efuse_mode_wait_ready();
}

static u32 make_adr_val(u32 address, u32 bit_i)
{
  const u32 address_mask = (1 << NUM_ADDRESS_BITS) - 1;
  return (address & address_mask) |
      ((bit_i & 0x1f) << NUM_ADDRESS_BITS);
}

/**
 * For normal bit program use below method. 
 * Step 1 : Program EFUSE_ADR 
 * Step 2 : Program EFUSE_MODE[1:0] = 2’b11
 * Step 3 : Wait EFUSE_MODE[1:0] return back to 2’b00. 
        eFuse controller will program the bit pointed by EFUSE_ADR. 
        As finished, return EFUSE_MODE[1:0] back to 2’b00. 
*/
static void efuse_set_bit(uint32_t address, uint32_t bit_i)
{
    // efuse_mode_reset();
    u32 adr_val = make_adr_val(address, bit_i);
    cpu_write32(EFUSE_ADR, adr_val);
    efuse_mode_md_write(0b11);
    efuse_mode_wait_ready();
}

/**
 * Step 1 : Program EFUSE_ADR
  Step 2 : Program EFUSE_MODE[1:0] = 2’b10
  Step 3 : Wait EFUSE_MODE[1:0] return back to 2’b00.
  eFuse controller will read 32-bit data specified by EFUSE_ADR[6:0].
  As finished, return EFUSE_MODE[1:0] back to 2’b00.
  Step 4 : Get those 4 read out bytes at EFUSE_RD_DATA[31:0]
*/
u32 efuse_embedded_read(u32 address)
{
  efuse_mode_reset();
  u32 adr_val = make_adr_val(address, 0);
  cpu_write32(EFUSE_ADR, adr_val);
  efuse_mode_md_write(0b10);
  efuse_mode_wait_ready();
  // while (cpu_read32(EFUSE_MODE) & 0x3);
  // uartlog("--embedded read success\n");

  return cpu_read32(EFUSE_RD_DATA);
}

void efuse_embedded_write(u32 address, u32 val)
{
  for (int i = 0; i < 32; i++)
    if ((val >> i) & 1)
      efuse_set_bit(address, i);
}


u32 efuse_ecc_read(u32 address)
{
   static int ecc_read_cnt = 0;
   efuse_mode_reset();
   u32 adr_val = make_adr_val(address, 0);


  /**
   * 31 RW  0  ECC_READ.The signal which is about switching to ECC Mode.
   * 30 RW  1  ECC_EN,Enable the ECC engine to decode read out data.
   * 28 WO  1  Mask ECC data.MASK=H for original data,MASK=L for repairing data.
  */
   /**Step 1 : Make sure EFUSE_MODE[1:0] = 2’b00
      Step 1 : Program EFUSE_MODE[31:30] = 2’b11
      Step 3 : Wait EFUSE_MODE[31] return back to 1’b0.
              eFuse controller will read out the eFuse content from Address_8 ~ Address_127.
              Decode them by Hamming code N(31.26) and write the decoded data into the
              ECCSRAM(30x26).
              Those eFuse content are divided into 30 blocks. The associated ECC decoded status
              Are stored in EFUSE_ECC_STATUS0/1 registers.
      Step 4 : Read out ECCSRAM data
              Program EFUSE_ECCSRAM_ADR. Range from 0 ~ 29
              Read the EFUSE_ECCSRAM_RDPORT to retrieve the ECCSRAM content. LSB 26 bits
              are valid for each read.
    */
  if(0 == ecc_read_cnt) {
    // cpu_write32(EFUSE_MODE, (cpu_read32(EFUSE_MODE) | 0x3));
    cpu_write32(EFUSE_MODE, (cpu_read32(EFUSE_MODE) | (0x3 << 30) | 1<<28));
    // uartlog("--- write efuse mode success--\n");
    while (cpu_read32(EFUSE_MODE) & 0x80000000);
    ecc_read_cnt++;
  }

  // uartlog("---ecc read success--\n");

  cpu_write32(EFUSE_ECCSRAM_ADR, adr_val);
  return cpu_read32(EFUSE_ECCSRAM_RDPORT);
}


u32 efuse_num_cells()
{
  return (u32)1 << NUM_ADDRESS_BITS;
}