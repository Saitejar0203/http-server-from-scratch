import argparse
import gzip
import os
from pathlib import Path
import socket
import threading


def read_request(conn, pending):
    # Keep bytes beyond this request for the next request on this connection.
    while b"\r\n\r\n" not in pending:
        chunk = conn.recv(4096)
        if not chunk:
            return None
        pending += chunk
    head, pending = pending.split(b"\r\n\r\n", 1)
    lines = head.split(b"\r\n")
    method, path, version = lines[0].split(b" ")
    headers = {}
    for line in lines[1:]:
        name, value = line.split(b":", 1)
        headers[name.lower()] = value.strip(b" \t")
    length_value = headers.get(b"content-length", b"0")
    if not length_value.isdigit():
        raise ValueError("Invalid Content-Length")
    length = int(length_value)
    while len(pending) < length:
        chunk = conn.recv(min(4096, length - len(pending)))
        if not chunk:
            return None
        pending += chunk
    return method, path, headers, pending[:length], pending[length:]


def build_response(method, path, headers, body, directory):
    if path == b"/":
        response = b"HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n"
    elif path.startswith(b"/echo/") or path == b"/user-agent":
        body = (headers.get(b"user-agent", b"") if path == b"/user-agent"
                else path[len(b"/echo/"):])
        encoding_header = b""
        accepted_encodings = [
            value.strip(b" \t").lower()
            for value in headers.get(b"accept-encoding", b"").split(b",")
        ]
        if path.startswith(b"/echo/") and b"gzip" in accepted_encodings:
            body = gzip.compress(body, mtime=0)
            encoding_header = b"Content-Encoding: gzip\r\n"
        response = (
            b"HTTP/1.1 200 OK\r\n"
            b"Content-Type: text/plain\r\n"
            + encoding_header
            + f"Content-Length: {len(body)}\r\n\r\n".encode("ascii")
            + body
        )
    elif method == b"POST" and path.startswith(b"/files/"):
        try:
            if directory is None:
                raise FileNotFoundError
            root = Path(directory).resolve()
            file_path = (root / os.fsdecode(path[len(b"/files/"):])).resolve()
            if not file_path.is_relative_to(root) or file_path == root:
                raise FileNotFoundError
            file_path.write_bytes(body)
            response = b"HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n"
        except (OSError, ValueError):
            response = b"HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n"
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
            response = b"HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n"
    else:
        response = b"HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n"
    return response


def handle_client(conn, directory=None):
    with conn:
        pending = b""
        while True:
            try:
                parsed = read_request(conn, pending)
            except ValueError:
                conn.sendall(b"HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n")
                return
            if parsed is None:
                return
            method, path, headers, body, pending = parsed
            response = build_response(method, path, headers, body, directory)
            conn.sendall(response)


def serve_connection(conn, directory):
    # A disconnected client must not affect another connection worker.
    try:
        handle_client(conn, directory)
    except (ConnectionError, TimeoutError):
        pass


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--directory")
    args = parser.parse_args()
    print("Logs from your program will appear here!")
    with socket.create_server(("localhost", 4221), reuse_port=True) as server_socket:
        while True:
            conn, address = server_socket.accept() # wait for client
            threading.Thread(target=serve_connection, args=(conn, args.directory), daemon=True).start()


if __name__ == "__main__":
    main()
