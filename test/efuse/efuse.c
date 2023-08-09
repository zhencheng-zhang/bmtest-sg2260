#include "system_common.h"
#include "efuse.h"

/*
 * Organization of EFUSE_ADR register:
 *   [  i:   0]: i <= 6, address of 32-bit cell
 *   [i+6: i+1]: 5-bit bit index within the cell
 */

#define NUM_ADDRESS_BITS 7

#define EFUSE_BASE 0x50028000
static const u64 EFUSE_MODE = EFUSE_BASE;
static const u64 EFUSE_ADR = EFUSE_BASE + 0x4;
static const u64 EFUSE_RD_DATA = EFUSE_BASE + 0xc;
static const u64 EFUSE_ECCSRAM_ADR = EFUSE_BASE + 0x10;
static const u64 EFUSE_ECCSRAM_RDPORT = EFUSE_BASE + 0x14;



static void efuse_mode_md_write(u32 val)
{
  u32 mode = cpu_read32(EFUSE_MODE);
  u32 new = (mode & 0xfffffffc) | (val & 0b11);
  cpu_write32(EFUSE_MODE, new);
}

static void efuse_mode_wait_ready()
{
  while (cpu_read32(EFUSE_MODE) != 0x80)
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

static void efuse_set_bit(u32 address, u32 bit_i)
{
  efuse_mode_reset();
  u32 adr_val = make_adr_val(address, bit_i);
  cpu_write32(EFUSE_ADR, adr_val);
  efuse_mode_md_write(0b11);
  efuse_mode_wait_ready();
}

u32 efuse_embedded_read(u32 address)
{
  efuse_mode_reset();
  u32 adr_val = make_adr_val(address, 0);
  cpu_write32(EFUSE_ADR, adr_val);
  efuse_mode_md_write(0b10);
  efuse_mode_wait_ready();
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

   //read out efuse and write into sram, once is ok
   if(0 == ecc_read_cnt) {
		cpu_write32(EFUSE_MODE, (cpu_read32(EFUSE_MODE) | (0x3 << 30) | (0x1 << 28)));
		while (cpu_read32(EFUSE_MODE) & 0x80000000);
		ecc_read_cnt++;
   }
  cpu_write32(EFUSE_ECCSRAM_ADR, adr_val);
  return cpu_read32(EFUSE_ECCSRAM_RDPORT);
}



u32 efuse_num_cells()
{
  return (u32)1 << NUM_ADDRESS_BITS;
}
