ALL = client server
HEADERS = tcp.h
FLAGS = -g -lpthread -lrt

all: $(ALL)

client: client.c tcp.c $(HEADERS)
	gcc -o client client.c tcp.c $(FLAGS)

server: server.c tcp.c $(HEADERS)
	gcc -o server server.c tcp.c $(FLAGS)

clean:
	rm client server
