/* vim: set ts=4 sw=4 noet : */

/*
   Copyright (c) 2014-2023 Malte Hildingsson, malte (at) afterwi.se

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

#if !defined(_MSC_VER) || _MSC_VER >= 1600
# include <stdint.h>
#endif

#include <stdarg.h>
#include <stdio.h>

#if !defined(_lma_assert)
# include <assert.h>
# define _lma_assert(x) assert(x)
#endif

#if defined(_HAS_CXX17)
# define _lma_unused [[maybe_unused]]
#elif defined(__GNUC__)
# define _lma_unused __attribute__((unused))
#elif defined(_MSC_VER)
# define _lma_unused
#endif

#if defined(__GNUC__)
# define _lma_alwaysinline inline __attribute__((always_inline))
# define _lma_format(a,b) __attribute__((format(__printf__,a,b)))
# define _lma_malloc __attribute__((malloc, warn_unused_result))
#elif defined(_MSC_VER)
# define _lma_alwaysinline __forceinline
# define _lma_format(a,b)
# define _lma_malloc
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
	lma_addr_t brks[2];
#ifdef LMA_DEBUG
	int debug;
#endif
};

_lma_alwaysinline
void lma_init(struct lma *lma, void *base, size_t size) {
	_lma_assert(((uintptr_t) base & 15) == 0);
	_lma_assert((size & 15) == 0);
	lma->base = lma->brks[LMA_LOW] = (lma_addr_t) base;
	lma->end = lma->brks[LMA_HIGH] = (lma_addr_t) base + size;
#ifdef LMA_DEBUG
	lma->debug = 0;
#endif
}

_lma_alwaysinline void lma_reset_low(struct lma *lma) { lma->brks[LMA_LOW] = lma->base; }
_lma_alwaysinline void lma_reset_high(struct lma *lma) { lma->brks[LMA_HIGH] = lma->end; }

_lma_alwaysinline size_t lma_avail(const struct lma *lma) { return lma->brks[LMA_HIGH] - lma->brks[LMA_LOW]; }
_lma_alwaysinline size_t lma_inuse_low(const struct lma *lma) { return lma->brks[LMA_LOW] - lma->base; }
_lma_alwaysinline size_t lma_inuse_high(const struct lma *lma) { return lma->end - lma->brks[LMA_HIGH]; }

_lma_malloc _lma_alwaysinline
void *lma_alloc_low_aligned(struct lma *lma, size_t size, size_t align) {
	lma_addr_t low = lma->brks[LMA_LOW], nxt = lma->brks[LMA_LOW] + ((size + (align - 1)) & ~(align - 1));
	return (lma->brks[LMA_HIGH] >= nxt) ? lma->brks[LMA_LOW] = nxt, low : NULL;
}

_lma_malloc _lma_alwaysinline
void *lma_alloc_low(struct lma *lma, size_t size) {
	return lma_alloc_low_aligned(lma, size, 16);
}

_lma_malloc _lma_alwaysinline
void *lma_alloc_high_aligned(struct lma *lma, size_t size, size_t align) {
	lma_addr_t high = lma->brks[LMA_HIGH] - ((size + (align - 1)) & ~(align - 1));
	return (lma->brks[LMA_LOW] <= high) ? lma->brks[LMA_HIGH] = high : NULL;
}

_lma_malloc _lma_alwaysinline
void *lma_alloc_high(struct lma *lma, size_t size) {
	return lma_alloc_high_aligned(lma, size, 16);
}

_lma_unused _lma_format(3, 4)
static int lma_asprintf_low(struct lma *lma, char **ret, const char *fmt, ...) {
	lma_addr_t low = lma->brks[LMA_LOW];
	size_t size = lma_avail(lma);
	va_list va;
	va_start(va, fmt);
	int n = vsnprintf(low, size, fmt, va);
	va_end(va);
	if (n >= 0 && (size_t) n < size) {
		lma->brks[LMA_LOW]= low + ((n + 16) & ~15);
		*ret = low;
	} else
		*ret = NULL;
	return n;
}

_lma_unused _lma_format(3, 4)
static int lma_asprintf_low_aligned(struct lma *lma, char **ret, size_t align, const char *fmt, ...) {
	lma_addr_t low = lma->brks[LMA_LOW];
	size_t size = lma_avail(lma);
	va_list va;
	va_start(va, fmt);
	int n = vsnprintf(low, size, fmt, va);
	va_end(va);
	if (n >= 0 && (size_t) n < size) {
		lma->brks[LMA_LOW]= low + ((n + align) & ~(align - 1));
		*ret = low;
	} else
		*ret = NULL;
	return n;
}

/*
   Utilities for scoping temporary allocations, allowing ping-ponging of
   temporary allocations between the high and low areas for nested call sites.

	void foo(struct lma_scope *ls) {
		const struct lma_scope ll = lma_push(ls);
		void *tmp = lma_alloc(ll, ...);
		...
		lma_pop(&ll);
	}
 */

