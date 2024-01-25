/*
 * NOTE: Efuse is organized as efuse_num_cells() 32-bit
 *       words, named cells.
 */

u32 efuse_num_cells();

#ifdef CONFIG_CHIP_SG2042
#define SOFT_RESET_EFUSE0_BIT   6
#define SOFT_RESET_EFUSE1_BIT   7
#elif CONFIG_CHIP_SG2260
#define SOFT_RESET_EFUSE0_BIT   10
#define SOFT_RESET_EFUSE1_BIT   11
#endif

#define CLK_EN_EFUSE_BIT    17
#define CLK_EN_APB_EFUSE_BIT    18
#define EFUSE_PD_BIT		8

#define SCS					1
#define SEC_REGION			2
#define SEC_REGION_EXTRA	3
#define W_LOCK				20
#define W_LOCK_NUM			4
#define KPUB_HASH			12
#define KPUB_HASH_NUM		8

#define SOFT_RESET_EFUSE_BIT	SOFT_RESET_EFUSE0_BIT
#define EFUSE_BASE				EFUSE0_BASE
#define SHADOW_REG_BASE			0x7040000800UL
#define ROM_BASE				0x7000020000UL
#define ROM_PATCH_N				50
#define ROM_PATCH_OFFSET		24

#define BOOT_DONE_BIT		7
#define EFUSE_MAX			127	// 256*32

/*
 * address: [0, efuse_num_cells() - 1]
 */
u32 efuse_embedded_read(u32 address);
void efuse_embedded_write(u32 address, u32 val);
u32 efuse_ecc_read(u32 address);
void efuse_reset(void);