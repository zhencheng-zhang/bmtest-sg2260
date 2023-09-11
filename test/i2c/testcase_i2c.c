#include <stdio.h>
#include <string.h>
#include <boot_test.h>
#include <system_common.h>
#include "reg_i2c.h"
#include "command.h"
#include "cli.h"
#include "mmio.h"

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))
#define CMD_BUF_MAX 512
#define bool	_Bool


typedef u8 uchar;
typedef unsigned long ulong;

struct dw_scl_sda_cfg {
	u32 ss_hcnt;
	u32 fs_hcnt;
	u32 ss_lcnt;
	u32 fs_lcnt;
	u32 sda_hold;
};
struct dw_i2c {
	unsigned int i2c_clk_frq;
	struct i2c_regs *regs;
	struct dw_scl_sda_cfg *scl_sda_cfg;
};
static struct i2c_info *i2c_info;
static struct i2c_info i2c_info_instance = {
    .base = 0x7030006000,   // 设置合适的 I2C 基地址
    .freq = 166000000,       // 设置合适的频率
    .speed = I2C_FAST_SPEED,  // 设置合适的速度模式
    .dev = {0},
};

static inline void writel_1(u32 val, void *io)
{
	mmio_write_32((uintptr_t)io, val);
}

static inline u32 readl_1(void *io)
{
	return mmio_read_32((uintptr_t)io);
}

static void dw_i2c_enable(struct i2c_regs *i2c_base, bool enable)
{
	u32 ena = enable ? IC_ENABLE_0B : 0;
	int timeout = 100;

	do {
		writel_1(ena, &i2c_base->ic_enable);
		if ((readl_1(&i2c_base->ic_enable_status) & IC_ENABLE_0B) == ena)
			return;

		/*
		 * Wait 10 times the signaling period of the highest I2C
		 * transfer supported by the driver (for 400KHz this is
		 * 25us) as described in the DesignWare I2C databook.
		 */
		udelay(25);
	} while (timeout--);

	printf("timeout in %sabling I2C adapter\n", enable ? "en" : "dis");
}

static unsigned int __dw_i2c_set_bus_speed(struct dw_i2c *i2c,
					   unsigned int speed)
{
	int i2c_spd;
	unsigned int clk_freq;
	unsigned int cntl;
	unsigned int hcnt, lcnt;
	struct i2c_regs *i2c_base = i2c->regs;
	struct dw_scl_sda_cfg *scl_sda_cfg = i2c->scl_sda_cfg;

	clk_freq = i2c->i2c_clk_frq ? (i2c->i2c_clk_frq / 1000000) : IC_CLK;

	if (speed >= I2C_MAX_SPEED)
		i2c_spd = IC_SPEED_MODE_MAX;
	else if (speed >= I2C_FAST_SPEED)
		i2c_spd = IC_SPEED_MODE_FAST;
	else
		i2c_spd = IC_SPEED_MODE_STANDARD;

	/* to set speed cltr must be disabled */
	dw_i2c_enable(i2c_base, false);

	cntl = (readl_1(&i2c_base->ic_con) & (~IC_CON_SPD_MSK));

	/* 这样算出来的hcnt和lcnt是对的吗？以及下面对high speed 的赋值....?????
	hcnt = (clk_freq * MIN_HS_SCL_HIGHTIME) / NANO_TO_MICRO;
	lcnt = (clk_freq * MIN_HS_SCL_LOWTIME) / NANO_TO_MICRO;
	case IC_SPEED_MODE_MAX:
		cntl |= IC_CON_SPD_SS | IC_CON_RE;
		if (scl_sda_cfg) {
	*/
	

	switch (i2c_spd) {
	case IC_SPEED_MODE_MAX:
		cntl |= IC_CON_SPD_SS | IC_CON_RE;
		if (scl_sda_cfg) {
			hcnt = scl_sda_cfg->fs_hcnt;
			lcnt = scl_sda_cfg->fs_lcnt;
		} else {
			hcnt = (clk_freq * MIN_HS_SCL_HIGHTIME) / NANO_TO_MICRO;
			lcnt = (clk_freq * MIN_HS_SCL_LOWTIME) / NANO_TO_MICRO;
		}
		writel_1(hcnt, &i2c_base->ic_hs_scl_hcnt);
		writel_1(lcnt, &i2c_base->ic_hs_scl_lcnt);
		break;

	case IC_SPEED_MODE_STANDARD:
		cntl |= IC_CON_SPD_SS;
		if (scl_sda_cfg) {
			hcnt = scl_sda_cfg->ss_hcnt;
			lcnt = scl_sda_cfg->ss_lcnt;
		} else {
			hcnt = (clk_freq * MIN_SS_SCL_HIGHTIME) / NANO_TO_MICRO;
			lcnt = (clk_freq * MIN_SS_SCL_LOWTIME) / NANO_TO_MICRO;
		}
		writel_1(hcnt, &i2c_base->ic_ss_scl_hcnt);
		writel_1(lcnt, &i2c_base->ic_ss_scl_lcnt);
		break;

	case IC_SPEED_MODE_FAST:
	default:
		cntl |= IC_CON_SPD_FS;
		if (scl_sda_cfg) {
			hcnt = scl_sda_cfg->fs_hcnt;
			lcnt = scl_sda_cfg->fs_lcnt;
		} else {
			hcnt = (clk_freq * MIN_FS_SCL_HIGHTIME) / NANO_TO_MICRO;
			lcnt = (clk_freq * MIN_FS_SCL_LOWTIME) / NANO_TO_MICRO;
		}
		writel_1(hcnt, &i2c_base->ic_fs_scl_hcnt);
		writel_1(lcnt, &i2c_base->ic_fs_scl_lcnt);
		break;
	}

	/* always working in master mode and enable restart */
	writel_1(cntl | IC_CON_RE | IC_CON_MM, &i2c_base->ic_con);

	/* Configure SDA Hold Time if required */
	if (scl_sda_cfg)
		writel_1(scl_sda_cfg->sda_hold, &i2c_base->ic_sda_hold);

	/* Enable back i2c now speed set */
	dw_i2c_enable(i2c_base, true);

	return 0;
}

