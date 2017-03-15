ALL = client server
HEADERS = tcp.h

all: $(ALL)

client: client.c tcp.c $(HEADERS)
	gcc -o client client.c tcp.c -g

server: server.c tcp.c $(HEADERS)
	gcc -o server server.c tcp.c -pthread -g

clean:
	rm client server
