#include "utils.h"

typedef struct user_node{
  struct user* user;
  struct user_node* next;
  int fd;
  char new_arrival;
} user_node;
