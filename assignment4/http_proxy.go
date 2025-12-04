/*****************************************************************************
 * http_proxy.go
 * Names:
 * NetIds:
 *****************************************************************************/

package main

import (
	"bufio"
	"fmt"
	"log"
	"net"
	"net/http"
	"os"
	"strings"
)
 
 func handleConnection(conn net.Conn) {
	 defer conn.Close()
	 // ========== 阶段1: 接收并解析请求 ==========
	 // 读取客户端请求
	 req, err := http.ReadRequest(bufio.NewReader(conn))  // default: 4096 bytes
	 if err != nil {
		 log.Println("Error reading request:", err)
		 return
	 }
 
	 // 只处理GET请求
	 if req.Method != "GET" {
		 resp := fmt.Sprintf("HTTP/1.1 500 Internal Error\r\nContent-Length: 0\r\nConnection: close\r\n\r\n")
		 conn.Write([]byte(resp))
		 return
	 }
 
	 // 创建proxy->server请求，"默认"端口号80
	 url := req.URL
	 remoteAddr := strings.Split(url.Host, ":")[0]
	 remotePort := "80"
	 if len(strings.Split(url.Host, ":")) > 1 {
		 remotePort = strings.Split(url.Host, ":")[1]
	 }
	 // ========== 阶段2: 建立到服务器的连接 ==========
	 // proxy <-> server 建立TCP连接
	 remoteConn, err := net.Dial("tcp", net.JoinHostPort(remoteAddr, remotePort))
	 if err != nil {
		 resp := fmt.Sprintf("HTTP/1.1 500 Internal Error\r\nContent-Length: 0\r\nConnection: close\r\n\r\n")
		 conn.Write([]byte(resp))
		 return
	 }
	 defer remoteConn.Close()
 
	 // 修改请求头以确保关闭连接
	 req.Header.Set("Connection", "close") 
	 req.RequestURI = "" 
	 req.URL.Scheme = "" 
	 req.URL.Host = ""   
 
	 // ========== 阶段3: 转发请求 ==========
	 // client -> server 请求写入 proxy -> server
	 if err := req.Write(remoteConn); err != nil {
		 resp := fmt.Sprintf("HTTP/1.1 500 Internal Error\r\nContent-Length: 0\r\nConnection: close\r\n\r\n")
		 conn.Write([]byte(resp))
		 return
	 }
 
	 // ========== 阶段4: 转发响应 ==========
	 // server -> proxy -> client
	 // buf[1024] 循环读取 server 响应,转发给client
	 buf := make([]byte, 1024)
	 for {
		 n, err := remoteConn.Read(buf)
		 if err != nil {
			 break
		 }
		 if _, err := conn.Write(buf[:n]); err != nil {
			 break
		 }
	 }
 }
 
 func main() {
	 port := os.Args[1] 
	 listener, err := net.Listen("tcp", ":"+port)
	 if err != nil {
		 log.Fatal("Failed to start listening on port ", port, " error: ", err)
	 }
	 defer listener.Close()
	 log.Println("Listening on port", port)
 
	 for {
		 conn, err := listener.Accept()
		 if err != nil {
			 log.Println("Error accepting connection:", err)
			 continue
		 }
		 go handleConnection(conn)
	 }
 }
 