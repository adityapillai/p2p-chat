#include <stdlib.h>
#include <unistd.h>
#include "utils.h"

void read_string_socket(int fd, char* buff, size_t maxBytes){
  size_t bytesRead = 0;
  char c = 1;
  while(bytesRead < maxBytes && c){
    // assuming read always works for now
    read(fd, &c, 1);
    buff[bytesRead++] = c;
  }

}
