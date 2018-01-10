#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <glib.h>
#include "utils.h"


//char* serverAddress;
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
  int serverFd = start_tcp_server(args, 10);
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
      }
      pthread_barrier_wait(&barrier);
      clientFd = accept(serverFd, (struct sockaddr*)&addr, &addrlen);
      continue;
    }
    g_async_queue_push(queue, strdup(buff));
  }

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
      receive_user_network(serverFd, sendUser);
      if(sendFd > 0){
        shutdown(sendFd, SHUT_RDWR);
        close(sendFd);
      }
      sendFd = start_tcp_client(sendUser->IP_ADDRESS, sendUser->port);
      write_not_ready = 0;
      pthread_mutex_unlock(&lock);
      pthread_cond_signal(&cv);
    } else if(!strcmp(buff, "A ")){
      pthread_barrier_wait(&barrier);
    }

  }
  shutdown(serverFd, SHUT_RDWR);
  close(serverFd);
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
  pthread_barrier_init(&barrier, NULL, 2);

  pthread_t threads[3];
  pthread_create(threads, NULL, chat_listener, argv[4]);
  //pthread_detach(listener);

  int sockfd = start_tcp_client(argv[2], argv[3]); // server thread, will clean this up


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
  g_async_queue_push(queue, finished);
  // attemping to break a thrad out of barrier call if it is stuck
  if(pthread_barrier_destroy(&barrier)){
    if(errno == EBUSY){
      pthread_barrier_wait(&barrier);
    } else{
      exit(1);
    }
    pthread_barrier_destroy(&barrier);
  }
  pthread_cond_destroy(&cv);
  pthread_mutex_destroy(&lock);
  size_t i;
  for(i = 0; i < sizeof(threads) / sizeof(threads[0]); ++i){
    pthread_join(threads[i], NULL);
  }


  return 0;
}
