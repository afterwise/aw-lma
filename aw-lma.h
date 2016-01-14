
/*
   Copyright (c) 2014-2016 Malte Hildingsson, malte (at) afterwi.se

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
 */

#ifndef AW_LMA_H
#define AW_LMA_H

#if !_MSC_VER || _MSC_VER >= 1600
# include <stdint.h>
#endif

#include <stdarg.h>
#include <stdio.h>

#ifndef _lma_assert
# include <assert.h>
# define _lma_assert(x) assert(x)
#endif

#if __GNUC__
# define _lma_alwaysinline inline __attribute__((always_inline))
# define _lma_format(a,b) __attribute__((format(__printf__,a,b)))
# define _lma_malloc __attribute__((malloc, warn_unused_result))
# define _lma_unused __attribute__((__unused__))
#elif _MSC_VER
# define _lma_alwaysinline __forceinline
# define _lma_format(a,b)
# define _lma_malloc
# define _lma_unused
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Bi-directional linear memory allocator */

typedef char *lma_addr_t;

struct lma {
	lma_addr_t base;
	lma_addr_t end;
	lma_addr_t low;
	lma_addr_t high;
};

_lma_alwaysinline
void lma_init(struct lma *lma, void *base, size_t size) {
	_lma_assert(((uintptr_t) base & 15) == 0);
	_lma_assert((size & 15) == 0);
	lma->base = lma->low = (lma_addr_t) base;
	lma->end = lma->high = (lma_addr_t) base + size;
}

_lma_alwaysinline void lma_reset_low(struct lma *lma) { lma->low = lma->base; }
_lma_alwaysinline void lma_reset_high(struct lma *lma) { lma->high = lma->end; }

_lma_alwaysinline size_t lma_avail(struct lma *lma) { return lma->high - lma->low; }
_lma_alwaysinline size_t lma_inuse_low(struct lma *lma) { return lma->low - lma->base; }
_lma_alwaysinline size_t lma_inuse_high(struct lma *lma) { return lma->end - lma->high; }

_lma_malloc _lma_alwaysinline
void *lma_alloc_low(struct lma *lma, size_t size) {
	lma_addr_t low = lma->low, nxt = lma->low + ((size + 15) & ~15);
	return (lma->high >= nxt) ? lma->low = nxt, low : NULL;
}

_lma_malloc _lma_alwaysinline
void *lma_alloc_high(struct lma *lma, size_t size) {
	lma_addr_t high = lma->high - ((size + 15) & ~15);
	return (lma->low <= high) ? lma->high = high : NULL;
}

_lma_unused _lma_format(3, 4)
static int lma_asprintf_low(struct lma *lma, char **ret, const char *fmt, ...) {
	lma_addr_t low = lma->low;
	size_t size = lma_avail(lma);
	va_list va;
	va_start(va, fmt);
	int n = vsnprintf(low, size, fmt, va);
	va_end(va);
	if (n >= 0 && (size_t) n < size) {
		lma->low = low + ((n + 16) & ~15);
		*ret = low;
	} else
		*ret = NULL;
	return n;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AW_LMA_H */

