# c compiler: like gcc clang ...
cc = gcc
# debug mode: -g or empty
dbg = -g
# c standard: like c17 gnu17 ...
std = gnu17
# build dir:  like . or ./build
dir = ./build
# bin dir:    like . or ./bin or ./build
bin = ./bin
# compile options
cflags = -std=$(std) $(dbg)

#############################################################
targets = server client

.PHONY: all
all: $(addprefix $(bin)/, $(targets))

# executable target: client
src_client = $(dir)/client.o $(dir)/assist.o
$(bin)/client: $(src_client)
	$(cc) $(dbg) -o $(bin)/client $(dir)/client.o $(dir)/assist.o

# executable target: server
src_server = $(dir)/server.o $(dir)/assist.o
$(bin)/server: $(src_server)
	$(cc) $(dbg) -o $(bin)/server $(dir)/server.o $(dir)/assist.o

# assist files
assist = assist.c assist.h
objects = client.o server.o assist.o

# all objects depend on assist
$(addprefix $(dir)/, $(objects)): $(assist)

# object targets
$(dir)/client.o: client.c
	$(cc) $(cflags) -c -o $(dir)/client.o client.c

$(dir)/server.o: server.c
	$(cc) $(cflags) -c -o $(dir)/server.o server.c

$(dir)/assist.o: assist.c
	$(cc) $(cflags) -c -o $(dir)/assist.o assist.c

.PHONY: clean
clean:
	-rm $(addprefix $(dir)/, $(objects)) \
		$(addprefix $(bin)/, $(targets))