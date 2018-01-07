#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include "utils.h"

int read_string_socket(int fd, char* buff, size_t maxBytes){
  size_t bytesRead = 0;
  char c = 1;
  while(bytesRead < maxBytes && c){

    ssize_t res = read(fd, &c, 1);
    if(res == -1){
      if(errno == EINTR){
        continue;
      }
      return 0;
    } else if(!res){
      break;
    }
    buff[bytesRead++] = c;
  }
  return c == 0 ? bytesRead : 0;
}

void destroyUser(user* user){
  if(!user){
    return;
  }
  free(user->IP_ADDRESS);
  free(user->port);
  free(user->username);
  free(user);
}


int write_all_socket(int fd, char* buff, size_t count){
  size_t bytesWritten = 0;
  while(bytesWritten < count){
    ssize_t res = write(fd, buff + bytesWritten, count - bytesWritten);
    if(res == -1){
      if(errno == EINTR){
        continue;
      }
      return 0;
    } else if(!res){
      break;
    }
    bytesWritten += res;
  }
  return bytesWritten == count ? bytesWritten : 0;
}

int send_user_network(int fd, user* user){
  return
  write_all_socket(fd, "U\n", 3) &&
  write_all_socket(fd, user->username, strlen(user->username) + 1) &&
  write_all_socket(fd, user->IP_ADDRESS, strlen(user->IP_ADDRESS) + 1) &&
  write_all_socket(fd, user->port, strlen(user->port) + 1) &&
  write_all_socket(fd, "\n", 2);
}

int receive_user_network(int fd, user* user){
  char mode[3];
  if(!read_string_socket(fd, mode, sizeof(mode)) || strcmp("U\n", mode)){
    return 0;
  }
  char username[51];
  char IP_ADDRESS[INET_ADDRSTRLEN];
  char port[8];
  char end[2];
  if(!read_string_socket(fd, username, sizeof(username))  ||
     !read_string_socket(fd, IP_ADDRESS, INET_ADDRSTRLEN) ||
     !read_string_socket(fd, port, sizeof(port))          ||
     !read_string_socket(fd, end, sizeof(end))){
       return 0;
  }
  if(strcmp(end, "\n")){
    return 0;
  }

  user->username = strdup(username);
  user->IP_ADDRESS = strdup(IP_ADDRESS);
  user->port = strdup(port);
  return 1;
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
