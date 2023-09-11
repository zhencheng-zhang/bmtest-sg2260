#include "boot_test.h"
#include "mmio.h"
#include "i2c.h"

#define I2C_WRITEL(reg, val) mmio_write_32(ctrl_base + reg, val)
#define I2C_READL(reg) mmio_read_32(ctrl_base + reg)

#define REG_TOP_SOFT_RST0 0x7030013000
#define BIT_MASK_TOP_SOFT_RST0_I2C0		(1 << 14)

static uint32_t ctrl_base;
static uint8_t slave_addr;

int i2c_master_read(uint8_t reg)
{
	uint8_t data = 0xff;

	// read slave response
	I2C_WRITEL(REG_I2C_DATA_CMD, reg | BIT_I2C_CMD_DATA_RESTART_BIT);
	I2C_WRITEL(REG_I2C_DATA_CMD, 0 |
		BIT_I2C_CMD_DATA_READ_BIT |
		BIT_I2C_CMD_DATA_STOP_BIT);

	while (1) {
		uint32_t irq = I2C_READL(REG_I2C_RAW_INT_STAT);

		if (irq & BIT_I2C_INT_RX_FULL) {
			data = I2C_READL(REG_I2C_DATA_CMD);
			break;
		}
	}

	return data;
}

void i2c_master_write(uint8_t reg, uint8_t data)
{
	// write reg
	I2C_WRITEL(REG_I2C_DATA_CMD, reg | BIT_I2C_CMD_DATA_RESTART_BIT);
	// write data
	I2C_WRITEL(REG_I2C_DATA_CMD, data | BIT_I2C_CMD_DATA_STOP_BIT);

	while (1) {
		uint32_t irq = I2C_READL(REG_I2C_RAW_INT_STAT);

		if (irq & BIT_I2C_INT_TX_EMPTY)
			break;
	}
}

void i2c_master_reset(void)
{
	// soft reset
	mmio_clrbits_32(REG_TOP_SOFT_RST0, BIT_MASK_TOP_SOFT_RST0_I2C0);
	udelay(10);
	mmio_setbits_32(REG_TOP_SOFT_RST0, BIT_MASK_TOP_SOFT_RST0_I2C0);
	udelay(10);
}

void i2c_master_init(uint64_t base, uint16_t addr)
{
	uint32_t reg_val = 0;

	ctrl_base = base;
	slave_addr = addr;

	i2c_master_reset();

	I2C_WRITEL(REG_I2C_ENABLE, 0);
	reg_val = BIT_I2C_CON_MASTER_MODE |
			BIT_I2C_CON_SLAVE_DIS |
			BIT_I2C_CON_RESTART_EN |
			BIT_I2C_CON_STANDARD_SPEED;
	I2C_WRITEL(REG_I2C_CON, reg_val);

	I2C_WRITEL(REG_I2C_TAR,  slave_addr);

	// to get a 100KHz SCL when I2C source clock is 100MHz
	I2C_WRITEL(base + REG_I2C_SS_SCL_HCNT, 0x69);
	I2C_WRITEL(base + REG_I2C_SS_SCL_LCNT, 0x7c);
	I2C_WRITEL(base + REG_I2C_FS_SCL_HCNT, 0x14);
	I2C_WRITEL(base + REG_I2C_FS_SCL_LCNT, 0x27);
	/*
	reg_val = 0x1A4;
	I2C_WRITEL(REG_I2C_SS_SCL_HCNT, reg_val);
	reg_val = 0x1F0;
	I2C_WRITEL(REG_I2C_SS_SCL_LCNT, reg_val);
	*/
	

	reg_val = 0x0;
	I2C_WRITEL(REG_I2C_INT_MASK, reg_val);

	I2C_WRITEL(REG_I2C_RX_TL,  0x00);
	I2C_WRITEL(REG_I2C_TX_TL,  0x01);

	I2C_WRITEL(REG_I2C_ENABLE, 1);

	I2C_READL(REG_I2C_CLR_INTR);
	uartlog("master_init finished\n");
}
