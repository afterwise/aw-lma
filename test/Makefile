
export CFLAGS += -Wall -Wextra

test: test.o ../libaw-lma.a
	clang -o $@ $^

%.o: %.c
	clang $(CFLAGS) -I.. -c $< -o $@

../libaw-lma.a:
	$(MAKE) -C..

.PHONY: clean
clean:
	rm -f test test.o

