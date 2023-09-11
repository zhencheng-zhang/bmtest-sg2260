#include <stdio.h>
#include <timer.h>
#include <string.h>
#include <stdlib.h>
#include "asm/csr.h"
#include "smp.h"
#include "cli.h"
#include "command.h"
#include "thread_safe_printf.h"

#define STACK_SIZE	4096

#define current_hartid()	((unsigned int)read_csr(mhartid))
#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))

typedef struct {
	uint8_t stack[STACK_SIZE];
} core_stack;
static core_stack secondary_core_stack[CONFIG_SMP_NUM];

static void secondary_core_fun(void *priv)
{
	thread_safe_printf("hart id %u print hello world\n", current_hartid());

	mdelay(1000);
}

static int do_wakeup(int argc, char **argv)
{
	int chip_id;

	if (argc != 2) {
		thread_safe_printf("error input\n");
		return 0;
	}

	chip_id = strtoul(argv[1], NULL, 10);
	wake_up_other_core(chip_id, secondary_core_fun, NULL,
				   &secondary_core_stack[chip_id], STACK_SIZE);

	return 0;
}

static struct cmd_entry test_cmd_list[] __attribute__ ((unused)) = {
	{"wakeup", do_wakeup, 1, " [chip-id]"},
	{NULL, NULL, 0, NULL}
};

static int testcase_smp(void)
{
	unsigned int hartid = current_hartid();
	int ret;

	thread_safe_printf("main core id = %u\n", hartid);

	for (int i = 0; i < CONFIG_SMP_NUM; i++) {
		if (i == hartid)
			continue;
		wake_up_other_core(i, secondary_core_fun, NULL,
				   &secondary_core_stack[i], STACK_SIZE);
	}

	thread_safe_printf("hello, main core id = %u\n", current_hartid());

	mdelay(1000);
	for(int i = 0;i < ARRAY_SIZE(test_cmd_list) - 1;i++) {
		command_add(&test_cmd_list[i]);
	}

	do {
		ret = cli_simple_loop();
  	} while (ret == 0);

	return 0;
}

#ifndef BUILD_TEST_CASE_ALL
int testcase_main()
{
  return testcase_smp();
}
#endif