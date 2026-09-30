#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int send_all(int fd, const void *data, size_t length) {
    const unsigned char *p = data;
    while (length) {
        ssize_t n = send(fd, p, length, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        p += n; length -= (size_t)n;
    }
    return 0;
}

#define HEADER_LIMIT (64 * 1024)

typedef struct {
    unsigned char *data;
    size_t length;
    size_t capacity;
} Buffer;

static int receive_more(int fd, Buffer *buffer) {
    if (buffer->length >= HEADER_LIMIT) return -1;
    if (buffer->capacity - buffer->length < 4096 + 1) {
        size_t capacity = buffer->capacity + 4096 + 1;
        void *grown = realloc(buffer->data, capacity);
        if (!grown) return -1;
        buffer->data = grown; buffer->capacity = capacity;
    }
    ssize_t n;
    do { n = recv(fd, buffer->data + buffer->length, 4096, 0); }
    while (n < 0 && errno == EINTR);
    if (n <= 0) return -1;
    buffer->length += (size_t)n;
    buffer->data[buffer->length] = 0;
    return 0;
}

static void handle_client(int fd) {
    Buffer buffer = {0};
    while (!buffer.data || !strstr((char *)buffer.data, "\r\n\r\n")) {
        if (receive_more(fd, &buffer) < 0) { free(buffer.data); return; }
    }
    char *line_end = strstr((char *)buffer.data, "\r\n");
    *line_end = 0;
    char *save = NULL;
    char *method = strtok_r((char *)buffer.data, " ", &save);
    char *path = strtok_r(NULL, " ", &save);
    if (method && path) {
        const char *response = strcmp(path, "/") == 0
            ? "HTTP/1.1 200 OK\r\n\r\n"
            : "HTTP/1.1 404 Not Found\r\n\r\n";
        send_all(fd, response, strlen(response));
    }
    free(buffer.data);
}

int main(void) {
    signal(SIGPIPE, SIG_IGN);
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) { perror("socket"); return 1; }
    int reuse = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(4221);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listener, (struct sockaddr *)&address, sizeof(address)) < 0 ||
        listen(listener, 128) < 0) {
        perror("listen/bind"); close(listener); return 1;
    }
    for (;;) {
        int client = accept(listener, NULL, NULL);
        if (client < 0) { if (errno == EINTR) continue; perror("accept"); break; }
        handle_client(client);
        close(client);
    }
    close(listener);
    return 1;
}
