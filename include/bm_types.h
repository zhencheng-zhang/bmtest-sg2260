#ifndef __BM_TYPES_H__
#define __BM_TYPES_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef u32 laddr_t;
typedef u64 gaddr_t;

typedef union {
  int ival;
  char fval;
} IC_VAL;

typedef u32 BLOB_OP;
#define BLOB_ADD 0
#define BLOB_SUB 1
#define BLOB_MUL 2
#define BLOB_DIV 3
#define BLOB_INVALID 4

typedef u32 SFU_OP;
#define SFU_XN 0  // POWN
#define SFU_EX 1  // EXP
#define SFU_LNX 2  // LNX
#define SFU_RSQ 3  // RSQ
#define SFU_XA 4  // POWEREXP
#define SFU_INVALID 5

typedef u32 TENSOR_OP;
#define TENSOR_ADD 0
#define TENSOR_SUB 1
#define TENSOR_MUL 2
#define TENSOR_DIV 3
#define TENSOR_MAX 4
#define TENSOR_CPY 5
#define TENSOR_MAC 6

#define TENSOR_MUL_FIX8B 0
#define TENSOR_MAC_FIX8B 1
#define TENSOR_ADD_FIX8B 2
#define TENSOR_SUB_FIX8B 3
#define TENSOR_MAX_FIX8B 4
#define TENSOR_MIN_FIX8B 5
#define TENSOR_SHIFT_FIX8B 6
#define TENSOR_AND_FIX8B 7
#define TENSOR_OR_FIX8B 8
#define TENSOR_XOR_FIX8B 9
#define TENSOR_COPY_FIX8B 10

typedef u32 LINEAR_OP;
#define LINEAR_MAC 0
#define LINEAR_ADD_SQR 1
#define LINEAR_SUB_SQR 2
#define LINEAR_INVALID 3

#define FLOAT_SIZE 4
#define INT16_SIZE 2
#define INT8_SIZE 1

#define UNUSED(x)               (void)(x)

#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))
#define ALIGN(x,a)              __ALIGN_MASK(x,(__typeof__(x))(a)-1)
#define ALIGN_DOWN(x, a)        ((x) / (a) * (a))

#define math_min(x, y)          ((x) < (y) ? (x) : (y))
#define math_max(x, y)          ((x) > (y) ? (x) : (y))

#define ACCESS_ONCE(x)          (*(volatile typeof(x) *)&(x))
#define barrier()               asm volatile("": : :"memory")

#define RAW_READ32(x)           (*(volatile u32 *)(x))
#define RAW_WRITE32(x, val)     (*(volatile u32 *)(x) = (u32)(val))

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* __BM_TYPES_H__ */
