"""Run against a listening server: python3 test_concurrency.py [port]."""
import concurrent.futures
import gzip
import socket
import sys

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4221

def connect():
    return socket.create_connection(('localhost', PORT), timeout=3)

def response(stream):
    status = stream.readline()
    assert b' 200 ' in status, status
    headers = {}
    while (line := stream.readline()) != b'\r\n':
        assert line, 'Unexpected EOF in headers'
        key, value = line.split(b':', 1)
        headers[key.lower()] = value.strip()
    size = int(headers[b'content-length'])
    body = stream.read(size)
    assert len(body) == size
    return headers, body

def client(number):
    with connect() as connection, connection.makefile('rb') as stream:
        # Two requests arrive in one stream before either response is read.
        for batch in range(4):
            first = f'client-{number}-batch-{batch}'.encode()
            second = first + b'-gzip'
            connection.sendall(b'GET /echo/' + first + b' HTTP/1.1\r\nHost: localhost\r\n\r\n' +
                               b'GET /echo/' + second + b' HTTP/1.1\r\nAccept-Encoding: bogus, gzip\r\n\r\n')
            _, body = response(stream)
            assert body == first
            headers, body = response(stream)
            assert headers[b'content-encoding'] == b'gzip'
            assert gzip.decompress(body) == second

if __name__ == '__main__':
    # A stalled reader must not block other clients, nor share their buffers.
    with connect() as idle:
        idle.sendall(b'GET /echo/incomplete HTTP/1.1\r\n')
        with concurrent.futures.ThreadPoolExecutor(max_workers=24) as pool:
            list(pool.map(client, range(48)))
    print('Passed: 48 clients, 384 persistent responses, idle peer and gzip round trips')
