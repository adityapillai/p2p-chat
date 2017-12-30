typedef struct user{
  char* IP_ADDRESS;
  char* username;
  struct user* next;
  int fd;
} user;
