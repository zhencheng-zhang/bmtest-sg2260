#include <stdio.h>
#include <boot_test.h>
#include <system_common.h>
#include "efuse.h"
#include "efuse_info.h"
#include "string.h"
#include "command.h"
#include "cli.h"
#include "time.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static int print_efuse(int argc, char **argv)
{
	// efuse_embedded_write(0x3,0xffffffff);
	// efuse_embedded_write(0x8,0x12345678);

	u32 val,i;

	uartlog("0 efuse_ecc_read:\n");
	for(i=0; i < 16; i++){
		val = efuse_ecc_read(i);
		uartlog(" %8x", val);
		if ((i + 1) % 4 == 0)
			uartlog("\n");
	}

	uartlog("\n 0 efuse_embedded_read:\n");
	for (i = 0; i < 16; i++) {
		val = efuse_embedded_read(i);
		uartlog(" %8x", val);
		if ((i + 1) % 4 == 0)
	 		uartlog("\n");
	}

	efuse_embedded_write(0x6,0x12345678);
	efuse_embedded_write(0x8,0x12345678);
	efuse_embedded_write(0x12,0x12345678);
	uartlog("0x12 [%x]\n", efuse_embedded_read(0x12));

	uartlog("\n 1 efuse had been changed by embedwrite ((0x6, 8, 12,0x12345678)), efuse_ecc_read:\n");
	for(i=0; i < 32; i++){
		val = efuse_ecc_read(i);
		uartlog(" %8x", val);
		if ((i + 1) % 4 == 0)
			uartlog("\n");
	}

	uartlog("\n 1 efuse had been changed by embedwrite ((0x6, 8, 12,0x12345678)), efuse_embedded_read:\n");
	for (i = 0; i < 32; i++) {
		val = efuse_embedded_read(i);
		uartlog(" %8x", val);
		if ((i + 1) % 4 == 0)
			uartlog("\n");
	}

#if 0
	// bit 6 in soft_reset_reg
	writel(SOFT_RESET_REG0,readl(SOFT_RESET_REG0)&~(1<<SOFT_RESET_EFUSE_BIT));
	mdelay(1);
	writel(SOFT_RESET_REG0,readl(SOFT_RESET_REG0)|(1<<SOFT_RESET_EFUSE_BIT));
#endif

	efuse_embedded_write(0x5,0x12567f);
	uartlog("\n 2 write((0x5,0x12567f)) efuse_embedded_read:\n");
	for (i = 0; i < 256; i++) {
		val = efuse_embedded_read(i);
		uartlog(" %8x", val);
		if ((i + 1) % 4 == 0)
			uartlog("\n");
	}
	uartlog("\n 2 write((0x5,0x12567f)) efuse_ecc_read:\n");
	for(i=0; i <64; i++){
		val = efuse_ecc_read(i);
		uartlog(" %8x", val);
		if ((i + 1) % 4 == 0)
			uartlog("\n");
	}

	return 0;
}

static int embedded_write_test(int argc, char **argv)
{
	u32 addr = strtol(argv[1], NULL, 16);
	u32 val = strtol(argv[2], NULL, 16);

	efuse_embedded_write(addr, val);
	uartlog("Embedded write addr[%0x]: %0x\n", addr, val);

	return 0;
}

static int embedded_read_test(int argc, char **argv)
{
	u32 addr = strtol(argv[1], NULL, 16);

	uartlog("Embedded read from addr[%0x]: %0x\n", addr, efuse_embedded_read(addr));

	return 0;
}

static void autoload(void)
{
	writel(SOFT_RESET_REG0,readl(SOFT_RESET_REG0)&~(1<<SOFT_RESET_EFUSE_BIT));
	writel(SOFT_RESET_REG0,readl(SOFT_RESET_REG0)|(1<<SOFT_RESET_EFUSE_BIT));
	// uartlog("Power down\n");
	// writel(EFUSE_BASE, readl(EFUSE_BASE) | (1 << EFUSE_PD_BIT));

	// uartlog("Power on\n");
	// writel(EFUSE_BASE, (readl(EFUSE_BASE) & ~(1 << EFUSE_PD_BIT) )| (1 << 28));
	
	uartlog("Waiting autoload down\n");
	while (1) {
		if (readl(EFUSE_BASE) & (1<<BOOT_DONE_BIT))
			break;
	}
	uartlog("boot done\n");
}

