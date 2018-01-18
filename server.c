#include "server.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <netdb.h>


volatile int running = 1;
user_node* front;
user_node* end;

user_node* find_previous(user_node* node){
  user_node* curr = front;
  do{
    if(curr->next == node){
      return curr;
    }
    curr = curr->next;
  } while(curr != front);
  return NULL;
}


//TODO notify clients of listener changes
void addNewUser(user_node* newUser){
  if(!front){
    newUser->next = newUser;
    front = newUser;
    end = newUser;
    return;
  }
  //user_node* newNode = malloc(sizeof(user_node));
  //newNode->user = newUser;
  newUser->next = end->next;
  end->next = newUser;
  front = newUser;
  newUser->new_arrival = 0;
  // TODO wait for for receving connection to confirm it is ready to accept before sending its address out

  write_all_socket(end->fd, "C ", 3);
  write_all_socket(newUser->fd, "A ", 3);

char message[2];
    if(!read_string_socket(newUser->fd, message, sizeof(message)) || strcmp("R", message)){
      return;
    }
    //newUser->new_arrival = 0;

  send_user_network(end->fd, newUser->user);



  write_all_socket(newUser->fd, "C ", 3);
  write_all_socket(newUser->next->fd, "A ", 3);

  if(!read_string_socket(newUser->next->fd, message, sizeof(message))){
    return;
  }
  if(!strcmp(message, "F")){
    if(!read_string_socket(newUser->next->fd, message, sizeof(message)) || strcmp(message, "R"))
      return;
  } else if(strcmp(message, "R")){
    return;
  }


  send_user_network(newUser->fd, newUser->next->user);

  // write to newUser who to listen to


  // write to "end" new connection
}

void removeUser(user_node* toRemove){
  // find previous of param in CLL
  user_node* prev = find_previous(toRemove);
  prev->next = toRemove->next;

  if(toRemove == front){
    front = front->next;
  } else if(toRemove == end){
    end = prev;
  }

  shutdown(toRemove->fd, SHUT_RDWR);
  close(toRemove->fd);
  destroyUser(toRemove->user);
  free(toRemove);


  if(front == end){
    return;
  }

  // TODO may need to process ready message here too
  write_all_socket(prev->fd, "C ", 3);
  write_all_socket(prev->next->fd, "A ", strlen("A ") + 1);

  char buff[2];
  if(!read_string_socket(prev->next->fd, buff, sizeof(buff)) || buff[0] != 'R'){
    return;
  }

  send_user_network(prev->fd, prev->next->user);
}

void handleNewConnection(int pollFd, int serverFd){
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  int clientFd = accept(serverFd, (struct sockaddr*)&addr, &addrlen);

  char str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(addr.sin_addr), str, INET_ADDRSTRLEN);
  user_node* newUser = malloc(sizeof(user_node));
  newUser->user = malloc(sizeof(user));
  newUser->user->IP_ADDRESS = strdup(str);
  newUser->fd = clientFd;
  newUser->new_arrival = 1;
  char buff[8];
  char username[51];
  if(!read_string_socket(clientFd, buff, sizeof(buff)) ||
     !read_string_socket(clientFd, username, sizeof(username))){
       shutdown(clientFd, SHUT_RDWR);
       close(clientFd);
       return;
  }
  if(*buff == 'P' && buff[1] == ' ' && *username == 'U' && username[1] == ' '){
    newUser->user->port = strdup(buff + 2);
    newUser->user->username = strdup(username + 2);
    addNewUser(newUser);
  } else{
    shutdown(clientFd, SHUT_RDWR);
    close(clientFd);
  }
  struct epoll_event newEvent;
  newEvent.data.ptr = newUser;
  newEvent.events = EPOLLIN | EPOLLET;

  if(epoll_ctl(pollFd, EPOLL_CTL_ADD, clientFd, &newEvent) == -1){
    exit(1);
  }
}

void handleRequest(int pollFd, struct epoll_event* event){
  user_node* curr = (user_node*)event->data.ptr;
  /*if(curr->new_arrival){
    return;
  }*/
  char buff[2];
  if(!read_string_socket(curr->fd, buff, sizeof(buff)) || strcmp("F", buff)){
    return;
  }
  //curr->new_arrival = 1;
  user_node* prev = find_previous(curr);
  epoll_ctl(pollFd, EPOLL_CTL_DEL, prev->fd, NULL);
  removeUser(prev);
}




// argv[1] = listening port,
//
int main(int argc, char** argv){
  int serverFd = start_tcp_server(argv[1], 10);

  int epollFd = epoll_create(1);
  if(epollFd == -1){
    perror(NULL);
    exit(1);
  }

  struct epoll_event event;
  event.data.fd = serverFd;
  event.events = EPOLLIN | EPOLLET;

  if(epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &event)){
    perror(NULL);
    exit(1);
  }

  while(running){
    struct epoll_event newEvent;
    if(epoll_wait(epollFd, &newEvent, 1, -1) > 0){
      if(serverFd == newEvent.data.fd){
        handleNewConnection(epollFd, serverFd);
      } else{
        handleRequest(epollFd, &newEvent);
      }

    }
  }

  return 0;
}
