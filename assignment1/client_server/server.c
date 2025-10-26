/*****************************************************************************
 * server.c                                                                 
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
 * Open socket and wait for clients to connect (infinite loop)
 * Print each received message to stdout
 * Return 0 on success, non-zero on failure
*/
int server(char *server_port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return 1;
  }

  struct sockaddr_in saddr;
  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(atoi(server_port));
  saddr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0: 监听所有网卡（包含 127.0.0.1）

  // 允许地址复用
  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
    perror("setsockopt");
    close(fd);
    return 1;
  }

  int ret = bind(fd, (struct sockaddr *)&saddr, sizeof(saddr));
  if (ret < 0) {
    perror("bind");
    close(fd);
    return 1;
  }

  ret = listen(fd, QUEUE_LENGTH);
  if (ret < 0) {
    perror("listen");
    close(fd);
    return 1;
  }

  char buf[RECV_BUFFER_SIZE];
  for (;;) {
    struct sockaddr_in caddr;
    socklen_t caddr_len = sizeof(caddr);
    int cfd = accept(fd, (struct sockaddr *)&caddr, &caddr_len);
    if (cfd < 0) {
      perror("accept");
      continue; // 继续服务其他客户端
    }

    ssize_t nread;
    while ((nread = recv(cfd, buf, sizeof(buf), 0)) > 0) {
      int off = 0; // nread 最多为缓冲区大小 2048，安全
      while (off < (int)nread) {
        size_t to_write = (size_t)((int)nread - off);
        int w = (int)fwrite(buf + off, 1, to_write, stdout);
        if (w == 0 && ferror(stdout)) {
          perror("fwrite");
          break;
        }
        off += w;
      }
      fflush(stdout);
    }
    if (nread == -1) perror("recv");
    close(cfd);
  }

  // 不会走到这里
  close(fd);
  return 0;
}

/*
 * main():
 * Parse command-line arguments and call server function
*/
int main(int argc, char **argv) {
  char *server_port;

  if (argc != 2) {
    fprintf(stderr, "Usage: ./server [server port] > [output file]\n");
    exit(EXIT_FAILURE);
  }

  server_port = argv[1];
  return server(server_port);
}
