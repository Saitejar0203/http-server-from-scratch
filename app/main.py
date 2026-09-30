import socket  # noqa: F401


def main():

    print("Logs from your program will appear here!")

    with socket.create_server(("localhost", 4221), reuse_port=True) as server_socket:
        while True:
            conn, address = server_socket.accept() # wait for client
            with conn:
                request = b""
                # TCP reads may return only part of the request.
                while b"\r\n\r\n" not in request:
                    chunk = conn.recv(1024)
                    if not chunk:
                        break
                    request += chunk

                if b"\r\n\r\n" not in request:
                    continue  # Client disconnected before sending complete headers.

                request_line = request.split(b"\r\n", 1)[0]
                method, path, version = request_line.split(b" ")

                if path == b"/":
                    response = b"HTTP/1.1 200 OK\r\n\r\n"
                else:
                    response = b"HTTP/1.1 404 Not Found\r\n\r\n"

                conn.sendall(response)



if __name__ == "__main__":
    main()
