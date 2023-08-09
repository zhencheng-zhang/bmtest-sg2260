#include "boot_test.h"
#include "uart.h"
#include "cli.h"

int uart_loopback_test(uint8_t data)
{
  uint8_t recv;

  uart_putc(data);
  recv = (uint8_t)uart_getc();
  if (data != recv)
    return -1;

  return 0;
}

int testcase_uart(void)
{
  int ret;

  uartlog("%s\n", __func__);

  do {
    ret = cli_simple_loop();
  } while (ret == 0);
  return uart_loopback_test('C');
}

#ifndef BUILD_TEST_CASE_all
int testcase_main()
{
  int ret = testcase_uart();
  uartlog("testcase uart %s\n", ret?"failed":"passed");
  return ret;
}
#endif
