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
  int serverFd = socket(AF_INET, SOCK_STREAM, 0);
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_flags = AI_PASSIVE;
  struct addrinfo* result;
  int e = getaddrinfo(NULL, argv[1], &hints, &result);
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

  while(running){
    struct sockaddr_in* addr;
    socklen_t addrlen = sizeof(addr);
    int clientFd = accept(serverFd, (struct sockaddr*)&addr, &addrlen);

    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr, str, INET_ADDRSTRLEN);
    user* newUser = calloc(sizeof(user));
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
