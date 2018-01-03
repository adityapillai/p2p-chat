#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include "utils.h"

void read_string_socket(int fd, char* buff, size_t maxBytes){
  size_t bytesRead = 0;
  char c = 1;
  while(bytesRead < maxBytes && c){
    // assuming read always works for now
    read(fd, &c, 1);
    buff[bytesRead++] = c;
  }
}

int start_tcp_client(char* ip, char* port){
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  struct addrinfo* res;
  int e;
  if((e = getaddrinfo(ip, port, &hints, &res))){
    fprintf(stderr, "%s\n", gai_strerror(e));
    exit(1);
  }
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if(connect(sockfd, res->ai_addr, res->ai_addrlen)){
    perror(NULL);
    exit(1);
  }
  freeaddrinfo(res);
  return sockfd;
}


int start_tcp_server(char* port, int backlog){
  int serverFd = socket(AF_INET, SOCK_STREAM, 0);
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_socktype = SOCK_STREAM;
  struct addrinfo* result;
  int e = getaddrinfo(NULL, port, &hints, &result);
  if(e == -1){
    fprintf(stderr, "%s\n", gai_strerror(e));
    exit(1);
  }

  int opt = 1;
  if(setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))){
    perror(NULL);
    exit(1);
  }

  if(bind(serverFd, result->ai_addr, result->ai_addrlen) != 0){
    perror(NULL);
    exit(1);
  }
  freeaddrinfo(result);

  if(listen(serverFd, backlog)){
    perror(NULL);
    exit(1);
  }
  return serverFd;
}
