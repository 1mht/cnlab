server N client _ addrinfo()
## 一、socket编程流程概述

### 服务端（server）的典型流程

1. **地址与端口解析**（getaddrinfo）
2. **创建socket**（socket）
3. **设置socket选项**（setsockopt，常用于端口重用）
4. **绑定端口**（bind）
5. **监听端口**（listen）
6. **等待并接受客户端连接**（accept）
7. **接收数据**（recv）
8. **处理数据**（如fwrite输出到屏幕）
9. **关闭连接**（close）

### 客户端（client）的典型流程

1. **地址与端口解析**（getaddrinfo）
2. **创建socket**（socket）
3. **连接服务端**（connect）
4. **发送数据**（send）
5. **关闭连接**（close）

---

## 二、逐步详细解释

### 1. 地址与端口解析：getaddrinfo/freeaddrinfo

#### 作用
- `getaddrinfo` 根据 IP（或域名）、端口号和协议类型，返回一个（或多个）可用于 socket 的地址结构体。
- `freeaddrinfo` 用于释放由 `getaddrinfo` 动态分配的内存。

#### server 端用法
```c
memset(&hints, 0, sizeof hints);
hints.ai_family = AF_INET;
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE; // 作为服务器，需要绑定本地地址

rv = getaddrinfo("127.0.0.1", server_port, &hints, &res);
```
- `hints`用于指定期望的地址类型（如 IPv4、TCP）。
- 这里 `"127.0.0.1"` 为本地环回地址，`server_port` 是端口号。
- 得到的`res`是一个链表，存储各种可能的地址组合（这里只用第一个）。

#### client 端用法
```c
memset(&hints, 0, sizeof hints);
hints.ai_family = AF_INET;
hints.ai_socktype = SOCK_STREAM;

rv = getaddrinfo(server_ip, server_port, &hints, &res);
```
- 与server类似，但没有`AI_PASSIVE`，因为client主动连接目标地址。
- `server_ip`和`server_port`由命令行参数传入，指定要连接的服务端。

#### freeaddrinfo
- 用完后必须调用`freeaddrinfo(res)`释放资源，防止内存泄漏。

---

### 2. 创建 socket

```c
sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
```
- 用`getaddrinfo`得到的参数创建socket（包括协议族、类型、协议）。

---

### 3. 服务端额外步骤

#### setsockopt（端口复用）
```c
int yes = 1;
setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
```
- 避免"Address already in use"错误，方便调试。

#### bind
```c
bind(sockfd, res->ai_addr, res->ai_addrlen);
```
- 绑定本地IP和端口号。

#### listen
```c
listen(sockfd, QUEUE_LENGTH);
```
- 开始监听端口，允许最多QUEUE_LENGTH个等待连接。

---

### 4. 建立连接

#### 服务端 accept
```c
int connfd = accept(sockfd, NULL, NULL);
```
- 阻塞等待客户端连接，返回新的已连接socket。

#### 客户端 connect
```c
connect(sockfd, res->ai_addr, res->ai_addrlen);
```
- 主动发起连接到服务器。

---

### 5. 数据收发细节

#### client 端 send
```c
while ((nread = fread(buf, 1, sizeof buf, stdin)) > 0) {
    size_t total = 0;
    while (total < nread) {
        ssize_t sent = send(sockfd, buf + total, nread - total, 0);
        ...
        total += (size_t)sent;
    }
}
```
- 从标准输入读数据到buf，每次最多2048字节。
- 可能`send`一次不能把buf全部发送（例如网络阻塞），所以用内部循环确保所有数据都发完（`total < nread`）。
- `send`返回实际发送字节数，如果出错返回-1。

#### server 端 fwrite
```c
while ((n = recv(connfd, buf, sizeof buf, 0)) > 0) {
    size_t off = 0;
    while (off < (size_t)n) {
        size_t w = fwrite(buf + off, 1, (size_t)n - off, stdout);
        ...
        off += w;
    }
    fflush(stdout);
}
```
- `recv`从socket读取数据到buf。
- 由于`fwrite`理论上也可能一次写不完所有字节（极少见），所以也用内部循环确保写全。
- 读到多少写多少，保证数据完整输出到标准输出。

---

### 6. 关闭连接

- 客户端和服务端用完socket都要`close`，服务端还要关闭主监听socket。

---

## 三、重点解释

### getaddrinfo/freeaddrinfo 工作流程

- `getaddrinfo`将主机名/IP和端口转为一个或多个`struct addrinfo`结构体，包含了协议族、端口、二进制地址等信息，屏蔽了底层细节。
- 通常只用第一个（即`res`本身）。用完后调用`freeaddrinfo`释放链表。

### client中的send

- `fread`从标准输入读取数据，`send`发送到服务端socket。
- 由于`send`可能一次发不完全部数据（返回值<输入长度），需要循环直到全部发完。
- 这样确保数据不会丢失或截断。

### server中的fwrite

- `recv`从socket接收数据到buf，`fwrite`写到标准输出。
- `fwrite`同样可能部分写入，需循环直到写完全部接收到的数据。
- 用`fflush`确保数据及时显示。

---

## 四、总结

- 这段代码完整实现了基于TCP的socket通信，流程标准、鲁棒性好。
- `getaddrinfo/freeaddrinfo`为socket编程提供了统一、灵活的地址处理方式。
- `send`和`fwrite`都可能出现部分发送/写入，代码中都做了健壮处理，确保数据不丢失。
- 适合用于理解和学习socket编程的基本流程和细节。

如需对某一部分再详细举例或分析，可补充说明！