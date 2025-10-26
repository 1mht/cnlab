/*****************************************************************************
 * client-c.c                                                                 
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

#define SEND_BUFFER_SIZE 2048


/* TODO: client()
 * Open socket and send message from stdin.
 * Return 0 on success, non-zero on failure
*/
int client(char *server_ip, char *server_port) {
  struct addrinfo hints, *res;
  int sockfd, rv;
  char buf[SEND_BUFFER_SIZE];

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(server_ip, server_port, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  /* Use only the first addrinfo result (no iteration) */
  if (res == NULL) {
    fprintf(stderr, "client: getaddrinfo returned no results\n");
    return 1;
  }

  sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (sockfd == -1) {
    perror("socket");
    freeaddrinfo(res);
    return 1;
  }

  if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
    perror("connect");
    close(sockfd);
    freeaddrinfo(res);
    return 1;
  }

  size_t nread;
  while ((nread = fread(buf, 1, sizeof buf, stdin)) > 0) {
    size_t total = 0;
    while (total < nread) {
      ssize_t sent = send(sockfd, buf + total, nread - total, 0);
      if (sent == -1) { 
        perror("send"); 
        close(sockfd); 
        freeaddrinfo(res); 
        return 1; 
      }
      total += (size_t)sent;
    }
  }
  if (ferror(stdin)) perror("fread");

  close(sockfd);
  freeaddrinfo(res);
  return 0;
}

/*
 * main()
 * Parse command-line arguments and call client function
*/
int main(int argc, char **argv) {
  char *server_ip;
  char *server_port;

  if (argc != 3) {
    fprintf(stderr, "Usage: ./client-c [server IP] [server port] < [message]\n");
    exit(EXIT_FAILURE);
  }

  server_ip = argv[1];
  server_port = argv[2];
  return client(server_ip, server_port);
}
