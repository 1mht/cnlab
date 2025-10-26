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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define SEND_BUFFER_SIZE 2048


/* TODO: client()
 * Open socket and send message from stdin.
 * Return 0 on success, non-zero on failure
*/
int client(char *server_ip, char *server_port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    perror("socket");
    return 1;
  };

  struct sockaddr_in saddr;
  memset(&saddr, 0, sizeof saddr);
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(atoi(server_port));

  int ipok = inet_pton(AF_INET, server_ip, &saddr.sin_addr.s_addr);
  if (ipok == 0) {
    fprintf(stderr, "inet_pton: invalid IPv4 address '%s'\n", server_ip);
    return 1;
  } else if (ipok < 0) {
    perror("inet_pton");
    return 1;
  }

  if (connect(fd, (struct sockaddr *)&saddr, sizeof saddr) == -1) {
    perror("connect");
    close(fd);
    return 1;
  }

  char buf[SEND_BUFFER_SIZE];
  // 从 stdin 读数据，发送到服务器（处理部分发送）
  size_t nread;
  while ((nread = fread(buf, 1, sizeof buf, stdin)) > 0) {
    int off = 0;
    while (off < (int)nread) {
      size_t to_send = (size_t)((int)nread - off);
      ssize_t sent = send(fd, buf + off, to_send, 0);
      if (sent == -1) {
        perror("send");
        close(fd);
        return 1;
      }
      off += (int)sent;
    }
  }
  if (ferror(stdin)) perror("fread");

  close(fd);
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
