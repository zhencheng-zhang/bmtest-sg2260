#include "system_common.h"
#include "efuse_info.h"
#include "efuse.h"
#include "uart.h"
#include <string.h>

#define PLL_MAX_VALUE		825

#define CELL_TYPE_BAD_NPU         0x10
#define CELL_TYPE_SIGNATURE       0x20
#define CELL_TYPE_SERDES_PCS_L    0x30
#define CELL_TYPE_SERDES_PCS_H    0x40
#define CELL_TYPE_BOARD_MPLL      0x50
#define CELL_TYPE_BOARD_DPLL      0x60
#define CELL_TYPE_CHIP_INFO      0xF1
#define CELL_TYPE_SN_INFO      	0xF2

#define NPU_ID_INVALID 255
#define PLL_STEP		25
#define CELL_CHIP_START      0x07

static void parse_bad_npu_cell(efuse_info_t *e, u32 c)
{
  for (int i = 16; i >= 0; i -= 8) {
    u8 id = (c >> i) & 0xff;
    if (id >> 7) {
      u8 value = id & 0x7f;
      for (int i = 0; i < 2; i++) {
        if (e->bad_npus[i] == NPU_ID_INVALID) {
          e->bad_npus[i] = value;
          break;
        }
      }
    }
  }
}

static void parse_signature_cell(efuse_info_t *e, u32 c)
{
  char *sig = e->signature;
  size_t max_len = sizeof(e->signature);

  size_t len = strlen(sig);
  for (int i = 16; i >= 0 && len < max_len - 1; i -= 8) {
    char x = (c >> i) & 0xff;
    if (x != '\0')
      sig[len++] = x;
  }
  sig[len] = '\0';
}

static void parse_serdes_pcs_cell(efuse_info_t *e, u32 c)
{
  u8 index = 0;
  u8 type = c >> 24;
  if (type == CELL_TYPE_SERDES_PCS_L)
    index = 0;
  if (type == CELL_TYPE_SERDES_PCS_H)
    index = 3;

  for (int i = 0; i < 3; i++) {
    u8 pcs = (c >> (i * 8)) & 0xff;
    if (index < 5)
      e->serdes_pcs[index++] = pcs;
    else
      break;
  }
}

static void parse_chip_cell(efuse_chip_t *e, u32 c)
{
  e->flag = (c>>24)&0xff;
  e->bad_npus[1] = (c>>16)&0xff;
  e->bad_npus[0] = (c>>8)&0xff;
  e->pll = PLL_MAX_VALUE - ((c>>4)&0x0f)*PLL_STEP;
  e->result = c&0x0f;
}

static int cell_is_dead(u32 c)
{
  return c >> 31;
}

static int cell_is_empty(u32 c)
{
  return c == 0;
}

static void parse_cell(efuse_info_t *e, u32 c)
{
  if (cell_is_empty(c) || cell_is_dead(c))
    return;

  u8 type = c >> 24;
  switch (type) {
    case CELL_TYPE_BAD_NPU:
      parse_bad_npu_cell(e, c);
      break;
    case CELL_TYPE_SIGNATURE:
      parse_signature_cell(e, c);
      break;
    case CELL_TYPE_SERDES_PCS_L:
    case CELL_TYPE_SERDES_PCS_H:
      parse_serdes_pcs_cell(e, c);
      break;
  }
}

static void parse_board_freq(efuse_special_info_t *e, u32 c)
{
  u8 type = c >> 24;
  u8 index = 0;
  if (type == CELL_TYPE_BOARD_MPLL)
    index = 2;

  e->board_freq[index] = c & 0xff;
  e->board_freq[index + 1] = (c >> 8) & 0xff;
}

static void parse_special_cell(efuse_special_info_t *e, u32 c)
{
  if (cell_is_empty(c) || cell_is_dead(c))
    return;

  u8 type = (c >> 24) & 0xff;
  if (type == CELL_TYPE_BOARD_MPLL || type == CELL_TYPE_BOARD_DPLL)
    parse_board_freq(e, c);
}

void efuse_info_init(efuse_info_t *e)
{
  e->bad_npus[0] = e->bad_npus[1] = NPU_ID_INVALID;
  e->serdes_pcs[0] = NPU_ID_INVALID;
  e->serdes_pcs[1] = NPU_ID_INVALID;
  e->serdes_pcs[2] = NPU_ID_INVALID;
  e->serdes_pcs[3] = NPU_ID_INVALID;
  e->serdes_pcs[4] = NPU_ID_INVALID;
  e->signature[0] = '\0';
}

