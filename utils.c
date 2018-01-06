#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include "utils.h"

ssize_t read_string_socket(int fd, char* buff, size_t maxBytes){
  size_t bytesRead = 0;
  char c = 1;
  while(bytesRead < maxBytes && c){
    // assuming read always works for now
    ssize_t res = read(fd, &c, 1);
    if(res == -1){
      if(errno == EINTR){
        continue;
      }
      return -1;
    } else if(!res){
      break;
    }
    buff[bytesRead++] = c;
  }
  return bytesRead;
}

ssize_t write_all_socket(int fd, char* buff, size_t count){
  size_t bytesWritten = 0;
  while(bytesWritten < count){
    ssize_t res = write(fd, buff + bytesWritten, count - bytesWritten);
    if(res == -1){
      if(errno == EINTR){
        continue;
      }
      return -1;
    } else if(!res){
      break;
    }
    bytesWritten += res;
  }
  return bytesWritten;
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
  if(sockfd == -1){
    perror(NULL);
    exit(1);
  }

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
