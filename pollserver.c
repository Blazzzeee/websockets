#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>

#define PORT "5500"
#define BACKLOG 10

#define MAX_PARR_SIZE 1024
#define INIT_ARR_SIZE 10

struct _pfds {
  int count;
  int size;
  struct pollfd *pfds[];
};

typedef struct _pfds pollArray ;

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int get_listener_socket(void) {
  int listener, rv;
  struct addrinfo hints, *servinfo, *p;
  int yes = 1;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) == -1) {
    fprintf(stderr, "sever: getaddrinfo %s", gai_strerror(rv));
    exit(1);
  }

  for (p = servinfo; p != NULL; p = p->ai_next) {
    // Create a socket descriptor out of the first one we can
    if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
        -1) {
      perror("server: socket\n");
      continue;
      exit(1);
    }
    // Tell the socket to use port address
    setsockopt(listener, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes));

    // bind the socket to system port
    if ((rv = bind(listener, p->ai_addr, p->ai_addrlen)) == -1) {
      perror("server: bind\n");
      continue;
    }
    break;
  }

  if (p == NULL) {
    fprintf(stderr, "server: could not create a socket \n");
    exit(1);
  }

  // All done with this
  freeaddrinfo(servinfo);

  // start listening for incoming connections
  listen(listener, BACKLOG);
  printf("server: created socket\n");
  // Return the socket descriptort
  return listener;
}

int add_to_pfds(pollArray* _pollArray, int newfd){
  //Handle Overflow
  int *count = &_pollArray->count;
  int *size = &_pollArray->size;
  if (*count==*size) {
      printf("sever: pollArray overflow");
      int newSize = (*size)*2;
      if (newSize<=MAX_PARR_SIZE) {
          (*size)=newSize;
          void* temp = realloc(_pollArray, (sizeof(pollArray *) * (*size)));
      }
      else {
          fprintf(stderr, "server: critical pollArray cannot handle overflow");
          return 1;
      }
  }
  _pollArray->pfds[* count]->fd=newfd;
  _pollArray->pfds[* count]->events=POLLIN; //Register connect , and ready_to_send 
  _pollArray->pfds[* count]->revents=0; //revent is used to check if the event has occurred
  *(count)++;
  fprintf(stdout, "server: added sockfd to poll array");
  return 0;

}

void del_from_pfds(pollArray *_pollArray , int i){
  //copy the one from last here 
  _pollArray->pfds[i]=_pollArray->pfds[_pollArray->count-1];
  //decrement count to overwrite the one we copied on the next add_to 
  _pollArray->count--;
}


int main(int argc, char *argsv[]) {

  // get_listener_socket
  int listener = get_listener_socket();
  // add_pfds
  // del_pfds

  // get_listener_socket
  //  add listener to pfds
  //  start event loop
  // check if listener got a new connection
  //  check if client sent a message
  //  broadcast message
  // end event loop
  return 0;
}
