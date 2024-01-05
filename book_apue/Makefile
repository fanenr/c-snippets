cc     := gcc
dbg    := -ggdb
std    := -std=gnu17
cflags := $(std) $(dbg)

targets := ls 1-4 1-5 sh hole fcntl 4-3 abort
objects := $(addsuffix .o, $(targets) apue)

.PHONY: all
all: $(targets)

$(targets): %: %.o apue.o
	$(cc) $(std) -o $@ $< apue.o

$(objects): %.o: %.c apue.h
	$(cc) $(cflags) -c -o $@ $<

.PHONY: json
json: clean
	bear --output compile_commands.json -- make

.PHONY: clean
clean:
	-rm $(targets) $(objects)