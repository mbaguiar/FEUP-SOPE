all: client server
	
client: client.c
	gcc client.c -o client -D_REENTRANT -lpthread -Wall

server: server.c
	gcc server.c -o server -D_REENTRANT -lpthread -Wall

clean:
	rm -f client
	rm -f server