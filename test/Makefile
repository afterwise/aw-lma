
export CFLAGS += -std=c99 -Wall -Wextra

test: test.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.x
	$(CC) $(CFLAGS) -I.. -xc -c $< -o $@

.PHONY: clean
clean:
	rm -f test test.o

