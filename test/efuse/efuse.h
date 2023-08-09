/*
 * NOTE: Efuse is organized as efuse_num_cells() 32-bit
 *       words, named cells.
 */

u32 efuse_num_cells();

#define SOFT_RESET_REG0 	0x30013000
#define SOFT_RESET_REG1 	0x30013004
#define SOFT_RESET_EFUSE0_BIT   6
#define SOFT_RESET_EFUSE1_BIT   7

#define CLK_EN_REG0 		0x7030012000
#define CLK_EN_EFUSE_BIT    20
#define CLK_EN_APB_EFUSE_BIT    21



/*
 * address: [0, efuse_num_cells() - 1]
 */
u32 efuse_embedded_read(u32 address);
void efuse_embedded_write(u32 address, u32 val);
u32 efuse_ecc_read(u32 address);