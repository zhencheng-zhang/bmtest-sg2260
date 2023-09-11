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

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static int uart_loopback_test(uint8_t data)
{
	uint8_t recv;

	uart_putc(data);
	recv = (uint8_t)uart_getc();
	if (data != recv)
		return -1;

	return 0;
}

static inline void uart2_pinmux(void)
{
	int pin;

	//pinmux uart2
  // Pin Mux and IO Control Register for UART1_RX and UART2_TX (0x50010484)
	pin = mmio_read_32(0x50010484);
	pin = pin & ~(0x3 << 20);
	pin = pin | 1 << 20;
	mmio_write_32(0x50010484, pin);

  // Pin Mux and IO Control Register for UART2_RX and GPIO0 (0x50010488)
	pin = mmio_read_32(0x50010488);
	pin = pin & ~(0x3 << 4);
	pin = pin | 1 << 4;
	mmio_write_32(0x50010488, pin);

}

static int do_test(int argc, char **argv)
{
	int ret;

	ret = uart_loopback_test('C');
	uartlog("testcase uart %s\n", ret?"failed":"passed");

	return ret;
}

static int do_loop(int argc, char **argv)
{
	// int baudrate;
	// struct dw_regs *send, *recv;
	char ch = 0;

	// uart2_pinmux();

	// baudrate = strtoul(argv[1], NULL, 10);

	// printf("baudrate %d\n", baudrate);

loop:
	uart_puts("bmtest uart loop test\n");
	int str_len = 0;
	do {
		uart_init(0);
		ch = (char)uart_getc();
		uart_putc(ch);
		uart_init(1);
		uart_putc(ch);
		str_len++;
	} while (ch != '\n');

	if (str_len <= 1)
		return 0;

	timer_mdelay(500);
	goto loop;

	return 0;
}

static int test_uart0(int argc, char **argv)
{
	// int baudrate;
	// struct dw_regs *send, *recv;
	char ch = 0;

	// uart2_pinmux();

	// baudrate = strtoul(argv[1], NULL, 10);

	// printf("baudrate %d\n", baudrate);

loop:
	uart_puts("bmtest uart0 test\n");
  int str_len = 0;
	do {
		uart_init(0);
		ch = (char)uart_getc();
		uart_putc(ch);
    // uart_init(1);
		// uart_putc(ch);
    str_len++;
	} while (ch != '\n');

	if (str_len <= 1)
    	return 0;

	timer_mdelay(500);
	goto loop;

	return 0;
}

static int test_uart1(int argc, char **argv)
{
	// int baudrate;
	// struct dw_regs *send, *recv;
	char ch = 0;

	// uart2_pinmux();

	// baudrate = strtoul(argv[1], NULL, 10);

	// printf("baudrate %d\n", baudrate);

loop:
	uart_puts("bmtest uart1 test\n");
	int str_len = 0;
	do {
		uart_init(0);
		ch = (char)uart_getc();
		// uart_putc(ch);
		uart_init(1);
		uart_putc(ch);
		str_len++;
	} while (ch != '\n');

	if (str_len <= 1)
		return 0;

	timer_mdelay(500);
	goto loop;

	return 0;
}

static struct cmd_entry test_cmd_list[] __attribute__((unused)) = {
	{ "run", do_test, 0, NULL },
	{ "test_uart0", test_uart0, 0, NULL },
	{ "test_uart1", test_uart1, 0, NULL },
	{ "test_uartall", do_loop, 1, "loop baudrate" },
	//{ "loop", do_loop, 3, "loop port master/slave baudrate" },
	{ NULL, NULL, 0, NULL },
};

int testcase_uart(void)
{
	int i, ret = 0;

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
