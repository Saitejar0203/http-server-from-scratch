import socket  # noqa: F401


def main():

    print("Logs from your program will appear here!")

    server_socket = socket.create_server(("localhost", 4221), reuse_port=True)
    conn, address = server_socket.accept() # wait for client
    with conn:
        conn.recv(1024)  # Read request bytes; parsing comes in later stages.
        conn.sendall(b"HTTP/1.1 200 OK\r\n\r\n")
    server_socket.close()


if __name__ == "__main__":
    main()
