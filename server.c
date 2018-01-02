#include "server.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>


int running = 1;
user* front;
user* end;

//TODO notify clients of listener changes
void addNewUser(user* newUser){
  if(!front){
    newUser->next = newUser;
    front = newUser;
    end = newUser;
    return;
  }
  newUser->next = end->next;
  end->next = newUser;
  front = newUser;
  // write message to newUser about who to connect to(address + port)
  write(newUser->fd, "C ", 2);
  write(newUser->fd, newUser->next->IP_ADDRESS, strlen(newUser->next->IP_ADDRESS) + 1);
  write(newUser->fd, newUser->next->port, strlen(newUser->next->port) + 1);
  // write to newUser who to listen to

  // write to "end" new connection
  write(end->fd, "C ", 2);
  write(end->fd, newUser->IP_ADDRESS, strlen(newUser->IP_ADDRESS) + 1);
  write(end->fd, newUser->port, strlen(newUser->port) + 1);
}

void removeUser(user* toRemove){
  // find previous of param in CLL
  user* prev = NULL;
  prev->next = toRemove->next;

  if(toRemove == front){
    front = front->next;
  } else if(toRemove == end){
    end = prev;
  }

  shutdown(toRemove->fd, SHUT_RDWR);
  close(toRemove->fd);
  free(toRemove);


}




// argv[1] = listening port,
//
int main(int argc, char** argv){
  int serverFd = start_tcp_server(argv[1], 10);

  while(running){
    struct sockaddr_in* addr;
    socklen_t addrlen = sizeof(addr);
    int clientFd = accept(serverFd, (struct sockaddr*)addr, &addrlen);

    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr->sin_addr), str, INET_ADDRSTRLEN);
    user* newUser = calloc(sizeof(user), 1);
    newUser->IP_ADDRESS = strdup(str);
    newUser->fd = clientFd;
    char buff[1024];
    read_string_socket(clientFd, buff, 1024);
    if(*buff == 'P' && buff[1] == ' '){
      newUser->port = strdup(buff + 2);
    } else{
      // invalid message
    }
    addNewUser(newUser);
  }

  return 0;
}
