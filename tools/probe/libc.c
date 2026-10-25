// SPDX-License-Identifier: GPL-2.0-only
/* Minimal freestanding memcpy/memset for the EFI probe (no CRT). */
#include <stddef.h>

#pragma function(memcpy)
#pragma function(memset)

void *memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dst;
}

void *memset(void *dst, int value, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    while (n--) *d++ = (unsigned char)value;
    return dst;
}