void efuse_chip_init(efuse_chip_t *e)
{
  e->flag = NPU_ID_INVALID;
  e->bad_npus[0] = e->bad_npus[1] = NPU_ID_INVALID;
  e->pll = NPU_ID_INVALID;
  e->result = NPU_ID_INVALID;
}

void efuse_special_info_init(efuse_special_info_t *e)
{
  memset(e->board_freq, 0xff, sizeof(e->board_freq));
}

static u32 num_empty_cells()
{
  u32 cells = 0;
  u32 n = efuse_num_cells();
  for (u32 i = 0; i < n; i++) {
    u32 v = efuse_embedded_read(i);
    if (cell_is_empty(v))
      cells++;
  }
  uartlog("num of empty cells:%d\n", cells);
  return cells;
}

static u32 cells_for_info(const efuse_info_t *e)
{
  u32 n = 0;

  for (int i = 0; i < 2; i++)
    if (e->bad_npus[i] != NPU_ID_INVALID)
      n = 1;

  n += (strlen(e->signature) + 2) / 3;
  return (n + 2);
}

static u32 find_empty_cell(u32 start)
{
  u32 n = efuse_num_cells();
  for (u32 i = start; i < n; i++) {
    u32 v = efuse_embedded_read(i);
    if (cell_is_empty(v))
      return i;
  }
  return n;
}

static void write_bad_npu_ids(const efuse_info_t *e)
{
  u32 n = 0, value = CELL_TYPE_BAD_NPU << 24;
  for (int i = 0; i < 2; i++) {
    u32 id = e->bad_npus[i];
    if (id != NPU_ID_INVALID) {
      value |= ((id | 0x80) << 16) >> (n * 8);
      n++;
    }
  }

  if (n != 0) {
    u32 addr = find_empty_cell(0);
    efuse_embedded_write(addr, value);
  }
}

static void write_serdes_pcs(const efuse_info_t *e, u8 type)
{
  int start = 0, end = 3;
  u32 value = type << 24;
  switch (type) {
  case CELL_TYPE_SERDES_PCS_L:
    start = 0;
    end = 3;
    break;
  case CELL_TYPE_SERDES_PCS_H:
    start = 3;
    end = 5;
    break;
  }
  int i, j;
  for (i = start, j = 0; i < end; i++, j++) {
    u32 pcs = e->serdes_pcs[i];
    //uartlog("write info, pcs[%d]=%02x\n", i, pcs);
    value |= (pcs << (j * 8));
  }
  //uartlog("value:%d\n", value);
  if (i != 0) {
    u32 addr = find_empty_cell(0);
    efuse_embedded_write(addr, value);
  }
}

static void write_sig_cell(u32 addr, char a, char b, char c)
{
  u32 value = (CELL_TYPE_SIGNATURE << 24) |
      ((u32)a << 16) |
      ((u32)b << 8) |
      (u32)c;
  efuse_embedded_write(addr, value);
}

static void write_signature(const efuse_info_t *e)
{
  const char *sig = e->signature;
  size_t len = strlen(sig);
  u32 i, addr = 0;
  for (i = 0; i + 3 <= len; i += 3) {
    addr = find_empty_cell(addr);
    write_sig_cell(addr, sig[i], sig[i + 1], sig[i + 2]);
  }

  switch (len - i) {
    case 1:
      write_sig_cell(addr, sig[i], 0, 0);
      break;
    case 2:
      write_sig_cell(addr, sig[i], sig[i + 1], 0);
      break;
  }
}

static void write_board_freq(const efuse_special_info_t *e, u32 type)
{
  u8 index = 0;
  if (type == CELL_TYPE_BOARD_MPLL)
    index = 2;

  u32 value = type << 24;
  value |= ((e->board_freq[index + 1]) << 8) + e->board_freq[index];

  uartlog("value:%d\n", value);
  if (e->board_freq[index] != 0xff) {
    u32 addr = find_empty_cell(0);
    efuse_embedded_write(addr, value);
  }
}

void efuse_info_read(efuse_info_t *e)
{
  efuse_info_init(e);
  u32 n = efuse_num_cells();
  for (u32 i = 0; i < n; i++) {
    u32 v = efuse_embedded_read(i);
    parse_cell(e, v);
  }
}

