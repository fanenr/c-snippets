MODE = debug

include config.mk
export CFLAGS LDFLAGS

LDFLAGS += -lcurl -ljansson

.PHONY: all
all: main

main: main.o
	gcc $(LDFLAGS) -o $@ $^

%.o: %.c
	gcc $(CFLAGS) -c $<

.PHONY: json
json: clean
	bear -- make

.PHONY: clean
clean:
	-rm -f main *.o
