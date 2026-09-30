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

static void handle_client(int fd) {
    char buffer[4096];
    recv(fd, buffer, sizeof(buffer), 0);
    const char response[] = "HTTP/1.1 200 OK\r\n\r\n";
    send_all(fd, response, sizeof(response) - 1);
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