struct lma_scope {
	struct lma *lma;
	lma_addr_t end;
	int area;
};

_lma_alwaysinline struct lma_scope lma_scope(struct lma *lma, int area) {
	struct lma_scope scope;
	scope.lma = lma;
	scope.end = lma->brks[area];
	scope.area = area;
	return scope;
}
_lma_alwaysinline struct lma_scope lma_push(const struct lma_scope *ls) {
	return lma_scope(ls->lma, ls->area ^ LMA_HIGH);
}
_lma_alwaysinline void lma_pop(const struct lma_scope *ls) {
	ls->lma->brks[ls->area] = ls->end;
}
_lma_alwaysinline size_t lma_inuse(const struct lma_scope *ls) {
	return (ls->area == LMA_LOW ? lma_inuse_low(ls->lma) : lma_inuse_high(ls->lma));
}
_lma_malloc _lma_alwaysinline void *lma_alloc(const struct lma_scope *ls, size_t size) {
	return (ls->area == LMA_LOW ? lma_alloc_low(ls->lma, size) : lma_alloc_high(ls->lma, size));
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
	(lma_reset_low(lma), (void) _lma_debug(NULL, lma, "reset", LMA_LOW, 0, __FILE__, __LINE__))
# define lma_reset_high(lma) \
	(lma_reset_high(lma), (void) _lma_debug(NULL, lma, "reset", LMA_HIGH, 0, __FILE__, __LINE__))
# define lma_alloc_high(lma,size) \
	_lma_debug(lma_alloc_high(lma, size), lma, "alloc", LMA_HIGH, size, __FILE__, __LINE__)
# define lma_alloc_low(lma,size) \
	_lma_debug(lma_alloc_low(lma, size), lma, "alloc", LMA_LOW, size, __FILE__, __LINE__)
# define lma_alloc(lma,area,size) \
	_lma_debug(lma_alloc(lma, area, size), lma, "alloc", area, size, __FILE__, __LINE__)
# define lma_asprintf_low(lma,ret,fmt,...) \
	_lma_debug_asprintf( \
		lma_asprintf_low(lma, ret, fmt, __VA_ARGS__), *(ret), lma, "alloc", LMA_LOW, \
		(*(ret) ? strlen(*(ret)) + 1 : 0), __FILE__, __LINE__)
_lma_alwaysinline void *_lma_debug(
		void *p, struct lma *lma, const char *what, int area, size_t size,
		const char *file, int line) {
	if (lma->debug)
		_lma_debugf(
# if defined(_MSC_VER) || defined(__MINGW32__)
			"lma: %s:%d: %08p-%08p:%06Ix %06Ix-%06Ix %04.01f%% %s %-4s %08p:%06Ix\n",
# else
			"lma: %s:%d: %08p-%08p:%06zx %06zx-%06zx %04.01f%% %s %-4s %08p:%06zx\n",
# endif
			file, line, lma->base, lma->end, lma->end - lma->base,
			lma_inuse_low(lma), lma_inuse_high(lma),
			(double) ((lma->end - lma->base) - lma_avail(lma)) / (lma->end - lma->base) * 100.0,
			what, area ? "high" : "low", p, size);
	return p;
}
_lma_alwaysinline int _lma_debug_asprintf(
		int err, void *p, struct lma *lma, const char *what, int area, size_t size,
		const char *file, int line) {
	_lma_debug(p, lma, what, area, size, file, line);
	return err;
}
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AW_LMA_H */

