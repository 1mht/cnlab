###############################################################################
# server-python.py
# Name:
# NetId:
###############################################################################

import sys
import socket

RECV_BUFFER_SIZE = 2048
QUEUE_LENGTH = 10    

def server(server_port):
    """TODO: Listen on socket and print received message to sys.stdout"""
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) # 通用socket级别 -> 地址复用
    s.bind(("127.0.0.1", server_port))
    s.listen(QUEUE_LENGTH)
    out = sys.stdout.buffer
    while True:
        conn, addr = s.accept()
        with conn:
            # print(f"Connection from {addr}")
            while True:
                data = conn.recv(RECV_BUFFER_SIZE)
                if not data:
                    break
                out.write(data)
                out.flush()


def main():
    """Parse command-line argument and call server function """
    if len(sys.argv) != 2:
        sys.exit("Usage: python server-python.py [Server Port]")
    server_port = int(sys.argv[1])
    server(server_port)

if __name__ == "__main__":
    main()


# python server-python.py 10101 > [output file]
# python client-python.py 127.0.0.1 10101 < [message file]