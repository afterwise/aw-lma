
#define LMA_DEBUG
#include "aw-lma.h"
#include <assert.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	struct lma lma;
	void *p;

	(void) argc;
	(void) argv;

	p = malloc(1024);
	lma_init(&lma, p, 1024);
	lma_debug(&lma, 1);

	assert(lma_inuse_low(&lma) == 0);
	assert(lma_inuse_high(&lma) == 0);

	p = lma_alloc_high(&lma, 1);
	assert(lma_inuse_low(&lma) == 0);
	assert(lma_inuse_high(&lma) == 16);

	lma_reset_high(&lma);
	assert(lma_inuse_low(&lma) == 0);
	assert(lma_inuse_high(&lma) == 0);

	p = lma_alloc_high(&lma, 256);
	assert(lma_inuse_low(&lma) == 0);
	assert(lma_inuse_high(&lma) == 256);

	p = lma_alloc_high(&lma, 256);
	assert(lma_inuse_low(&lma) == 0);
	assert(lma_inuse_high(&lma) == 512);

	lma_asprintf_low(&lma, (char **) &p, "hello world #%d", 1);
	printf("lma: <%s>\n", (char *) p);
	assert(lma_inuse_low(&lma) == 16);

	return 0;
}

