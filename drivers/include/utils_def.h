/*
 * Copyright (c) 2016-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __UTILS_DEF_H__
#define __UTILS_DEF_H__

#define u64 uint64_t
#define u32 uint32_t
#define u16 uint16_t

/* Compute the number of elements in the given array */
#define ARRAY_SIZE(a)				\
	(sizeof(a) / sizeof((a)[0]))

#define IS_POWER_OF_TWO(x)			\
	(((x) & ((x) - 1)) == 0)

#define SIZE_FROM_LOG2_WORDS(n)		(4 << (n))

#define BIT(nr)				(1UL << (nr))

#define MIN(x, y) __extension__ ({	\
	__typeof__(x) _x = (x);		\
	__typeof__(y) _y = (y);		\
	(void)(&_x == &_y);		\
	_x < _y ? _x : _y;		\
})

#define MAX(x, y) __extension__ ({	\
	__typeof__(x) _x = (x);		\
	__typeof__(y) _y = (y);		\
	(void)(&_x == &_y);		\
	_x > _y ? _x : _y;		\
})

/*
 * The round_up() macro rounds up a value to the given boundary in a
 * type-agnostic yet type-safe manner. The boundary must be a power of two.
 * In other words, it computes the smallest multiple of boundary which is
 * greater than or equal to value.
 *
 * round_down() is similar but rounds the value down instead.
 */
#define round_boundary(value, boundary)		\
	((__typeof__(value))((boundary) - 1))

#define round_up(value, boundary)		\
	((((value) - 1) | round_boundary(value, boundary)) + 1)

#define round_down(value, boundary)		\
	((value) & ~round_boundary(value, boundary))

/*
 * Evaluates to 1 if (ptr + inc) overflows, 0 otherwise.
 * Both arguments must be unsigned pointer values (i.e. uintptr_t).
 */
#define check_uptr_overflow(ptr, inc)		\
	(((ptr) > UINTPTR_MAX - (inc)) ? 1 : 0)

/*
 * For those constants to be shared between C and other sources, apply a 'u'
 * or 'ull' suffix to the argument only in C, to avoid undefined or unintended
 * behaviour.
 *
 * The GNU assembler and linker do not support the 'u' and 'ull' suffix (it
 * causes the build process to fail) therefore the suffix is omitted when used
 * in linker scripts and assembler files.
*/
#if defined(__LINKER__) || defined(__ASSEMBLY__)
# define  U(_x)		(_x)
# define ULL(_x)	(_x)
#else
# define  U(_x)		(_x##u)
# define ULL(_x)	(_x##ull)
#endif

#ifndef __ASSEMBLY__
#define zeromem(a, n)		memset(a, 0, n)

#ifndef __riscv
static inline int fls(int x)
{
	int r = 32;

	if (!x)
		return 0;
	if (!(x & 0xffff0000u)) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & 0xff000000u)) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & 0xf0000000u)) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & 0xc0000000u)) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & 0x80000000u)) {
		x <<= 1;
		r -= 1;
	}
	return r;
}
#endif /* ! __riscv */
#endif /* __ASSEMBLY__ */

#endif /* __UTILS_DEF_H__ */
