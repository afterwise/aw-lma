
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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "aw-lma.h"

void *lma_alloc(struct lma *lma, size_t size) {
	uintptr_t brk = lma->brk;
	lma->brk += (size + 15) & ~15;

	assert(lma->base + lma->size >= brk + size);
	return (void *) brk;
}

char *lma_asprintf(struct lma *lma, const char *fmt, ...) {
	va_list va;
	char *p = lma_getbrk(lma);
	size_t m = lma->size - lma_used(lma);
	int n;

	va_start(va, fmt);
	n = vsnprintf(p, m, fmt, va);

	if (n >= 0 && (size_t) n < m)
		lma->brk += (n + 16) & ~15;
	else
		p = NULL;

	va_end(va);
	return p;
}

