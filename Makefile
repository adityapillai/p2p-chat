all: client server

client: client.c utils.c
	gcc -pthread client.c utils.c `pkg-config --cflags --libs glib-2.0` -o client

server: server.c utils.c
	gcc server.c utils.c -o server

debug: clean
	gcc -pthread -g client.c utils.c `pkg-config --cflags --libs glib-2.0` -o client-debug
	gcc -g server.c utils.c -o server-debug

clean:
	rm -f client server client-debug server-debug
