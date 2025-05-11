#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT "3490"
#define MAX_BUFFERLEN 1024


void *get_in_addr(struct sockaddr *sa){
  if (sa->sa_family==AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc , char* args[]){

  char buf[MAX_BUFFERLEN];
  struct addrinfo hints, *servinfo, *p;
  int sockfd, ret,yes =1;
  char s[INET6_ADDRSTRLEN];
  int buflen;

  
  if (argc!=2) {
    fprintf(stderr, "usage: client hostname \n");
    exit(1);
  }

  
  //Fill the presets for connection
  memset(&hints, 0, sizeof(hints));
  hints.ai_family=AF_UNSPEC;
  hints.ai_socktype=SOCK_STREAM;


  //Create struct addrinfo
  if ((ret==getaddrinfo(args[1], PORT, &hints, &servinfo))!=0) {
    fprintf(stderr, "error getaddrinfo %s\n", gai_strerror(ret));
  }

  for (p=servinfo; p!=NULL; p=p->ai_next) {
      //Create a socket descriptor out of addrinfo
      if ((sockfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))) {
          perror("client: socket\n");
      }

      //Connect to remote host
      if (connect(sockfd, p->ai_addr, p->ai_addrlen)==-1) {
          perror("client: connect\n");
      }

      break;
     }

     //All done with this
     freeaddrinfo(servinfo);

     if (p==NULL) {
       perror("client failed to connect \n");
       exit(1);
     }

    //print the address of host
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof(s));
    printf("connecting to %s", s);

    //recieve bytes
    if ((buflen=recv(sockfd, buf, MAX_BUFFERLEN-1, 0))==-1) {
        perror("client: recv\n");
        exit(1);
    }

    //safe termination of string
    buf[buflen]= '\0';

    printf("client: recived message %s\n", buf);

    close(sockfd); 
    
    
    return 0;
}
