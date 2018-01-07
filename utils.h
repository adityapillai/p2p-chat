#include <stddef.h>
#include <unistd.h>

#pragma once

typedef struct user{
  char* IP_ADDRESS;
  char* username;
  char* port; // port that user is listening on
} user;


/**
  *Reads bytes from socket until null byte is read '\0' or when maxium number
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
