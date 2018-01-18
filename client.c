#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include <glib.h>
#include "utils.h"

volatile int running = 1;
volatile int write_not_ready = 1;

pthread_barrier_t barrier;
int sendFd = -1;

char finished[1];
user* sendUser;

GAsyncQueue* queue;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

void* chat_listener(void* args){
  char* port = (char*)args;
  int serverFd = start_tcp_server(port, 10);
  int chat_server = *(int*)(port + strlen(port) + 1);
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  int clientFd = -1;
  //TODO verify connector with ip address, changing listener fd

  while(running){
    char buff[1024];
    if(!read_string_socket(clientFd, buff, sizeof(buff))){
      if(clientFd > 0){
        shutdown(clientFd, SHUT_RDWR);
        close(clientFd);
        write_all_socket(chat_server, "F", strlen("F") + 1);
      }
      //fprintf(stderr, "wrote ready message\n");
      pthread_barrier_wait(&barrier);
      write_all_socket(chat_server, "R", strlen("R") + 1);
      clientFd = accept(serverFd, (struct sockaddr*)&addr, &addrlen);
      continue;
    }
    g_async_queue_push(queue, strdup(buff));
  }

  shutdown(serverFd, SHUT_RDWR);
  close(serverFd);
  shutdown(clientFd, SHUT_RDWR);
  close(clientFd);
  return NULL;
}

void* server_listener(void* arg){
  int serverFd = *(int*)arg;
  while (running) {
    char buff[3];
    if(!read_string_socket(serverFd, buff, sizeof(buff))){
      break;
    }

    if(!strcmp(buff, "C ")){
      pthread_mutex_lock(&lock);
      write_not_ready = 1;
      destroyUser(sendUser);
      sendUser = malloc(sizeof(user));
      if(sendFd > 0){
        shutdown(sendFd, SHUT_RDWR);
        close(sendFd);
      }
      receive_user_network(serverFd, sendUser);
      sendFd = start_tcp_client(sendUser->IP_ADDRESS, sendUser->port);
      write_not_ready = 0;
      pthread_mutex_unlock(&lock);
      pthread_cond_signal(&cv);
    } else if(!strcmp(buff, "A ")){
      pthread_barrier_wait(&barrier);
    }

  }

  return NULL;
}

void* chat_writer(void* data){
  while(running){
    char* message = g_async_queue_pop(queue);
    if(message == finished){
      break;
    }

    pthread_mutex_lock(&lock);
    if(!sendUser || strstr(message, sendUser->username) != message){
      while(!write_all_socket(sendFd, message, strlen(message) + 1)){
        write_not_ready = 1;
        while(write_not_ready){
          pthread_cond_wait(&cv, &lock);
        }
      }
    }
    pthread_mutex_unlock(&lock);
    printf("%s\n",message);
  }
  return NULL;
}

void int_handler(int sig){
  running = 0;
  close(STDIN_FILENO);
}

// argv 1 username
// argv 2 server ip
// argv 3 server port
// argv 4 listening port
int main(int argc, char** argv){
  if(argc < 5){
    return 1;
  }
  queue = g_async_queue_new_full(free);
  pthread_barrier_init(&barrier, NULL, 2);
  //signal(SIGINT, int_handler);
  int sockfd = start_tcp_client(argv[2], argv[3]);

  pthread_t threads[3];
  char chat_args[strlen(argv[4]) + 1 + sizeof(int)];
  strcpy(chat_args, argv[4]);
  memcpy(chat_args + strlen(argv[4]) + 1, &sockfd, sizeof(sockfd));
  pthread_create(threads, NULL, chat_listener, chat_args);




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
    char* message = malloc(strlen(argv[1]) + strlen(line) + 3);
    sprintf(message, "%s: %s", argv[1], line);
    g_async_queue_push(queue, message);
  }
  shutdown(sockfd, SHUT_RDWR);
  close(sockfd);
  free(line);
  g_async_queue_push(queue, finished);

  size_t i;
  for(i = 0; i < sizeof(threads) / sizeof(threads[0]); ++i){
    pthread_join(threads[i], NULL);
  }

  pthread_barrier_destroy(&barrier);
  pthread_cond_destroy(&cv);
  pthread_mutex_destroy(&lock);
  g_async_queue_unref(queue);
  return 0;
}
