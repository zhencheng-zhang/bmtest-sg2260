/*
 * Copyright (C) 2018 Bitmain Technologies Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include "io.h"
#include "system_common.h"
#include "mmio.h"

int request_irq(unsigned int irqn, irq_handler_t handler, unsigned long flags,
                 const char *name, void *priv);

static int printFlag = 0;
static int irq_handler(int irqn, void *priv)
{
	if (printFlag != irqn) {
		printf("#### [SG2260]irqn = %d ####\n", irqn);
		printFlag = irqn;
    }

	return 0;
}

int bm_riscv_test(void)
{
	printf("SG2260 irq test...\n");

	for(int irqNum = 16; irqNum < 256; irqNum++) {
        if (request_irq(irqNum , irq_handler, 0, NULL, NULL))
			printf("request_irq [%d] failed!\n", irqNum);
	}

	printf("SG2260 reqest irq success.\n");

	while(1);

	return 0;
}

#ifndef BUILD_TEST_CASE_all
int testcase_main(void)
{
	return bm_riscv_test();
}
#endif