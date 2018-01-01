#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <assert.h>
#include <sys/socket.h>
#include "utils.h"


char* serverAddress;
int running = 1;
int sendFd;

void* chat_listener(void* args){
  char* port = (char*)args;
  int serverFd = socket(AF_INET, SOCK_STREAM, 0);
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_flags = AI_PASSIVE;
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
  if(listen(serverFd, 10)){
    perror(NULL);
    exit(1);
  }
  struct sockaddr_in* addr;
  socklen_t addrlen = sizeof(addr);
  int clientFd = accept(serverFd, (struct sockaddr*)&addr, &addrlen);
  //TODO verfify connector with ip address

  while(running){
    char buff[1024];
    read_string_socket(clientFd, buff, 1024);
    // put writing fd here, for sender
    write(sendFd, buff, strlen(buff) + 1);
    printf("%s\n",buff);
  }

  return NULL;
}

// argv 1 username
// argv 2 server ip
// argv 3 server port
// argv 4 listening port
int main(int argc, char** argv){
  if(argc < 5){
    return 1;
  }
  serverAddress = argv[2];

  pthread_t listener;
  pthread_create(&listener, NULL, chat_listener, argv[4]);
  pthread_detach(listener);


  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  struct addrinfo* res;
  int e;
  if((e = getaddrinfo(argv[2], argv[3], &hints, &res))){
    fprintf(stderr, "%s\n", gai_strerror(e));
    exit(1);
  }
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if(connect(sockfd, res->ai_addr, res->ai_addrlen)){
    perror(NULL);
    exit(1);
  }
  freeaddrinfo(res);

  write(sockfd, "P ", 2);
  write(sockfd, argv[4], strlen(argv[4]) + 1);
  // read ip address of who to connect to
  char buff[1024];
  read_string_socket(sockfd, buff, 1024);
  assert(buff[0] == 'C');
  assert(buff[1] == ' ');
  // read port to connect on
  read_string_socket(sockfd, buff + strlen(buff) + 1, 1024 - strlen(buff) - 1);
  // connect to sender
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if((e = getaddrinfo(buff + 2, buff + strlen(buff) + 1, &hints, &res))){
    fprintf(stderr, "%s\n", gai_strerror(e));
    exit(1);
  }
  sendFd = socket(AF_INET, SOCK_STREAM, 0);
  connect(sendFd, res->ai_addr, res->ai_addrlen);
  freeaddrinfo(res);


  char* line = NULL;
  size_t n = 0;
  ssize_t bytesRead;
  while((bytesRead = getline(&line, &n, stdin)) != -1){
    if(line[bytesRead - 1] == '\n'){
      line[bytesRead - 1] = 0;
    }
    char message[strlen(argv[1]) + strlen(line) + 3];
    sprintf(message, "%s: %s", argv[1], line);
    write(sendFd, message, strlen(message) + 1);
    printf("%s\n",message);
    // write message to sender
  }
  free(line);











  return 0;
}
