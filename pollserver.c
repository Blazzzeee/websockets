#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "5500"
#define BACKLOG 10

#define MAX_PARR_SIZE 1024
#define INIT_ARR_SIZE 10

#define MAX_BUF_SIZE 256

struct _pfds {
  int count;
  int size;
  struct pollfd *pfds;
};

typedef struct _pfds pollArray;

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

  printf("server: created socket\n");
  // start listening for incoming connections
  printf("server : listening to incoming connections\n");
  listen(listener, BACKLOG);
  // Return the socket descriptort
  return listener;
}

int add_to_pfds(pollArray *_pollArray, int newfd) {
  // Handle Overflow
  int *count = &_pollArray->count;
  int *size = &_pollArray->size;
  if (*count == *size) {
    printf("sever: pollArray overflow");
    int newSize = (*size) * 2;
    if (newSize <= MAX_PARR_SIZE) {
      (*size) = newSize;
      void *temp = realloc(_pollArray, (sizeof(pollArray *) * (*size)));
    } else {
      fprintf(stderr, "server: critical pollArray cannot handle overflow");
      return 1;
    }
  }
  _pollArray->pfds[*count].fd = newfd;
  _pollArray->pfds[*count].events =
      POLLIN; // Register connect , and ready_to_send
  _pollArray->pfds[*count].revents =
      0; // revent is used to check if the event has occurred
  *(count)++;
  fprintf(stdout, "server: added sockfd to poll array");
  return 0;
}

void del_from_pfds(pollArray *_pollArray, int i) {
  // copy the one from last here
  _pollArray->pfds[i] = _pollArray->pfds[_pollArray->count - 1];
  // decrement count to overwrite the one we copied on the next add_to
  _pollArray->count--;
}

int main(int argc, char *argsv[]) {

  // get_listener_socket
  pollArray *_pollArray =
      malloc(sizeof(pollArray)); // Allocate memory for pollArray structure
  if (!_pollArray) {
    fprintf(stderr, "server: malloc failed for pollArray\n");
    exit(1);
  }

  _pollArray->pfds = malloc(sizeof(struct pollfd) *
                            INIT_ARR_SIZE); 
  if (!_pollArray->pfds) {
    fprintf(stderr, "server: malloc failed for pfds array\n");
    free(_pollArray); 
    exit(1);
  }

  _pollArray->count = 0;
  _pollArray->size = INIT_ARR_SIZE;

  int newfd;
  struct sockaddr_storage remoteaddr;
  socklen_t addrlen;

  char buf[MAX_BUF_SIZE];

  char remoteIP[INET6_ADDRSTRLEN];

  // get_listener_socket
  int listener = get_listener_socket();
  //  add listener to pfds array
  if (listener == -1) {
    fprintf(stderr, "server: could not create listener\n");
    exit(2);
  }
  add_to_pfds(_pollArray, listener);

  //  start event loop
  while (1) {
    int poll_count = poll(_pollArray->pfds, _pollArray->count, -1);

    if (poll_count == -1) {
      perror("server: poll \n");
      exit(2);
    }
    for (int i = 0; i < poll_count; i++) {
      // If some client got a event

      // Identify the client using bitwise AND for incoming connections as well
      // as data from one of the sockets
      if (_pollArray->pfds[i].revents & (POLLIN | POLLHUP)) {
        if (_pollArray->pfds[i].fd == listener) {
          // Listener got a new connection
          addrlen = sizeof remoteaddr;
          newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

          if (newfd == -1) {
            fprintf(stderr, "server: new connection invalid descriptor\n");
            exit(2);
          }

          // Otherwise we will listen from this socket
          add_to_pfds(_pollArray, newfd);

          printf("server got connection from %s on socket %d\n",
                 inet_ntop(remoteaddr.ss_family,
                           get_in_addr((struct sockaddr *)&remoteaddr),
                           remoteIP, INET6_ADDRSTRLEN),
                 newfd);
        }

        else {
          // socket pfds[i] send some message
          int nbytes = recv(_pollArray->pfds[i].fd, buf, sizeof(buf), 0);

          int senderfd = _pollArray->pfds[i].fd;

          if (nbytes == -1) {
            // TODO Handle if 0 bytes were sent
            fprintf(stderr, "server: could not write into buffer for socket %d",
                    senderfd);

            close(_pollArray->pfds[i].fd);

            del_from_pfds(_pollArray, i);
          }

          else {
            // Broadcast message to other sockets
            for (int j = 0; j < poll_count; j++) {
              int destfd = _pollArray->pfds[j].fd;
              if (destfd == senderfd || destfd == listener) {
                if (send(destfd, buf, sizeof(buf), 0) == -1) {
                  fprintf(stderr, "could not send message to %d socket \n",
                          destfd);
                } else {
                  printf("server sent message to %d socket\n", destfd);
                }
              }
            }
          }
        }
      }
    }
  }

  return 0;
}
