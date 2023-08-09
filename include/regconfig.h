#ifndef __REG_CFG_H__
#define __REG_CFG_H__

#include <stdint.h>
#include "ddr_sys.h"

#define ARRAY_SIZE(arr)		(sizeof(arr) / sizeof((arr)[0]))

typedef struct regconf {
	uint32_t addr;
	uint32_t val;
} regconf_t;

extern uint32_t ddr_data_header;
extern uint16_t ddr_data_version;
extern uint32_t ddr_conf_addr;
extern uint32_t ddr_data_rate;
extern regconf_t ddrc_regs[];
extern uint32_t ddrc_regs_count;
extern uint32_t pi_regs[];
extern uint32_t pi_regs_count;
extern regconf_t phy_regs[];
extern uint32_t phy_regs_count;
extern regconf_t ddrc_patch_regs[];
extern uint32_t ddrc_patch_regs_count;

#endif /* __REG_CFG_H__ */
