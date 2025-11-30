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
	 // 读取客户端请求
	 req, err := http.ReadRequest(bufio.NewReader(conn))
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
 
	 // 创建一个新的请求发送到远程服务器，默认端口号80
	 url := req.URL
	 remoteAddr := strings.Split(url.Host, ":")[0]
	 remotePort := "80"
	 if len(strings.Split(url.Host, ":")) > 1 {
		 remotePort = strings.Split(url.Host, ":")[1]
	 }
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
 
	 // 发送请求到远程服务器
	 if err := req.Write(remoteConn); err != nil {
		 resp := fmt.Sprintf("HTTP/1.1 500 Internal Error\r\nContent-Length: 0\r\nConnection: close\r\n\r\n")
		 conn.Write([]byte(resp))
		 return
	 }
 
	 // 从远程服务器读取响应并转发给客户端 
	 // 使用1024字节缓冲区循环读取服务器响应并原样转发给客户端
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
	 port := os.Args[1] // 从命令行参数获取端口号
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
 