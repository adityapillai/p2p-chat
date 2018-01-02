#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include <sys/socket.h>
#include "utils.h"


char* serverAddress;
int running = 1;
int sendFd;

void* chat_listener(void* args){
  int serverFd = start_tcp_server(args, 10);
  struct sockaddr_in* addr;
  socklen_t addrlen = sizeof(addr);
  int clientFd = accept(serverFd, (struct sockaddr*)&addr, &addrlen);
  //TODO verify connector with ip address, changing listener fd

  while(running){
    char buff[1024];
    read_string_socket(clientFd, buff, 1024);
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

  int sockfd = start_tcp_client(argv[2], argv[3]);


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
  sendFd = start_tcp_client(buff + 2, buff + strlen(buff) + 1);


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
