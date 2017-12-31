typedef struct user{
  char* IP_ADDRESS;
  char* username;
  char* port;
  struct user* next;
  int fd;
} user;