static void __dw_i2c_init(struct dw_i2c *i2c, int speed, int slaveaddr)
{
	struct i2c_regs *i2c_base = i2c->regs;
	/* Disable i2c */
	dw_i2c_enable(i2c_base, false);
	writel_1((IC_CON_SD | IC_CON_SPD_FS | IC_CON_MM), &i2c_base->ic_con);
	writel_1(IC_RX_TL, &i2c_base->ic_rx_tl);
	writel_1(IC_TX_TL, &i2c_base->ic_tx_tl);
	writel_1(IC_STOP_DET, &i2c_base->ic_intr_mask);
	__dw_i2c_set_bus_speed(i2c, speed);
	writel_1(slaveaddr, &i2c_base->ic_sar);
	/* Enable i2c */
	dw_i2c_enable(i2c_base, true);
}

static int designware_i2c_init(struct dw_i2c *i2c, void *reg,
			       unsigned long freq, unsigned long speed)
{
	i2c->regs = reg;
	i2c->i2c_clk_frq = freq;

	__dw_i2c_init(i2c, speed, 0);
	//__dw_i2c_init(i2c, speed, 0x17);

	return 0;
}

int i2c_init(struct i2c_info *info, int n)
{
	int i, err;

	i2c_info = info;

	for (i = 0; i < n; ++i) {
		err = designware_i2c_init((void *)&i2c_info[i].dev,
					  (void *)i2c_info[i].base,
					  i2c_info[i].freq,
					  i2c_info[i].speed);
		if (err)
			return err;
	}

	return 0;
}

static void i2c_flush_rxfifo(struct i2c_regs *i2c_base)
{
	while (readl_1(&i2c_base->ic_status) & IC_STATUS_RFNE)
		readl_1(&i2c_base->ic_cmd_data);
}

static void i2c_setaddress(struct i2c_regs *i2c_base, unsigned int i2c_addr)
{
	// unsigned int a, b;

	// asm volatile ("here_0:");
	// a = i2c_readl(0x7030006004UL);
	// asm volatile ("here_1:");
	// b = readl_1(&i2c_base->ic_tar);
	// asm volatile ("here_2:");

	/* Disable i2c */
	dw_i2c_enable(i2c_base, false);

	writel_1(i2c_addr, &i2c_base->ic_tar);
	
	/* Enable i2c */
	dw_i2c_enable(i2c_base, true);
}

