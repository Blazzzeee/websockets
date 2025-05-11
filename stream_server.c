#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define MY_PORT "3490"
#define BACKLOG 10



void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
int main(int argc, char* argv[]){
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr;
  socklen_t sin_size;
  int sockfd, newfd;
  int ret;
  int yes=1;
  char s[INET6_ADDRSTRLEN];

  //The hints is used to specify presets for the socket
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM; 
  hints.ai_flags = AI_PASSIVE;

   //Fill the socket information
  if ((ret=getaddrinfo(NULL, MY_PORT, &hints, &servinfo)!=0)){
    fprintf(stderr, "Error in getaddrinfo: %s\n", gai_strerror(ret));
    return 1;
    }

    //Loop through the linked list 
    for (p=servinfo; p!=NULL; p=p->ai_next) {

      //Create a socket descriptor 
        if ((sockfd=socket(p->ai_family,p->ai_socktype, p->ai_protocol))==-1) {
          perror("server: socket\n");
          continue;
          
        }

        //Tell system to reuse socket if its in wait state       
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
            sizeof(int)) == -1) {
            perror("setsockopt\n");
            exit(1);
        }

        //Bind the part to machine address 
        if (bind(sockfd, p->ai_addr, p->ai_addrlen)==-1){
          close(sockfd);
          perror("server: bind\n");          
          continue;
        }

        break;
      }

      freeaddrinfo(servinfo);

      if (p ==NULL){
        fprintf(stderr, "server failed to bind\n");
        exit(1);
      }
      
      // Start listening for new connections
      if (listen(sockfd, BACKLOG)==-1) {
        perror("failed to start listening for new connections\n");
        exit(1);
      }

      printf("server: waiting for connections....\n");

      //Accept connections
      while (1) {
        sin_size = sizeof(their_addr);
        newfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size); 

        if (newfd ==-1) {
          perror("accept");
          continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);        
            printf("server got connection from %s\n", s);


        if (!fork()){
          //The child does not need to listen
          close(sockfd);

          //send static buffer
          if (send(newfd, "Hello, world!", 13, 0) ==-1) {
            perror("send");
          }

          //close the socket
          close(newfd);
          exit(0);
                    
        }

        close(newfd);
      }
      
      return 0;

}  

