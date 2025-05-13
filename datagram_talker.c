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

int main(int argc, char *argv[]){
  if (argc!=3) {
    fprintf(stderr, "usage: talker hotsname message");
    exit(1);
  }

  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  int numbytes;

  memset(&hints, 0, sizeof hints);
  hints.ai_socktype=SOCK_DGRAM;
  hints.ai_family=AF_INET6;

  if ((rv=getaddrinfo(argv[1], PORT, &hints, &servinfo))==-1) {
    fprintf(stderr, "talker: getaddrinfo %s\n", gai_strerror(rv));
    exit(1);
  }


  for (p=servinfo; p!=NULL; p=p->ai_next) {
      if ((sockfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1) {
        perror("talker: socket \n");
        continue;
      }

      break;
            
  }

  if (p==NULL) {
    fprintf(stderr, "talker failed to create socket");
    close(sockfd);
    exit(2);
  }

  if ((numbytes=sendto(sockfd, argv[2], strlen(argv[2]),0, p->ai_addr, p->ai_addrlen ))==-1) {
      perror("listener: sendto \n");
      exit(3);
  }

  printf("listener sent %d bytes to %s", numbytes, argv[1]);
  close(sockfd);
  freeaddrinfo(servinfo);
  return 0;
  
  
}
