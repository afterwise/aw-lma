
/*
   Copyright (c) 2014 Malte Hildingsson, malte (at) afterwi.se

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

#include <stdarg.h>
#include <stdio.h>

#if !_MSC_VER
# include <stdint.h>
#endif

#if __GNUC__
# define _lma_alwaysinline inline __attribute__((always_inline))
# define _lma_format(a,b) __attribute__((format(__printf__,a,b)))
# define _lma_malloc __attribute__((malloc, warn_unused_result))
#elif _MSC_VER
# define _lma_alwaysinline __forceinline
# define _lma_format(a,b)
# define _lma_malloc
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct lma {
	uintptr_t base;
	uintptr_t brk;
	size_t size;
};

static _lma_alwaysinline void lma_init(struct lma *lma, void *base, size_t size) {
	lma->base = (uintptr_t) base;
	lma->brk = (uintptr_t) base;
	lma->size = size;
}

static _lma_alwaysinline void *lma_getbrk(struct lma *lma) {
	return (void *) lma->brk;
}

static _lma_alwaysinline void lma_setbrk(struct lma *lma, void *brk) {
	lma->brk = (uintptr_t) brk;
}

static _lma_alwaysinline void lma_reset(struct lma *lma) {
	lma->brk = lma->base;
}

static _lma_alwaysinline size_t lma_used(struct lma *lma) {
	return lma->brk - lma->base;
}

static _lma_alwaysinline _lma_malloc void *lma_alloc(struct lma *lma, size_t size) {
	uintptr_t brk = lma->brk;
	lma->brk += (size + 15) & ~15;
	return (lma->base + lma->size >= brk + size) ? (void *) brk : NULL;
}

#ifdef _need_lma_asprintf
static char *lma_asprintf(struct lma *lma, const char *fmt, ...) _lma_format(2, 3) _lma_malloc;
static char *lma_asprintf(struct lma *lma, const char *fmt, ...) {
	va_list va;
	char *p = lma_getbrk(lma);
	size_t m = lma->size - lma_used(lma);
	int n;

	va_start(va, fmt);
	n = vsnprintf(p, m, fmt, va);
	va_end(va);

	if (n >= 0 && (size_t) n < m) {
		lma->brk += (n + 16) & ~15;
		return p;
	}

	return NULL;
}
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AW_LMA_H */

