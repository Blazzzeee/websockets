#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define PORT "5500"
#define BACKLOG 10

void *get_in_addr(struct sockaddr *sa){
  if (sa->sa_family==AF_INET) {
      return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}


int get_listener_socket(void){
  int listener, rv;
  struct addrinfo hints, *servinfo , *p;
  int yes=1;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family=AF_UNSPEC;
  hints.ai_socktype=SOCK_STREAM;
  hints.ai_flags=AI_PASSIVE;

  if ((rv=getaddrinfo(NULL, PORT, &hints, &servinfo))==-1) {
      fprintf(stderr, "sever: getaddrinfo %s", gai_strerror(rv));
      exit(1);
  }


  for (p=servinfo; p!=NULL; p=p->ai_next) {
      //Create a socket descriptor out of the first one we can
      if ((listener=socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1) {
          perror("server: socket\n");
          continue;
          exit(1);
      }
      //Tell the socket to use port address
      setsockopt(listener, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes));

      //bind the socket to system port
      if ((rv=bind(listener, p->ai_addr, p->ai_addrlen))==-1) {
          perror("server: bind\n");
          continue;
      }

      break;
  }

  if (p==NULL) {
      fprintf(stderr, "server: could not create a socket \n");
      exit(1);
  }

  //All done with this
  freeaddrinfo(servinfo);

  //start listening for incoming connections
  listen(listener, BACKLOG);  

  //Return the socket descriptor
  return listener;

}

int main(int argc, char* argsv[]){

  //get_listener_socket
  // add_pfds
  // del_pfds


  //get_listener_socket
  // add listener to pfds
  // start event loop
      //check if listener got a new connection
      // check if client sent a message
      // broadcast message
  //end event loop
  return 0;
}

