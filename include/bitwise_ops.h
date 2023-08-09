#ifndef __BITWISE_OPS_H__
#define __BITWISE_OPS_H__

#include <stdint.h>

#define BIT(nr)				(1U << (nr))
#define BITMASK(msb, lsb)	((2U << (msb))-(1U << (lsb)))

static inline uint32_t modified_bits_by_value(uint32_t orig, uint32_t value, uint32_t msb, uint32_t lsb)
{
	uint32_t bitmask = BITMASK(msb, lsb);

	orig &= ~bitmask;
	return (orig | ((value << lsb) & bitmask));
}

static inline uint32_t get_bits_from_value(uint32_t value, uint32_t msb, uint32_t lsb)
{
	// if (msb < lsb)
	//     uartlog("%s: msb %u < lsb %u\n", __func__, msb, lsb);
	return ((value & BITMASK(msb, lsb)) >> lsb);
}

#endif /* __BITWISE_OPS_H__ */
