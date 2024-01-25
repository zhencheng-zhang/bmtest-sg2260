#include <stdint.h>

#include "system_common.h"

#define thr rbr
#define iir fcr
#define dll rbr
#define dlm ier

#define UART_PCLK	153600
#define UART_BAUDRATE	9600

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

static struct dw_regs *uart = (struct dw_regs *)UART0_BASE;

#if 0
void uart_init(int port, int baudrate)
{
	switch (port)
	{
	case 0:
		uart = (struct dw_regs *)UART0_BASE;
		break;
	case 1:
		uart = (struct dw_regs *)UART1_BASE;
		break;
	case 2:
		uart = (struct dw_regs *)UART2_BASE;
		break;
	case 3:
		uart = (struct dw_regs *)UART3_BASE;
		break;
	default:
		uart = (struct dw_regs *)UART0_BASE;// default
		return;
	}

#if (defined(BOARD_FPGA))
	// int baudrate = 115200;
	int uart_clock = 25 * 1000 * 1000;
#elif defined(PLATFORM_PALLADIUM)
	// int baudrate = 153600;
	int uart_clock = 153600;
#else
	// int baudrate = 115200;
	int uart_clock = 500 * 1000 * 1000;
#endif

	int divisor = uart_clock / (16 * baudrate);

	uart->lcr = uart->lcr | UART_LCR_DLAB | UART_LCR_8N1;
	uart->dll = divisor & 0xff;
	uart->dlm = (divisor >> 8) & 0xff;
	uart->lcr = uart->lcr & (~UART_LCR_DLAB);

#if defined(UART_INTR_TEST)
	uart->ier = 0x5;
#else
	uart->ier = 0;
#endif
	uart->mcr = UART_MCRVAL;
	uart->fcr = UART_FCR_DEFVAL;

	uart->lcr = 3;
}
#else
// void uart_init(void) {}
int uart_init(unsigned int baudrate)
{
	unsigned int divisor;

	// unsigned int baudrate = UART_BAUDRATE;
	// unsigned int pclk = UART_PCLK;
	unsigned int pclk = 25000000;
	volatile uint32_t *usr = (uint32_t *)0x703000007c;

	divisor = pclk / (16 * baudrate);
	uartlog("divisor: %d\n", divisor);

	/* if any interrupt has been enabled, that means this uart controller
	 * may be initialized by some one before, just use it without
	 * reinitializing. such situation occur when main cpu and a53lite share
	 * the same uart port. main cpu should bringup first, reinitialize uart
	 * may cause unpredictable bug, especially disable all interrupts, which
	 * will cause linux running on main cpu lose of interrupts and cannot
	 * type into any character in serial console
	 */
	if (uart->ier == 0) {
		while (1) {
			if ((*usr & 0x1) != 0x1)
				break;
		}

		uart->lcr = uart->lcr | UART_LCR_DLAB | UART_LCR_8N1;
		uart->dll = divisor & 0xff;
		uart->dlm = (divisor >> 8) & 0xff;
		uart->lcr = uart->lcr & (~UART_LCR_DLAB);

		uartlog("to set ier\n");

		// uart->ier = 0x8f;
		uart->ier = 0;
		uart->mcr = UART_MCRVAL;
		uart->fcr = UART_FCR_DEFVAL;

		uart->lcr = 3;
	}

	// register_stdio(uart_getc, uart_putc);

	return 0;
}
#endif

void _uart_putc(uint8_t ch)
{
	while (!(uart->lsr & UART_LSR_THRE))
		;
	uart->rbr= ch;
}

void uart_putc(uint8_t ch)
{
	if (ch == '\n') {
		_uart_putc('\r');
	}
	_uart_putc(ch);
}

void uart_puts(char *str)
{
	if (!str)
		return;

	while (*str) {
		uart_putc(*str++);
	}
}

int uart_getc(void)
{
	while (!(uart->lsr & UART_LSR_DR))
		;
	return (int)uart->rbr;
}

int uart_tstc(void)
{
	return (!!(uart->lsr & UART_LSR_DR));
}