static int autoload_test(int argc, char **argv)
{
	int i;

	// for (i=0; i<16; i++) {
	// 	u64 shadow_reg_addr = SHADOW_REG_BASE + i*4;
	// 	uartlog("%lx %x\n", shadow_reg_addr, readl(shadow_reg_addr));
	// }
	for (i=2; i<EFUSE_MAX; i++) {
		if (i == SEC_REGION)
			efuse_embedded_write(SEC_REGION, 0xffffffc0);
		else if (i == SEC_REGION_EXTRA)
			efuse_embedded_write(SEC_REGION_EXTRA, 0xffffffff);
		else
			efuse_embedded_write(i, i);
		// if (efuse_embedded_read(i) != i)
		// 	uartlog("WRITING ERROR: w[%x], r[%x]\n", i, efuse_embedded_read(i));
	}

	autoload();
	// mdelay(1);
	int success = 1;

	for (i=0; i<EFUSE_MAX; i++) {
		u64 shadow_reg_addr = SHADOW_REG_BASE + i*4;
		// uartlog("%lx %x\n", shadow_reg_addr, readl(shadow_reg_addr));
		int r_data = readl(shadow_reg_addr);
		if (r_data != i) {
			uartlog("TEST FAILD %lx w[%d] r[%x] ecc_r[%x], eb_r[%x]\n", shadow_reg_addr, i, r_data, 
																		efuse_ecc_read(i), efuse_embedded_read(i));
			success = 0;
			// return 0;
		}
	}

	uartlog("autoload test %s!\n", success ? "pass" : "failed");

	// uartlog("0 efuse_ecc_read:\n");
	// for(i=0; i < 16; i++){
	// 	int val = efuse_ecc_read(i);
	// 	uartlog(" %8x", val);
	// 	if ((i + 1) % 4 == 0)
	// 		uartlog("\n");
	// }


	return 0;
}

static int rom_patch_test(int argc, char **argv)
{
	int i;
	u32 sec_val = 0xffffffc0;
	u32 sec_extra_val = 0xffffffff;

	// efuse_reset();

	// for(i=0; i<ROM_PATCH_N; i++) {
	// 	u64 rom_addr = ROM_BASE + i * 4;
	// 	uartlog("%lx %x\n", rom_addr, readl(rom_addr));
	// }
	if (efuse_embedded_read(SEC_REGION) == sec_val) {
		uartlog("first boot, writing rom patch\n");
		efuse_embedded_write(SEC_REGION, sec_val);
		efuse_embedded_write(SEC_REGION_EXTRA, sec_extra_val);

		for(i=0; i<ROM_PATCH_N; i++) {
			u32 addr_offset = ROM_PATCH_OFFSET + i * 2;
			u32 patch_offset = addr_offset + 1;
			u32 addr = (u32)(ROM_BASE + i * 4);
			// uartlog("efuse_cell: %d rom_addr: %x\n", addr_offset, addr);
			efuse_embedded_write(addr_offset, addr);
			efuse_embedded_write(patch_offset, 0xffffffff);
		}
		autoload();
	}

	CBO_inval(ROM_BASE, ROM_PATCH_N * 4);

	for(i=0; i<ROM_PATCH_N; i++) {
		// u64 shadow_reg_addr = SHADOW_REG_BASE + (i * 2 + ROM_PATCH_OFFSET) * 4;
		// uartlog("shadow reg: addr: %lx[%x], cmd %x\n", shadow_reg_addr, readl(shadow_reg_addr), readl(shadow_reg_addr+4));

		u64 rom_addr = ROM_BASE + i * 4;
		uartlog("rom %lx %x\n", rom_addr, readl(rom_addr));
	}

	return 0;
}

// force to bootfrom rom
static int boot_mode_test(int argc, char **argv)
{
	// efuse_reset();

	int boot_mode_bit = 12;
	efuse_embedded_write(SCS, 1<<boot_mode_bit);

	autoload();

	// TODO: reset ap

	return 0;
}

static int w_lock_test(int argc, char **argv)
{
	int i;

	efuse_reset();

	for(i=0; i<W_LOCK_NUM; i++)
		efuse_embedded_write(W_LOCK+i, 0xffffffff);
	// valid efuse_w_lock
	autoload();

	autoload_test(0, NULL);

	return 0;
}

