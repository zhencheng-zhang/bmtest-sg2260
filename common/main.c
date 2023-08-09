#include "boot_test.h"
#include "mmio.h"
#include "system_common.h"

extern module_func_t __module_testcase_start[];
extern module_func_t __module_testcase_end[];

// #define BUILD_TEST_CASE_ALL

#ifdef BUILD_TEST_CASE_ALL
int testcase_main(void)
{
	int ret;
	module_func_t *func;

	for(func = __module_testcase_start; func < __module_testcase_end; func++)
	{
		ret = (*func)();
		if(ret)
  			return ret;
	}

	return ret;
}
#else
extern int testcase_main();
#endif

int main(void)
{
	int ret;
	uartlog("%s %s\n", __DATE__, __TIME__);
	uartlog("SG2260 bmtest start\n");

	ret = testcase_main();
	uartlog("test done\n");

	return ret;
}
