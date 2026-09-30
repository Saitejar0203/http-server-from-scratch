import argparse
import os
from pathlib import Path
import socket
import threading


def handle_client(conn, directory=None):
    with conn:
        request = b""
        # TCP reads may return only part of the request.
        while b"\r\n\r\n" not in request:
            chunk = conn.recv(1024)
            if not chunk:
                break
            request += chunk

        if b"\r\n\r\n" not in request:
            return  # Client disconnected before sending complete headers.

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
        elif method == b"GET" and path.startswith(b"/files/"):
            try:
                if directory is None:
                    raise FileNotFoundError
                root = Path(directory).resolve()
                file_path = (root / os.fsdecode(path[len(b"/files/"):])).resolve()
                if not file_path.is_relative_to(root) or not file_path.is_file():
                    raise FileNotFoundError
                body = file_path.read_bytes()
                response = (
                    b"HTTP/1.1 200 OK\r\n"
                    b"Content-Type: application/octet-stream\r\n"
                    + f"Content-Length: {len(body)}\r\n\r\n".encode("ascii")
                    + body
                )
            except (OSError, ValueError):
                response = b"HTTP/1.1 404 Not Found\r\n\r\n"
        else:
            response = b"HTTP/1.1 404 Not Found\r\n\r\n"

        conn.sendall(response)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--directory")
    args = parser.parse_args()
    print("Logs from your program will appear here!")
    with socket.create_server(("localhost", 4221), reuse_port=True) as server_socket:
        while True:
            conn, address = server_socket.accept() # wait for client
            threading.Thread(target=handle_client, args=(conn, args.directory), daemon=True).start()


if __name__ == "__main__":
    main()
