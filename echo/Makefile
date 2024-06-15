MODE = debug

include config.mk
export CFLAGS LDFLAGS

.PHONY: all
all: server client

server: server.o assist.o
	gcc $(LDFLAGS) -o $@ $^

client: client.o assist.o
	gcc $(LDFLAGS) -o $@ $^

%.o: %.c
	gcc $(CFLAGS) -c $<

.PHONY: json
json: clean
	bear -- make

.PHONY: clean
clean:
	-rm -f *.o
