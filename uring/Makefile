MODE = debug

include config.mk
export CFLAGS LDFLAGS

LIBS     = -luring
LDFLAGS += $(LIBS)

files   = $(wildcard *.c)
targets = $(basename $(files))
objects = $(addsuffix .o,$(targets))

.PHONY: all
all: $(targets)

$(targets): %: %.o
	gcc $(LDFLAGS) -o $@ $^

$(objects): %.o: %.c
	gcc $(CFLAGS) -c $<

.PHONY: json
json: clean
	bear -- make

.PHONY: clean
clean:
	-rm -f *.o
