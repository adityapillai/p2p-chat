#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <glib.h>
#include "utils.h"


//char* serverAddress;
volatile int running = 1;
volatile int notReady = 1;
volatile int acceptNewConnection = 1;
int sendFd;
int firstRun = 1;

user* sendUser;

GAsyncQueue* queue;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

void* chat_listener(void* args){
  int serverFd = start_tcp_server(args, 10);
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  int clientFd = 0;//accept(serverFd, (struct sockaddr*)&addr, &addrlen);
  //TODO verify connector with ip address, changing listener fd


  while(running){
    char buff[1024];
    pthread_mutex_lock(&lock);
    if(acceptNewConnection){
      pthread_mutex_unlock(&lock);
      clientFd = accept(serverFd, (struct sockaddr*)&addr, &addrlen);
      acceptNewConnection = 0;
    } else{
      pthread_mutex_unlock(&lock);
    }

    read_string_socket(clientFd, buff, 1024);
    if(strlen(buff) && strstr(buff, sendUser->username) != buff)
      g_async_queue_push(queue, strdup(buff));
  }

  return NULL;
}

void* server_listener(void* arg){
  int serverFd = *(int*)arg;
  while (running) {
    char buff[1024];
    read_string_socket(serverFd, buff, 1024);
    pthread_mutex_lock(&lock);
    if(buff[0] == 'C' && buff[1] == ' '){
      notReady = 1;
      pthread_mutex_unlock(&lock);
      //read_string_socket(serverFd, buff + strlen(buff) + 1, 1024 - strlen(buff) - 1);
      destroyUser(sendUser);
      sendUser = malloc(sizeof(user));
      receive_user_network(serverFd, &sendUser);
      if(sendFd){
        shutdown(sendFd, SHUT_RDWR);
        close(sendFd);
      }
      sendFd = start_tcp_client(sendUser->IP_ADDRESS, sendUser->port);
      notReady = 0;
      pthread_cond_signal(&cv);
    } else if(buff[0] == 'A' && buff[1] == ' '){
      if(!firstRun){
        acceptNewConnection = 1;
      } else{
        firstRun = 0;
      }
      pthread_mutex_unlock(&lock);
    }

  }

  return NULL;
}

void* chat_writer(void* data){
  while(running){
    char* message = g_async_queue_pop(queue);
    pthread_mutex_lock(&lock);
    while(notReady){
      pthread_cond_wait(&cv, &lock);
    }
    pthread_mutex_unlock(&lock);
    write_all_socket(sendFd, message, strlen(message) + 1);
    printf("%s\n",message);
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
  //serverAddress = argv[2];
  queue = g_async_queue_new_full(free);

  pthread_t threads[3];
  pthread_create(threads, NULL, chat_listener, argv[4]);
  //pthread_detach(listener);

  int sockfd = start_tcp_client(argv[2], argv[3]);


  write_all_socket(sockfd, "P ", 2);
  write_all_socket(sockfd, argv[4], strlen(argv[4]) + 1);
  write_all_socket(sockfd, "U ", 2);
  write_all_socket(sockfd, argv[1], strlen(argv[1]) + 1);
  //pthread_t serverListener;
  pthread_create(threads + 1, NULL, server_listener, &sockfd);
  pthread_create(threads + 2, NULL, chat_writer, NULL);

  char* line = NULL;
  size_t n = 0;
  ssize_t bytesRead;
  while((bytesRead = getline(&line, &n, stdin)) != -1){
    if(line[bytesRead - 1] == '\n'){
      line[bytesRead - 1] = 0;
    }
    char message[strlen(argv[1]) + strlen(line) + 3];
    sprintf(message, "%s: %s", argv[1], line);
    g_async_queue_push(queue, strdup(message));
  }
  free(line);


  return 0;
}