int efuse_info_write(const efuse_info_t *e)
{
  if (cells_for_info(e) > num_empty_cells()) {
    uartlog("%s: error: not enough cells\n", __func__);
    return -1;
  }

  write_bad_npu_ids(e);
  write_serdes_pcs(e, CELL_TYPE_SERDES_PCS_L);
  write_serdes_pcs(e, CELL_TYPE_SERDES_PCS_H);
  write_signature(e);
  return 0;
}

void efuse_special_info_read(efuse_special_info_t *e)
{
  u32 n = efuse_num_cells();
  for (u32 i = 0; i < n; i++) {
    u32 v = efuse_embedded_read(i);
    parse_special_cell(e, v);
  }
}

int efuse_special_info_write(const efuse_special_info_t *e)
{
  if (num_empty_cells() < 1) {
    uartlog("%s: error: not enough cells\n", __func__);
    return -1;
  }

  write_board_freq(e, CELL_TYPE_BOARD_MPLL);
  write_board_freq(e, CELL_TYPE_BOARD_DPLL);

  return 0;
}

int efuse_info_is_empty(const efuse_info_t *e)
{
  return
      e->bad_npus[0] ==  NPU_ID_INVALID &&
      e->bad_npus[1] ==  NPU_ID_INVALID &&
      e->serdes_pcs[0] == NPU_ID_INVALID &&
      e->serdes_pcs[1] == NPU_ID_INVALID &&
      e->serdes_pcs[2] == NPU_ID_INVALID &&
      e->serdes_pcs[3] == NPU_ID_INVALID &&
      e->serdes_pcs[4] == NPU_ID_INVALID &&
      e->signature[0] == '\0';
}

static void write_chip_info(const efuse_chip_t *e)
{
  u32 addr=0, value=0;

  addr = find_empty_cell(CELL_CHIP_START);
  value = CELL_TYPE_CHIP_INFO << 24;
  value = value |((e->bad_npus[0])<<8) |((e->bad_npus[1])<<16);
  value = value | (((PLL_MAX_VALUE - e->pll)/PLL_STEP)<<4) | (e->result);
  efuse_embedded_write(addr, value);
}

void efuse_chip_info_read(efuse_chip_t *e)
{
  efuse_chip_init(e);

  if(num_empty_cells() == efuse_num_cells()) {
    uartlog("%s: all cell is empty, no chip info\n", __func__);
    return;
  }

  u32 n = efuse_num_cells();
  for (u32 i = CELL_CHIP_START; i < n; i++) {
    u32 v = efuse_embedded_read(i);
    if((v>>24) == CELL_TYPE_CHIP_INFO) {
      uartlog("%s: find chip info in addr %d\n", __func__, i);
      parse_chip_cell(e, v);
      uartlog("efuse read:\n"
            "  bad_npu0: %d\n"
            "  bad_npu1: %d\n"
            "  pll: %0d\n"
            "  result: %0d\n",
            e->bad_npus[0],
            e->bad_npus[1],
            e->pll,
            e->result);

    }
  }
}

static int check_chip_info(efuse_chip_t *rd, efuse_chip_t *wt)
{
  if(rd->bad_npus[0] == wt->bad_npus[0]
    && rd->bad_npus[1] == wt->bad_npus[1]
    && rd->pll == wt->pll
    && rd->result == wt->result) {
    uartlog("%s: chip info is same\n", __func__);
    uartlog("efuse info:\n"
              "  bad_npu0: %d\n"
              "  bad_npu1: %d\n"
              "  pll: %0d\n"
              "  result: %0d\n",
              rd->bad_npus[0],
              rd->bad_npus[1],
              rd->pll,
              rd->result);
    return 0;
  } else {
    uartlog("%s: chip info is different\n", __func__);
    uartlog("efuse read:\n"
              "  bad_npu0: %d\n"
              "  bad_npu1: %d\n"
              "  pll: %0d\n"
              "  result: %0d\n",
              rd->bad_npus[0],
              rd->bad_npus[1],
              rd->pll,
              rd->result);
    uartlog("efuse write:\n"
              "  bad_npu0: %d\n"
              "  bad_npu1: %d\n"
              "  pll: %0d\n"
              "  result: %0d\n",
              wt->bad_npus[0],
              wt->bad_npus[1],
              wt->pll,
              wt->result);
    return -1;
  }
}

