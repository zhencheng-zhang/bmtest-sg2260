/*
 * Copyright (c) 2013-2014, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h> /* size_t */
#include <assert.h>

/*
 * Fill @count bytes of memory pointed to by @dst with @val
 */
void *memset(void *dst, int val, size_t count)
{
	char *ptr = dst;

	while (count--)
		*ptr++ = val;

	return dst;
}

void memset_dword(void *buff, unsigned int val, unsigned int size)
{
	unsigned int *buff32 = (unsigned int*)buff;
	unsigned int size_dw = size >> 2;
	while (size_dw--){
		*buff32++ = val;
	}
}

void memset_dword_inc(void *buff, unsigned int start, unsigned int size)
{
	unsigned int *buff32 = (unsigned int*)buff;
	assert((size % 4) == 0);
	unsigned int size_dw = size >> 2;
	while (size_dw--){
		*buff32++ = start++;
	}
}

/*
 * Compare @len bytes of @s1 and @s2
 */
int memcmp(const void *s1, const void *s2, size_t len)
{
	int cnt = 0;
	const unsigned char *s = s1;
	const unsigned char *d = s2;
	unsigned char sc;
	unsigned char dc;

	while (len--) {
		sc = *s++;
		dc = *d++;
		cnt++;
		if (sc - dc)
			return cnt;
	}

	return 0;
}

/*
 * Copy @len bytes from @src to @dst
 */
void *memcpy(void *dst, const void *src, size_t len)
{
	const char *s = src;
	char *d = dst;

	while (len--)
		*d++ = *s++;

	return dst;
}

/*
 * Copy @len bytes from @src to @dst
 */
void *memcpy_dword(void *dst, const void *src, size_t len)
{
	const int *s = src;
	int *d = dst;

	len /= 4;

	while (len--)
		*d++ = *s++;

	return dst;
}


/*
 * Move @len bytes from @src to @dst
 */
void *memmove(void *dst, const void *src, size_t len)
{
	/*
	 * The following test makes use of unsigned arithmetic overflow to
	 * more efficiently test the condition !(src <= dst && dst < str+len).
	 * It also avoids the situation where the more explicit test would give
	 * incorrect results were the calculation str+len to overflow (though
	 * that issue is probably moot as such usage is probably undefined
	 * behaviour and a bug anyway.
	 */
	if ((size_t)dst - (size_t)src >= len) {
		/* destination not in source data, so can safely use memcpy */
		return memcpy(dst, src, len);
	} else {
		/* copy backwards... */
		const char *end = dst;
		const char *s = (const char *)src + len;
		char *d = (char *)dst + len;
		while (d != end)
			*--d = *--s;
	}
	return dst;
}

/*
 * Scan @len bytes of @src for value @c
 */
void *memchr(const void *src, int c, size_t len)
{
	const char *s = src;

	while (len--) {
		if (*s == c)
			return (void *) s;
		s++;
	}

	return NULL;
}
