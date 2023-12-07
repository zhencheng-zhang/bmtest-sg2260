#include "boot_test.h"
#include "uart.h"
#include "cli.h"
#include "command.h"
#include "mmio.h"

#define thr rbr
#define iir fcr
#define dll rbr
#define dlm ier

struct dw_regs {
	volatile uint32_t	rbr;		/* 0x00 Data register */
	volatile uint32_t	ier;		/* 0x04 Interrupt Enable Register */
	volatile uint32_t	fcr;		/* 0x08 FIFO Control Register */
	volatile uint32_t	lcr;		/* 0x0C Line control register */
	volatile uint32_t	mcr;		/* 0x10 Line control register */
	volatile uint32_t	lsr;		/* 0x14 Line Status Register */
	volatile uint32_t	msr;		/* 0x18 Modem Status Register */
	volatile uint32_t	spr;		/* 0x20 Scratch Register */
};

#define UART_LCR_WLS_MSK 0x03       /* character length select mask */
#define UART_LCR_WLS_5  0x00        /* 5 bit character length */
#define UART_LCR_WLS_6  0x01        /* 6 bit character length */
#define UART_LCR_WLS_7  0x02        /* 7 bit character length */
#define UART_LCR_WLS_8  0x03        /* 8 bit character length */
#define UART_LCR_STB    0x04        /* # stop Bits, off=1, on=1.5 or 2) */
#define UART_LCR_PEN    0x08        /* Parity eneble */
#define UART_LCR_EPS    0x10        /* Even Parity Select */
#define UART_LCR_STKP   0x20        /* Stick Parity */
#define UART_LCR_SBRK   0x40        /* Set Break */
#define UART_LCR_BKSE   0x80        /* Bank select enable */
#define UART_LCR_DLAB   0x80        /* Divisor latch access bit */

#define UART_MCR_DTR    0x01        /* DTR   */
#define UART_MCR_RTS    0x02        /* RTS   */

#define UART_LSR_THRE		0x20 /* Transmit-hold-register empty */
#define UART_LSR_DR		0x01 /* Receiver data ready */
#define UART_LSR_TEMT   0x40        /* Xmitter empty */

#define UART_FCR_FIFO_EN    0x01 /* Fifo enable */
#define UART_FCR_RXSR       0x02 /* Receiver soft reset */
#define UART_FCR_TXSR       0x04 /* Transmitter soft reset */

#define UART_MCRVAL (UART_MCR_DTR | UART_MCR_RTS)      /* RTS/DTR */
#define UART_FCR_DEFVAL	(UART_FCR_FIFO_EN | UART_FCR_RXSR | UART_FCR_TXSR)
#define UART_LCR_8N1    0x03

#define UART_INTR	41

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

int uart_irq_handler(int irqn, void *priv)
{
	uartlog("----- IIR: %x irqn: %d ------\n", readl(UART0_BASE + 0x8), irqn);
	return 0;
}

static int uart_loopback_test(uint8_t data)
{
	uint8_t recv;

	uart_putc(data);
	recv = (uint8_t)uart_getc();
	if (data != recv)
		return -1;

	return 0;
}

static int do_test(int argc, char **argv)
{
	int ret;

	ret = uart_loopback_test('C');
	uartlog("testcase uart %s\n", ret?"failed":"passed");

	return ret;
}

extern int cli_readline(char *prompt, char *cmdbuf);

static int do_loop(int argc, char **argv)
{
	// uart_init();

	uartlog("uart init success\n");

	static char cli_command[65] = { 0, };

	// uint8_t c = (uint8_t)uart_getc();
	// uart_putc(c);
	// if (c == 'q')
    // 	return 0;

	while (1) {
		int len = cli_readline("$ ", cli_command);

		if (len > 0) {
			uart_puts(cli_command);
			uart_putc('\n');
		} else if (len == 0) {
		// repeat
			continue;
		} else if (len == -1) {
		// interrupt
			continue;
		} else if (len == -2) {
		// timeout
			uart_puts("\nTimed out waiting for command\n");
			return 0;
		}
	}

	return 0;
}


static int test_irq(int argc, char **argv)
{
	// uart_log("LSR: %x\n", mmio_read_32(UART0_BASE+0xc));
	mmio_write_32(UART0_BASE + 0x4, 0xf);
	request_irq(UART_INTR, uart_irq_handler, 0, "uart int", NULL);

	// test_uart0(0, NULL);
	return 0;
}

static struct cmd_entry test_cmd_list[] __attribute__((unused)) = {
	{ "run", do_test, 0, NULL },
	{ "loop", do_loop, 1, "loop baudrate" },
	{ "test_irq", test_irq, 0, NULL },
	//{ "loop", do_loop, 3, "loop port master/slave baudrate" },
	{ NULL, NULL, 0, NULL },
};

int testcase_uart(void)
{
	int i, ret = 0;

	test_irq(0, NULL);

	printf("enter uart test\n");

	for (i = 0; i < ARRAY_SIZE(test_cmd_list) - 1; i++) {
		command_add(&test_cmd_list[i]);
	}
	cli_simple_loop();

	return ret;
}

#ifndef BUILD_TEST_CASE_all
int testcase_main()
{
  int ret = testcase_uart();
  uartlog("testcase uart %s\n", ret?"failed":"passed");
  return ret;
}
#endif
