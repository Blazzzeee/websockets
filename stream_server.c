#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


#define MY_PORT "3490"
#define BACKLOG 10

int main(int argc, char* argv[]){
  struct addrinfo hints, *servinfo, *p;
  int sockfd, newfd;
  int ret;
  int yes=1;

  //The hints is used to specify presets for the socket
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM; 
  hints.ai_flags = AI_PASSIVE;

   //Fill the socket information
  if ((ret=getaddrinfo(NULL, MY_PORT, &hints, &servinfo)!=0)){
    fprintf(stderr, "Error in getaddrinfo: %s", gai_strerror(ret));
    return 1;
    }

    //Loop through the linked list 
    for (p=servinfo; p!=NULL; p=p->ai_next) {

      //Create a socket descriptor 
        if ((sockfd=socket(p->ai_family,p->ai_family , p->ai_protocol))==-1) {
          perror("server: socket");
          continue;
          
        }

        //Tell system to reuse socket if its in wait state       
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
            sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        //Bind the part to machine address 
        if (bind(sockfd, p->ai_addr, p->ai_addrlen)==-1){
          close(sockfd);
          perror("server: bind");          
          continue;
        }

        break;
      }

      freeaddrinfo(servinfo);

      // Start listening for new connections
      if (listen(sockfd, BACKLOG)==-1) {
        perror("failed to start listening for new connections\n");
        exit(1);
      }

      printf("server: waiting for connections....\n");

      //Accept connections
      while (1) {
      
      }
      

}  