int efuse_chip_info_write(const efuse_chip_t *e)
{
  if (num_empty_cells() == 0) {
    uartlog("%s: error: not enough cells\n", __func__);
    return -1;
  }

  efuse_chip_t rd_chip;
  efuse_chip_info_read(&rd_chip);

  if(check_chip_info(&rd_chip, (efuse_chip_t *)e) != 0) {
    uartlog("%s: start to write chip info\n", __func__);
    write_chip_info(e);

    efuse_chip_info_read(&rd_chip);
    uartlog("%s: check write chip info:\n", __func__);
    return check_chip_info(&rd_chip, (efuse_chip_t *)e);
  } else {
    uartlog("%s: chip info is same, cancel write chip info\n", __func__);
    return 0;
  }
}


static void write_chip_SN(const efuse_SN_t *e)
{
  u32 addr=0, i=0, value=0;

  addr = find_empty_cell(0);
  value = CELL_TYPE_SN_INFO << 24;

  for(i=0;i<sizeof(efuse_SN_t); )  {
    if(i==0) {
      value = value | e->SN[i] << 8 | e->SN[i+1];
	i = i + 2;
    } else {
      value = e->SN[i] << 24 | e->SN[i+1] <<16 | e->SN[i+2] <<8 | e->SN[i+3];
	i = i + 4;
    }
    efuse_embedded_write(addr, value);
    addr++;
  }
}

static void read_chip_SN(efuse_SN_t *e)
{
  u32 addr=0, i=0, value=0;

  if(num_empty_cells() == efuse_num_cells()) {
    uartlog("%s: all cell is empty, no sn info\n", __func__);
    return;
  }

  u32 n = efuse_num_cells();
  for (addr = 0; addr < n; addr++) {
    value = efuse_embedded_read(addr);
    if((value>>24) == CELL_TYPE_SN_INFO) {
      uartlog("%s: find sn info in %d\n", __func__, addr);
      e->SN[0] = (value>>8) & 0xff;
      e->SN[1] = (value) & 0xff;

	addr++;
      for(i=2;i<sizeof(efuse_SN_t);i=i+4)  {
        value = efuse_embedded_read(addr);
        e->SN[i+3] = value & 0xff;
        e->SN[i+2] = (value>>8) & 0xff;
        e->SN[i+1] = (value>>16) & 0xff;
        e->SN[i] = (value>>24) & 0xff;
        addr++;
      }
      uart_puts("read SN is ");
      for(int j=0; j<sizeof(efuse_SN_t); j++) {
        uart_putc(e->SN[j]);
      }
      uartlog("\n");
      uartlog("\n");
    }
  }
}

static int check_SN(efuse_SN_t *rd, efuse_SN_t *wt)
{
  for(int i=0; i<sizeof(efuse_SN_t); i++) {
    if(rd->SN[i] != wt->SN[i]) {
      uartlog("%s: SN is different\n", __func__);
      uart_puts("read SN is ");
      for(int j=0; j<sizeof(efuse_SN_t); j++) {
        uart_putc(rd->SN[j]);
      }
      uartlog("\n");
      uartlog("\n");

      uart_puts("write SN is ");
      for(int j=0; j<sizeof(efuse_SN_t); j++) {
        uart_putc(wt->SN[j]);
      }
      uartlog("\n");
      uartlog("\n");
      return -1;
    }
  }
  uartlog("%s: SN is same: \n", __func__);
  uart_puts("SN = ");
  for(int j=0; j<sizeof(efuse_SN_t); j++) {
    uart_putc(wt->SN[j]);
  }
  uartlog("\n");
  uartlog("\n");

  return 0;
}
int efuse_SN_write(const efuse_SN_t *e)
{
  if (num_empty_cells() < CELL_CHIP_START) {
    uartlog("%s: error: not enough cells\n", __func__);
    return -1;
  }

  efuse_SN_t rd_sn;
  read_chip_SN(&rd_sn);

  if(check_SN(&rd_sn, (efuse_SN_t *)e) != 0) {
      uartlog("%s: start to write SN\n", __func__);
      write_chip_SN(e);

	//check write SN
      read_chip_SN(&rd_sn);
      uartlog("%s: check write SN:\n", __func__);
      return check_SN(&rd_sn, (efuse_SN_t *)e);
  } else {
    uartlog("%s: SN is same, cancel write SN\n", __func__);
    return 0;
  }
}

int efuse_SN_read(efuse_SN_t *e)
{
  read_chip_SN(e);
  return 0;
}

