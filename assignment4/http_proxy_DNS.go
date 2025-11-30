/*****************************************************************************
 * http_proxy_DNS.go
 * Names:
 * NetIds:
 *****************************************************************************/

// TODO: implement an HTTP proxy with DNS Prefetching

// Note: it is highly recommended to complete http_proxy.go first, then copy it
// with the name http_proxy_DNS.go, thus overwriting this file, then edit it
// to add DNS prefetching (don't forget to change the filename in the header
// to http_proxy_DNS.go in the copy of http_proxy.go)

package main

import (
	"bufio"
	"fmt"
	"log"
	"net"
	"net/http"
	"os"
	"strings"

	"golang.org/x/net/html"
)

func handleConnection(conn net.Conn) {
	defer conn.Close()
	req, err := http.ReadRequest(bufio.NewReader(conn))
	if err != nil {
		log.Println("Error reading request:", err)
		return
	}

	if req.Method != "GET" {
		resp := fmt.Sprintf("HTTP/1.1 500 Internal Error\r\nContent-Length: 0\r\nConnection: close\r\n\r\n")
		conn.Write([]byte(resp))
		return
	}

	// 创建一个新的请求
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

	// 关闭连接
	req.Header.Set("Connection", "close")
	req.RequestURI = ""
	req.URL.Scheme = ""
	req.URL.Host = ""

	// 发送请求并接受response
	if err := req.Write(remoteConn); err != nil {
		resp := fmt.Sprintf("HTTP/1.1 500 Internal Error\r\nContent-Length: 0\r\nConnection: close\r\n\r\n")
		conn.Write([]byte(resp))
		return
	}

	buf := make([]byte, 1024)
    var response []byte
    for {
        n, err := remoteConn.Read(buf)
        if err != nil {
            break
        }
        response = append(response, buf[:n]...)
        if _, err := conn.Write(buf[:n]); err != nil {
            break
        }
    }

    go DNSCache(response)
}

func DNSCache(htmlDocument []byte) {
	doc, err := html.Parse(strings.NewReader(string(htmlDocument)))

	if err != nil {
		log.Println("Broken html document!")
		return
	}

	// 深搜遍历
	var dfs func(*html.Node)
	dfs = func(node *html.Node) {
		if node.Type == html.ElementNode && node.Data == "a" {
			for _, a := range node.Attr {
				// 有href的a标签
				if a.Key == "href" && strings.HasPrefix(a.Val, "http") {
					log.Println("href=",a.Val)
					host := strings.Split(a.Val, "/")[2] // 获取链接地址（删掉http头）
					go func(host string) {
						result, err := net.LookupHost(host)
						if err != nil {
							log.Println("DNS lookup failed")
						} else{
							log.Println("DNS prefetched: ", host)
							log.Println("Result: ",result)
						}
					}(host)
				}
			}
		}
		for c := node.FirstChild; c != nil; c = c.NextSibling {
			dfs(c)
		}
	}
	dfs(doc)
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