static int i2c_xfer_init(struct i2c_regs *i2c_base, uchar chip, uint addr, int alen)
{
	i2c_setaddress(i2c_base, chip);
	while (alen) {
		alen--;
		/* high byte address going out first */
		writel_1((addr >> (alen * 8)) & 0xff,
		       &i2c_base->ic_cmd_data);
	}
	return 0;
}

static int i2c_xfer_finish(struct i2c_regs *i2c_base)
{
	/* send stop bit */
	writel_1(1 << 9, &i2c_base->ic_cmd_data);


	while (1) {
		if ((readl_1(&i2c_base->ic_raw_intr_stat) & IC_STOP_DET)) {
			readl_1(&i2c_base->ic_clr_stop_det);
			break;
		}
	}

	i2c_flush_rxfifo(i2c_base);

	return 0;
}

static int designware_i2c_xfer(struct dw_i2c *i2c, struct i2c_msg *msg,
			       int nmsgs)
{
	uint16_t cmd_buf[CMD_BUF_MAX];
	uint8_t data_buf[CMD_BUF_MAX];
	int i, j, k, l, err, wait;
	uint32_t status;
	struct i2c_regs *regs;
	int my_signal = 0;

	if (nmsgs <= 0)
		return 0;

	if (i2c_xfer_init(i2c->regs, msg[0].addr, 0, 0))
		return -EIO;

	regs = i2c->regs;

	/* stream messages */
	k = 0, l = 0;
	for (i = 0; i < nmsgs; ++i) {
		for (j = 0; j < msg[i].len; ++j) {
			if (msg[i].flags & I2C_M_RD) {
				cmd_buf[k] = IC_CMD;
				my_signal = 1;
				++l;
			} else {
				cmd_buf[k] = msg[i].buf[j];
			}
			// printf("************cmd_buf---- = 0x%x\n", cmd_buf[k]);
			++k;
			if (k > CMD_BUF_MAX) {
				printf("too many commands\n");
				return -ENOMEM;
			}
		}
	}
	j = 0;
	wait = 0;
	for (i = 0; i < k || j < l;) {

		/* check tx abort */ 
		if (readl_1(&regs->ic_raw_intr_stat) & IC_TX_ABRT) {
			err = (readl_1(&regs->ic_tx_abrt_source) &
			       IC_ABRT_7B_ADDR_NOACK) ?
				-ENODEV : -EIO;
			
			printf("*****check tx abort*****\n");
			// -ENODEV 表示设备不存在错误，通常表示设备的地址没有应答。-EIO 表示输入/输出错误，通常表示其他类型的传输错误。
			// clear abort source
			readl_1(&regs->ic_clr_tx_abrt);
			goto finish;
		}
		status = readl_1(&regs->ic_status);
		/* transmit fifo not full, push one command if we have not sent
		 * all commands out
		 */
		if ((status & IC_STATUS_TFNF) && (i < k)) {
			if (!my_signal && (i == k-1))
				cmd_buf[i] |= (1 << 9);
			writel_1(cmd_buf[i], &regs->ic_cmd_data);
			// printf("****second********cmd_buf---- = 0x%x\n", cmd_buf[i]);
			++i;
		}

		/* receive data if receive fifo not empty and if we have not
		 * receive enough data
		 */
		if ((status & IC_STATUS_RFNE) && (j < l)) {
			data_buf[j] = readl_1(&regs->ic_cmd_data);
			++j;
		}
		wait++;
		udelay(10);
		if (wait > 5000) { // 50ms
			ERROR("i2c xfer timeout %d\n", __LINE__);
			return -1;
		}
	}
	k = 0;
	for (i = 0; i < nmsgs; ++i) {
		if ((msg[i].flags & I2C_M_RD) == 0)
			continue;

		for (j = 0; j < msg[i].len; ++j) {
			msg[i].buf[j] = data_buf[k];
			++k;
			if (k > l) {
				printf("software logic error\n");
				err = -EFAULT;
				goto finish;
			}
		}
	}

	err = 0;

finish:
	i2c_xfer_finish(regs);
	return err;
}

