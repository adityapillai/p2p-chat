#include <stddef.h>
#include <unistd.h>

ssize_t read_string_socket(int , char* , size_t);

int start_tcp_server(char*, int);

int start_tcp_client(char*, char*);
