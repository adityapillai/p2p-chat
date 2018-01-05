all: client server
client: client.c utils.c
	gcc -pthread -g client.c utils.c `pkg-config --cflags --libs glib-2.0` -o client
server: server.c utils.c
	gcc -g server.c utils.c -o server
