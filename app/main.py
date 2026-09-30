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

                headers = {}
                for line in request.split(b"\r\n\r\n", 1)[0].split(b"\r\n")[1:]:
                    name, value = line.split(b":", 1)
                    headers[name.lower()] = value.strip(b" \t")

                if path == b"/":
                    response = b"HTTP/1.1 200 OK\r\n\r\n"
                elif path.startswith(b"/echo/") or path == b"/user-agent":
                    body = (headers.get(b"user-agent", b"") if path == b"/user-agent"
                            else path[len(b"/echo/"):])
                    response = (
                        b"HTTP/1.1 200 OK\r\n"
                        b"Content-Type: text/plain\r\n"
                        + f"Content-Length: {len(body)}\r\n\r\n".encode("ascii")
                        + body
                    )
                else:
                    response = b"HTTP/1.1 404 Not Found\r\n\r\n"

                conn.sendall(response)



if __name__ == "__main__":
    main()
