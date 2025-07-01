#! /usr/bin/env python3
import os
import socket
import random

def main():
    # # Get Ephemeral port
    # listen_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # listen_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    # listen_socket.bind(('0.0.0.0', 0)) 
    # port = listen_socket.getsockname()[1]

    # Get random port
    ports = list(range(1024, 32767))
    random.shuffle(ports)
    for p in ports:
        try:
            listen_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            listen_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            listen_socket.bind(('0.0.0.0', p))
            port = p
            break
        except OSError:
            continue

    # Fork the process
    pid = os.fork()

    if pid == 0:
        listen_socket.close()
        connect_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        connect_socket.connect(('127.0.0.1', port))
    else:
        listen_socket.listen(1)
        listen_socket.accept()
        listen_socket.close()
        print(port, end='')


if __name__ == '__main__':
    main()