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

#define PORT "4950"
#define MAXBUFLEN 100


void *get_in_addr(struct sockaddr* sa){
  if (sa->sa_family==AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void){

  struct addrinfo hints, *servinfo, *p;
  int rv, sockfd;
  struct sockaddr_storage their_addr;
  socklen_t addrlen;
  char buf[MAXBUFLEN];
  char s[INET6_ADDRSTRLEN];
  
  memset(&hints, 0, sizeof(hints));
  hints.ai_family=AF_INET6;
  hints.ai_socktype=SOCK_DGRAM;
  hints.ai_family=AI_PASSIVE;

  if ((rv=getaddrinfo(NULL,PORT,&hints, &servinfo))) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
  }


  for (p=servinfo; p!=NULL; p=p->ai_next) {

    if ((sockfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1) {
        perror("listener: socket \n");
        continue;
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen)) {
      perror("listener: bind\n");
      continue;
    }

    break;
  }

  if (p==NULL) {
    fprintf(stderr, "listener: failed to bind socket");
    exit(2);
  }

  printf("listener: waiting for sender....\n");

  addrlen=sizeof(their_addr);

  if ((rv=recvfrom(sockfd, buf, MAXBUFLEN-1, 0, (struct sockaddr *)&their_addr, &addrlen))==-1) {
      perror("listener: could not write to buffer\n");
      exit(1);
  }

  printf("listener: recieved message from %s",
  inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof(s)));
  
  printf("listener packet is %d bytes long\n", rv);
  s[rv]='\0';
  printf("listener packet contains \" %s \" \n", buf);
  close(sockfd);
    
    return 0;
}