static int kpub_w_lock_test(int argc, char **argv)
{
	efuse_reset();
	efuse_embedded_write(SEC_REGION, 0x1);

	int i;
	for(i=0; i<KPUB_HASH_NUM; i++)
		efuse_embedded_write(KPUB_HASH+i, 0xffff);
	autoload();
	for(i=0; i<KPUB_HASH_NUM; i++)
		efuse_embedded_write(KPUB_HASH+i, 0xffff0000);
	
	for(i=0; i<KPUB_HASH_NUM; i++) {
		u64 shadow_reg_addr = SHADOW_REG_BASE + (KPUB_HASH+i) * 4;
		uartlog("addr: %lx, val: %x, eb_r: %x\n", shadow_reg_addr, readl(shadow_reg_addr),
													efuse_embedded_read(KPUB_HASH+i));
	}
	return 0;
}

static int kpub_r_lock_test(int argc, char **argv)
{
	int i;

	efuse_reset();

	for(i=0; i<KPUB_HASH_NUM; i++)
		efuse_embedded_write(KPUB_HASH+i, 0xffffffff);
	efuse_embedded_write(SEC_REGION, 0x4);

	autoload();

	for(i=0; i<KPUB_HASH_NUM; i++) {
		u64 shadow_reg_addr = SHADOW_REG_BASE + (KPUB_HASH+i) * 4;
		uartlog("addr: %lx, val: %x, eb_r: %x\n", shadow_reg_addr, readl(shadow_reg_addr),
													efuse_embedded_read(KPUB_HASH+i));
	}

	return 0;
}

static int ecc_hanming_test(int argc, char **argv)
{
	// input raw data[25:0]: 10101010101010101010101010
	// hanming code: 1010101010101010010101011010010
	efuse_embedded_write(0, 0b1010101010101010010101011010010);
	// modify bit 25
	efuse_embedded_write(1, 0b0010101010101010010101011010010);

	// enable ecc
	// uartlog("Power down\n");
	// writel(EFUSE_BASE, readl(EFUSE_BASE) | (1 << EFUSE_PD_BIT));

	// uartlog("Power on\n");
	writel(EFUSE_BASE, (readl(EFUSE_BASE) & ~(1 << EFUSE_PD_BIT) & ~(1 << 28)));

	// uartlog("Waiting autoload down\n");
	// while (1) {
	// 	if (readl(EFUSE_BASE) & (1<<BOOT_DONE_BIT))
	// 		break;
	// }
	// uartlog("boot done\n");

	uartlog("EFUSE_MD: %x\necc_r %x %x\n", readl(EFUSE_BASE),efuse_ecc_read(0), efuse_ecc_read(1));
	uartlog("shadow reg %x %x\n", readl(SHADOW_REG_BASE), readl(SHADOW_REG_BASE + 4));

	return 0;
}

static struct cmd_entry test_cmd_list[] __attribute__((unused)) = {
	{ "eb_write", embedded_write_test, 0, "embedded write [addr] [val]" },
	{ "eb_read", embedded_read_test, 0, "embedded read [addr]" },
	{ "print_efuse", print_efuse, 0, NULL },
	{ "autoload", autoload_test, 0, NULL },
	{ "rom_patch", rom_patch_test, 0, NULL },
	{ "boot_mode", boot_mode_test, 0, NULL },
	{ "w_lock", w_lock_test, 0, NULL },
	{ "kpub_w_lock", kpub_w_lock_test, 0, NULL },
	{ "kpub_r_lock", kpub_r_lock_test, 0, NULL },
	{ "ecc_hanming", ecc_hanming_test, 0, NULL },
	//{ "loop", do_loop, 3, "loop port master/slave baudrate" },
	{ NULL, NULL, 0, NULL },
};

int testcase_efuse(void)
{
	int i, ret = 0;

	printf("Enter efuse test\n");

	efuse_reset();

	// writel(EFUSE_BASE, readl(EFUSE_BASE) | (1<<28));
	// uartlog("EFUSE_MD: %0x\n", readl(EFUSE_BASE));

	// efuse_embedded_write(0x3,0xffffffff);
	// efuse_embedded_write(0x8,0x12345678);
	// efuse_embedded_write(0x23,0x12345678);
	// efuse_embedded_write(0x33,0x12345678);
	// efuse_embedded_write(255,0x12345678);


	for (i = 0; i < ARRAY_SIZE(test_cmd_list) - 1; i++) {
		command_add(&test_cmd_list[i]);
	}
	cli_simple_loop();

	return ret;
}

#ifndef BUILD_TEST_CASE_ALL
int testcase_main()
{
  return testcase_efuse();
}
#endif
