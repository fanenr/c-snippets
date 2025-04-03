MODE = debug

include config.mk
export CFLAGS LDFLAGS

LDFLAGS += -lpq

.PHONY: all
all: test

test: test.o connpool.o
	gcc $(LDFLAGS) -o $@ $^

%.o: %.c
	gcc $(CFLAGS) -c $<

.PHONY: json
json: clean
	bear -- make

.PHONY: clean
clean:
	-rm -f test *.o
