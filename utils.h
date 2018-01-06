#include <stddef.h>
#include <unistd.h>


/**
  *Reads bytes from socket until null byte is read '\0' or when maxium number
  * of bytes specified by parameter is reached
  * Parameters
  *   int: file descriptor that is read from
  *   char*: buffer that data is read into
  *   size_t: maximum number of bytes read
  * Return: number of bytes read or -1, if failed
  */
ssize_t read_string_socket(int , void* , size_t);

ssize_t write_all_socket(int, void*, size_t);


/**
  * Creates a socket and starts tcp server, sets SO_REUSEADDR when binding to port
  * Parameters
  *   char*: port that tcp connections listens on
  *   int: backlog, max number of connections that can be queued
  * Return: file descriptor for tcp server
  */
int start_tcp_server(char*, int);

int start_tcp_client(char*, char*);
