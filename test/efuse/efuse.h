/*
 * NOTE: Efuse is organized as efuse_num_cells() 32-bit
 *       words, named cells.
 */

u32 efuse_num_cells();

#define SOFT_RESET_REG1 	0x50010c04 
#define CLK_EN_REG0 		0x50010800




/*
 * address: [0, efuse_num_cells() - 1]
 */
u32 efuse_embedded_read(u32 address);
void efuse_embedded_write(u32 address, u32 val);
u32 efuse_ecc_read(u32 address);

