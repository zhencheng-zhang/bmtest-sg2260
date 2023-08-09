#ifndef _COMMON_H_
#define _COMMON_H_

#include "bmtest_type.h"
#include "debug.h"

typedef u32 ENGINE_ID;
#define ENGINE_BD 0
#define ENGINE_ARM 1
#define ENGINE_GDMA 2
#define ENGINE_CDMA 3
#define ENGINE_END 4

typedef struct _cmd_id_node_s {
	unsigned int bd_cmd_id;
	unsigned int gdma_cmd_id;
	unsigned int cdma_cmd_id;
} CMD_ID_NODE;

typedef void * P_TASK_DESCRIPTOR;

#define RAW_READ32(x)		(*(volatile u32 *)(x))
#define RAW_WRITE32(x, val)		(*(volatile u32 *)(x) = (u32)(val))

#define RAW_REG_READ32(base, off)		RAW_READ32(((u8 *)(base)) + (off))
#define RAW_REG_WRITE32(base, off, val)		RAW_WRITE32(((u8 *)(base)) + (off), (val))
#define RAW_MEM_READ32(base, off)		RAW_READ32(((u8 *)(base)) + (off))
#define RAW_MEM_WRITE32(base, off, val)		RAW_WRITE32(((u8 *)(base)) + (off), (val))

#define FW_ADDR_T		unsigned long

static inline int calc_offset(int *shape, int *offset)
{
	return ((offset[0] * shape[1] + offset[1]) * shape[2] + offset[2])
		 * shape[3] + offset[3];
}

#define NPU_NUM		32
#define EU_NUM		16
#define MAX_NPU_NUM		32
#define MAX_EU_NUM		16
#define REG_BM1682_TOP_SOFT_RST		0x014

enum status_enum{
	STATUS_PRE,   /* before test*/
	STATUS_RUN,   /* testing */
	STATUS_CKE,   /* finish test,tcl check*/
	STATUS_END,   /* finish check */
};

struct comm
{
	int test_id;
	char test_name[20];
	enum status_enum status;   
	int in_addr;
	int in_size;
	int out_addr;
	int out_size;
};

#endif /* _COMMON_H_ */
