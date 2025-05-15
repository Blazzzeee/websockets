#include <arpa/inet.h>
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

// Get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// Return a listening socket
int get_listener_socket(void) {
  int listener, rv;
  struct addrinfo hints, *servinfo, *p;
  int yes = 1;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;       // IPv4 only; use AF_UNSPEC + disable IPV6_V6ONLY for dual-stack
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "server: getaddrinfo: %s\n", gai_strerror(rv));
    exit(1);
  }

  for (p = servinfo; p != NULL; p = p->ai_next) {
    // Create a socket descriptor out of the first one we can
    if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    // Lose the pesky "address already in use" error message
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
      perror("server: setsockopt");
      close(listener);
      continue;
    }

    // bind the socket to system port
    if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
      perror("server: bind");
      close(listener);
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "server: could not bind to any address\n");
    exit(1);
  }

  freeaddrinfo(servinfo);

  // start listening for incoming connections
  if (listen(listener, BACKLOG) == -1) {
    perror("server: listen");
    exit(1);
  }

  printf("server: created socket and listening on port %s\n", PORT);
  return listener;
}

int add_to_pfds(pollArray *pa, int newfd) {
  // Handle Overflow
  if (pa->count == pa->size) {
    printf("server: pollArray overflow\n");
    int newSize = pa->size * 2;
    if (newSize <= MAX_PARR_SIZE) {
      pa->size = newSize;
      struct pollfd *temp = realloc(pa->pfds, sizeof(struct pollfd) * pa->size);
      if (!temp) {
        perror("server: realloc");
        exit(1);
      }
      pa->pfds = temp;
    } else {
      fprintf(stderr, "server: critical pollArray cannot handle overflow\n");
      return -1;
    }
  }

  pa->pfds[pa->count].fd = newfd;
  pa->pfds[pa->count].events = POLLIN;    // Register for incoming data
  pa->pfds[pa->count].revents = 0;        // clear revents
  pa->count++;
  fprintf(stdout, "server: added sockfd %d to poll array\n", newfd);
  return 0;
}

void del_from_pfds(pollArray *pa, int i) {
  // copy the one from last here
  close(pa->pfds[i].fd);
  pa->pfds[i] = pa->pfds[pa->count - 1];
  // decrement count to overwrite the one we copied on the next add_to
  pa->count--;
}

int main(int argc, char *argv[]) {
  // Allocate pollArray structure
  pollArray *pa = malloc(sizeof(pollArray));
  if (!pa) {
    fprintf(stderr, "server: malloc failed for pollArray\n");
    exit(1);
  }
  // Initialize pfds array
  pa->pfds = malloc(sizeof(struct pollfd) * INIT_ARR_SIZE);
  if (!pa->pfds) {
    fprintf(stderr, "server: malloc failed for pfds array\n");
    free(pa);
    exit(1);
  }
  pa->count = 0;
  pa->size  = INIT_ARR_SIZE;

  int listener = get_listener_socket();
  if (listener == -1) {
    fprintf(stderr, "server: could not create listener\n");
    exit(2);
  }
  // add listener to pfds array
  add_to_pfds(pa, listener);

  struct sockaddr_storage remoteaddr;
  socklen_t addrlen;
  char buf[MAX_BUF_SIZE];
  char remoteIP[INET6_ADDRSTRLEN];

  // start event loop
  while (1) {
    int poll_count = poll(pa->pfds, pa->count, -1);
    if (poll_count == -1) {
      perror("server: poll");
      exit(2);
    }

    for (int i = 0; i < pa->count; i++) {
      // If some client got an event (incoming data or hangup)
      if (pa->pfds[i].revents & (POLLIN | POLLHUP)) {
        if (pa->pfds[i].fd == listener) {
          // Listener got a new connection
          addrlen = sizeof remoteaddr;
          int newfd = accept(listener,
                             (struct sockaddr *)&remoteaddr,
                             &addrlen);
          if (newfd == -1) {
            perror("server: accept");
            continue;
          }

          // add new client socket to poll array
          add_to_pfds(pa, newfd);
          printf("server: got connection from %s on socket %d\n",
                 inet_ntop(remoteaddr.ss_family,
                           get_in_addr((struct sockaddr *)&remoteaddr),
                           remoteIP, INET6_ADDRSTRLEN),
                 newfd);
        }
        else {
          // Data from a client socket
          int senderfd = pa->pfds[i].fd;
          int nbytes = recv(senderfd, buf, sizeof(buf) - 1, 0);

          if (nbytes <= 0) {
            // nbytes == 0: client closed; nbytes == -1: error
            if (nbytes == 0)
              printf("server: socket %d hung up\n", senderfd);
            else
              perror("server: recv");
            del_from_pfds(pa, i);
            i--;  // adjust index after removal
          } else {
            buf[nbytes] = '\0';
            printf("server: got '%s' from socket %d\n", buf, senderfd);

            // Broadcast message to other sockets
            for (int j = 0; j < pa->count; j++) {
              int destfd = pa->pfds[j].fd;
              if (destfd != senderfd && destfd != listener) {
                if (send(destfd, buf, nbytes, 0) == -1) {
                  perror("server: send");
                } else {
                  printf("server: sent message to socket %d\n", destfd);
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
