#include "boot_test.h"
#include "cli.h"
#include "command.h"
#include "system_common.h"

int testcase_hello()
{
    printf("hello 2260 user!\n");
    return 0;
}

#ifndef BUILD_TEST_CASE_ALL
int testcase_main()
{
  return testcase_hello();
}
#endif