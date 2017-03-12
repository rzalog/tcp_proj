ALL = client server
HEADERS = tcp.h

all: $(ALL)

client: client.c tcp.c $(HEADERS)
	gcc -o client client.c tcp.c

server: server.c tcp.c $(HEADERS)
	gcc -o server server.c tcp.c