int i2c_xfer(int i2c, struct i2c_msg *msg, int nmsgs)
{
	return designware_i2c_xfer((void *)&i2c_info[i2c].dev, msg, nmsgs);
}


int i2c_smbus_read_byte(int i2c, unsigned char addr,
			unsigned char cmd, unsigned char *data)
{
	struct i2c_msg msg[2];

	memset(msg, 0, sizeof(msg));
	msg[0].addr = addr;
	msg[0].flags = 0; /* 7bit address */
	msg[0].len = 1;
	msg[0].buf = &cmd;
	// 在主从机通信时，代码中打印相关信息，会影响示波器的信号采集，即影响通信，带来时延
	// printf("cmd = %x\n", cmd);
	// printf("msg[0].buf = %p\n", (void *)msg[0].buf);

	msg[1].addr = addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = data;

	return i2c_xfer(i2c, msg, 2);
}

int i2c_smbus_write_byte(int i2c, unsigned char addr,
			 unsigned char cmd, unsigned char data)
{
	struct i2c_msg msg;
	u8 buf[2];

	memset(&msg, 0, sizeof(msg));

	buf[0] = cmd;
	buf[1] = data;

	msg.addr = addr;
	msg.flags = 0; /* 7bit address */
	msg.len = 2;
	msg.buf = buf;

	return i2c_xfer(i2c, &msg, 1);
}

static int do_master_read_slave(int argc, char **argv)
{
	int slave_addr = strtoul(argv[1], NULL, 16);
	int offset = strtoul(argv[2], NULL, 16);
	unsigned char data;
	
	
	printf("********************coming master_read_slave************************\n");
    
	// 初始化 I2C
    int err = i2c_init(&i2c_info_instance, 1);
	if (err != 0){
		printf("I2C initialization failed\n");
		return 0;
	}
	else
		printf("I2C initialization success\n");
	
	// 读取一个字节
    err = i2c_smbus_read_byte(0, slave_addr, offset, &data);
	if (err != 0){
		printf("read failed\n");
		return 0;
	}
    printf("Read data = : 0x%02x\n", data);
	
	return 0;
}

static int do_master_write_slave(int argc, char **argv)
{
	int slave_addr = strtoul(argv[1], NULL, 16);
	int offset = strtoul(argv[2], NULL, 16);
	unsigned char w_data = strtoul(argv[3], NULL, 16), r_data;

	printf("********************coming master_write_slave************************\n");
    
	// 初始化 I2C
    int err = i2c_init(&i2c_info_instance, 1);
	if (err != 0){
		printf("I2C initialization failed\n");
		return 0;
	}
	else
		printf("I2C initialization success\n");

	// // 读取一个字节
    // err = i2c_smbus_read_byte(0, slave_addr, offset, &r_data);
	// if (err != 0){
	// 	printf("read failed\n");
	// 	return 0;
	// }

    // printf("The initial value =  0x%02x\n", r_data);
	// printf("The value to write = 0x%02x\n", w_data);

	// 写入一个字节
	err = i2c_smbus_write_byte(0, slave_addr, offset, w_data);
	if (err != 0){
		printf("write failed\n");
		return 0;
	}
	else
		printf("write successful\n");
	
	// 读取一个字节
	i2c_smbus_read_byte(0, slave_addr, offset, &r_data);
	if (err != 0){
		printf("read failed\n");
		return 0;
	}

	printf("The value read after writing = 0x%02x\n", r_data);

	return 0;
}

static struct cmd_entry test_cmd_list[] __attribute__((unused)) = {
	{"read", do_master_read_slave, 2, "do_master_read_slave [slave addr] [offset]"},
	{"write", do_master_write_slave, 3, "do_master_write_slave [slave addr] [offset] [value]"},
	{NULL, NULL, 0, NULL}
	};


int testcase_i2c(void)
{
	int i, ret = 0;
	uartlog("enter i2c testcase \n");
	for (i = 0; i < ARRAY_SIZE(test_cmd_list) - 1; i++)
	{
		command_add(&test_cmd_list[i]);
	}
	cli_simple_loop();

	return ret;
}

#ifndef BUILD_TEST_CASE_ALL
int testcase_main()
{
  return testcase_i2c();
}
#endif