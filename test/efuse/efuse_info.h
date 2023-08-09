typedef struct {
  u8 bad_npus[2];
  u8 serdes_pcs[5];
  char signature[16];
} efuse_info_t; //7cells

typedef struct {
  char SN[26];
} efuse_SN_t; //use 7 cells

typedef struct {
  u8 flag;
  u8 bad_npus[2];
  u32 pll;
  u8 result;
} efuse_chip_t; //use 1 cell


typedef struct {
  u8 board_freq[4];
}efuse_special_info_t; //1cells

void efuse_info_init(efuse_info_t *e);
void efuse_info_read(efuse_info_t *e);
int efuse_info_write(const efuse_info_t *e);
int efuse_info_is_empty(const efuse_info_t *e);

void efuse_special_info_init(efuse_special_info_t *e);
void efuse_special_info_read(efuse_special_info_t *e);
int efuse_special_info_write(const efuse_special_info_t *e);
void efuse_chip_info_read(efuse_chip_t *e);
int efuse_chip_info_write(const efuse_chip_t *e);
int efuse_SN_write(const efuse_SN_t *e);
int efuse_SN_read(efuse_SN_t *e);

