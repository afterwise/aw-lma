
export CFLAGS += -Wall -Wextra

test: test.o ../libaw-lma.a
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -I.. -c $< -o $@

../libaw-lma.a:
	$(MAKE) -C..

.PHONY: clean
clean:
	rm -f test test.o

