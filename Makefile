ALL = client server
HEADERS = tcp.h
FLAGS = -g -lpthread

all: $(ALL)

client: client.c tcp.c $(HEADERS)
	gcc -o client client.c tcp.c $(TAGS)

server: server.c tcp.c $(HEADERS)
	gcc -o server server.c tcp.c $(TAGS)

clean:
	rm client server
