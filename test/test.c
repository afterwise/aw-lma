
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "aw-lma.h"

int main(int argc, char *argv[]) {
	struct lma lma;
	void *p;

	(void) argc;
	(void) argv;

	p = malloc(1024);
	lma_init(&lma, p, 1024);
	assert(lma_used(&lma) == 0);

	p = lma_alloc(&lma, 1);
	assert(lma_used(&lma) == 16);

	lma_reset(&lma);
	assert(lma_used(&lma) == 0);

	p = lma_asprintf(&lma, "hello world #%d", 1);
	printf("<%s>\n", p);
	assert(lma_used(&lma) == 16);
}

