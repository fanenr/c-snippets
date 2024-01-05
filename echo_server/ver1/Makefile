all: server client
	

client: client.c assist.c assist.h
	gcc -std=gnu17 -g -o client client.c assist.c

server: server.c assist.c assist.h
	gcc -std=gnu17 -g -o server server.c assist.c