/*****************************************************************************
 * server-c.c                                                                 
 * Name:
 * NetId:
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>

#define QUEUE_LENGTH 10
#define RECV_BUFFER_SIZE 2048

/* TODO: server()
 * Open socket and wait for client to connect
 * Print received message to stdout
 * Return 0 on success, non-zero on failure
*/
int server(char *server_port) {
  int sockfd, rv;
  struct addrinfo hints, *res;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;        // IPv4 only
  hints.ai_socktype = SOCK_STREAM;  // TCP
  hints.ai_flags = AI_PASSIVE;      // use for server bind

  rv = getaddrinfo("127.0.0.1", server_port, &hints, &res);
  if (rv != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (sockfd == -1) {
    perror("socket");
    freeaddrinfo(res);
    return 1;
  }

  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
    perror("setsockopt");
    close(sockfd);
    freeaddrinfo(res);
    return 1;
  }

  if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
    perror("bind");
    close(sockfd);
    freeaddrinfo(res);
    return 1;
  }

  if (listen(sockfd, QUEUE_LENGTH) == -1) {
    perror("listen");
    close(sockfd);
    freeaddrinfo(res);
    return 1;
  }

  freeaddrinfo(res);
  if (sockfd == -1) { 
    fprintf(stderr, "server: failed to bind/listen\n"); 
    return 1; 
  }

  char buf[RECV_BUFFER_SIZE];
  for (;;) {
    int connfd = accept(sockfd, NULL, NULL);
    if (connfd == -1) { 
      perror("accept"); 
      continue; 
    }

    ssize_t n;
    while ((n = recv(connfd, buf, sizeof buf, 0)) > 0) {
      size_t off = 0; // 2048*n
      while (off < (size_t)n) {
        size_t w = fwrite(buf + off, 1, (size_t)n - off, stdout);
        if (w == 0 && ferror(stdout)) { 
          perror("fwrite"); 
          break; 
        }
        off += w;
      }
      fflush(stdout);
    }
    if (n == -1) perror("recv");
    close(connfd);
  }
  close(sockfd);
  return 0;
}

/*
 * main():
 * Parse command-line arguments and call server function
*/
int main(int argc, char **argv) {
  char *server_port;

  if (argc != 2) {
    fprintf(stderr, "Usage: ./server-c [server port]\n");
    exit(EXIT_FAILURE);
  }

  server_port = argv[1];
  return server(server_port);
}
