
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

/*
   Bi-directional linear memory allocator
 */

enum {
	LMA_LOW,
	LMA_HIGH
};

typedef char *lma_addr_t;

struct lma {
	lma_addr_t base;
	lma_addr_t end;
	lma_addr_t ends[2];
#ifdef LMA_DEBUG
	int debug;
#endif
};

_lma_alwaysinline
void lma_init(struct lma *lma, void *base, size_t size) {
	_lma_assert(((uintptr_t) base & 15) == 0);
	_lma_assert((size & 15) == 0);
	lma->base = lma->ends[LMA_LOW] = (lma_addr_t) base;
	lma->end = lma->ends[LMA_HIGH] = (lma_addr_t) base + size;
#ifdef LMA_DEBUG
	lma->debug = 0;
#endif
}

_lma_alwaysinline void lma_reset_low(struct lma *lma) { lma->ends[LMA_LOW] = lma->base; }
_lma_alwaysinline void lma_reset_high(struct lma *lma) { lma->ends[LMA_HIGH] = lma->end; }

_lma_alwaysinline size_t lma_avail(const struct lma *lma) { return lma->ends[LMA_HIGH] - lma->ends[LMA_LOW]; }
_lma_alwaysinline size_t lma_inuse_low(const struct lma *lma) { return lma->ends[LMA_LOW] - lma->base; }
_lma_alwaysinline size_t lma_inuse_high(const struct lma *lma) { return lma->end - lma->ends[LMA_HIGH]; }
_lma_alwaysinline size_t lma_inuse(const struct lma *lma, int area) {
	return (area == LMA_LOW ? lma_inuse_low(lma) : lma_inuse_high(lma));
}

_lma_malloc _lma_alwaysinline
void *lma_alloc_low(struct lma *lma, size_t size) {
	lma_addr_t low = lma->ends[LMA_LOW], nxt = lma->ends[LMA_LOW] + ((size + 15) & ~15);
	return (lma->ends[LMA_HIGH] >= nxt) ? lma->ends[LMA_LOW] = nxt, low : NULL;
}

_lma_malloc _lma_alwaysinline
void *lma_alloc_high(struct lma *lma, size_t size) {
	lma_addr_t high = lma->ends[LMA_HIGH] - ((size + 15) & ~15);
	return (lma->ends[LMA_LOW] <= high) ? lma->ends[LMA_HIGH] = high : NULL;
}

_lma_malloc _lma_alwaysinline
void *lma_alloc(struct lma *lma, int area, size_t size) {
	return (area == LMA_LOW ? lma_alloc_low(lma, size) : lma_alloc_high(lma, size));
}

_lma_unused _lma_format(3, 4)
static int lma_asprintf_low(struct lma *lma, char **ret, const char *fmt, ...) {
	lma_addr_t low = lma->ends[LMA_LOW];
	size_t size = lma_avail(lma);
	va_list va;
	va_start(va, fmt);
	int n = vsnprintf(low, size, fmt, va);
	va_end(va);
	if (n >= 0 && (size_t) n < size) {
		lma->ends[LMA_LOW]= low + ((n + 16) & ~15);
		*ret = low;
	} else
		*ret = NULL;
	return n;
}

/*
   Utility for scoping temporary allocations, this allows ping-ponging of
   temporary allocations between the high and low areas in nested call sites.

	void foo(struct lma_scope *ls) {
		const struct lma_scope ll = lma_push(ls);
		void *tmp = lma_alloc(ll.lma, ll.area, ...);
		...
		lma_pop(ll);
	}
 */

struct lma_scope {
	struct lma *lma;
	lma_addr_t end;
	int area;
};

_lma_alwaysinline struct lma_scope lma_push(struct lma_scope *ls) {
	return (struct lma_scope) {ls->lma, ls->lma->ends[ls->area ^ LMA_HIGH], ls->area ^ LMA_HIGH};
}

_lma_alwaysinline void lma_pop(struct lma_scope ls) {
	ls.lma->ends[ls.area] = ls.end;
}

/*
   Debug helper, useful to log activity of a single allocator (e.g. to disk),
   enable with lma_debug(lma, 1) after initializing the allocator.
 */

#ifndef LMA_DEBUG
# define lma_debug(lma,on) ((void) 0)
#else
# include <string.h>
# ifndef _lma_debugf
#  define _lma_debugf(...) fprintf(stderr, __VA_ARGS__)
# endif
# define lma_debug(lma,on) do { (lma)->debug = (on); } while (0)
# define lma_reset_low(lma) \
	(lma_reset_low(lma), (void) _lma_debug(NULL, lma, "reset", LMA_LOW, 0, __FILE__, __LINE__, __func__))
# define lma_reset_high(lma) \
	(lma_reset_high(lma), (void) _lma_debug(NULL, lma, "reset", LMA_HIGH, 0, __FILE__, __LINE__, __func__))
# define lma_alloc_high(lma,size) \
	_lma_debug(lma_alloc_high(lma, size), lma, "alloc", LMA_HIGH, size, __FILE__, __LINE__, __func__)
# define lma_alloc_low(lma,size) \
	_lma_debug(lma_alloc_low(lma, size), lma, "alloc", LMA_LOW, size, __FILE__, __LINE__, __func__)
# define lma_alloc(lma,area,size) \
	_lma_debug(lma_alloc(lma, area, size), lma, "alloc", area, size, __FILE__, __LINE__, __func__)
# define lma_asprintf_low(lma,ret,fmt,...) \
	_lma_debug_asprintf( \
		lma_asprintf_low(lma, ret, fmt, __VA_ARGS__), *(ret), lma, "alloc", LMA_LOW, \
		(*(ret) ? strlen(*(ret)) + 1 : 0), __FILE__, __LINE__, __func__)
_lma_alwaysinline void *_lma_debug(
		void *p, struct lma *lma, const char *what, int area, size_t size,
		const char *file, int line, const char *func) {
	if (lma->debug)
		_lma_debugf(
# if _MSC_VER
			"lma: %s:%d:%s: %14p-%14p:%08Ix %08Ix-%08Ix %04.01f%% %s %-4s %14p:%08Ix\n",
# else
			"lma: %s:%d:%s: %14p-%14p:%08zx %08zx-%08zx %04.01f%% %s %-4s %14p:%08zx\n",
# endif
			file, line, func, lma->base, lma->end, lma->end - lma->base,
			lma_inuse_low(lma), lma_inuse_high(lma),
			(double) ((lma->end - lma->base) - lma_avail(lma)) / (lma->end - lma->base) * 100.0,
			what, area ? "high" : "low", p, size);
	return p;
}
_lma_alwaysinline int _lma_debug_asprintf(
		int err, void *p, struct lma *lma, const char *what, int area, size_t size,
		const char *file, int line, const char *func) {
	_lma_debug(p, lma, what, area, size, file, line, func);
	return err;
}
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AW_LMA_H */

