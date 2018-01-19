#include <stddef.h>
#include <unistd.h>

#pragma once

typedef struct user{
  char* IP_ADDRESS;
  char* username;
  char* port; // port that user is listening on
} user;

// Following constants are messages passed between server anc client
#define CONNECT "C" // server sends to client when client needs to connect to new user
#define ACCEPT "A" // server sends to client when client needs to accept new connection
#define FAILIURE "F" // client sends to server when connetion client reads messages from fails
#define READY "R" // client sends to server when ready to accept new connection


#define MAX_USERNAME_LENGTH 50

/**
  * Reads a string from socket, and compares it to the expected message
  * Parameters
  *  int: file descriptor for socket
  *  char*: messsage that is expected to be read
  * Return: 0 if succesfull, -1 if read failed, -2 if message was different
  */
int expect_string_socket(int, char*);

/**
  * Reads bytes from socket until null byte is read '\0' or when maxium number
  * of bytes specified by parameter is reached
  * Parameters
  *   int: file descriptor that is read from
  *   char*: buffer that data is read into
  *   size_t: maximum number of bytes read
  * Return: number of bytes read or 0 if failed
  */
int read_string_socket(int , char* , size_t);

int write_all_socket(int, char*, size_t);

int send_user_network(int, user* user);

int receive_user_network(int, user* user);

void destroyUser(user*);

/**
  * Creates a socket and starts tcp server, sets SO_REUSEADDR when binding to port
  * Parameters
  *   char*: port that tcp connections listens on
  *   int: backlog, max number of connections that can be queued
  * Return: file descriptor for tcp server
  */
int start_tcp_server(char*, int);

int start_tcp_client(char*, char*);
