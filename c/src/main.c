#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
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
#define BODY_LIMIT (64 * 1024 * 1024)

typedef struct {
    unsigned char *data;
    size_t length;
    size_t capacity;
} Buffer;

static int receive_more(int fd, Buffer *buffer) {
    if (buffer->length >= BODY_LIMIT + HEADER_LIMIT) return -1;
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

static int respond(int fd, int status, const char *type,
                   const unsigned char *body, size_t length) {
    char header[512];
    const char *reason = status == 200 ? "OK" : status == 201 ? "Created" :
                         status == 400 ? "Bad Request" : "Not Found";
    int n = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n",
        status, reason, type, length);
    if (n < 0 || (size_t)n >= sizeof(header)) return -1;
    if (send_all(fd, header, (size_t)n) < 0) return -1;
    return send_all(fd, body, length);
}

/* The caller owns the returned header value. Header names are case-insensitive. */
static char *header_value(const char *headers, const char *name) {
    size_t name_length = strlen(name);
    while (*headers && strncmp(headers, "\r\n", 2) != 0) {
        const char *end = strstr(headers, "\r\n");
        if (!end) break;
        const char *colon = memchr(headers, ':', (size_t)(end - headers));
        if (colon && (size_t)(colon - headers) == name_length &&
            strncasecmp(headers, name, name_length) == 0) {
            const char *value = colon + 1;
            while (value < end && (*value == ' ' || *value == '\t')) value++;
            while (end > value && (end[-1] == ' ' || end[-1] == '\t')) end--;
            size_t length = (size_t)(end - value);
            char *copy = malloc(length + 1);
            if (copy) { memcpy(copy, value, length); copy[length] = 0; }
            return copy;
        }
        headers = end + 2;
    }
    return strdup("");
}

static int files_directory = -1;

static int valid_filename(const char *name) {
    return *name && !strchr(name, '/') && strcmp(name, ".") && strcmp(name, "..");
}

static void return_file(int fd, const char *name) {
    int file = -1;
    unsigned char *body = NULL;
    struct stat st;
    if (files_directory < 0 || !valid_filename(name)) goto missing;
    file = openat(files_directory, name, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    if (file < 0 || fstat(file, &st) < 0 || !S_ISREG(st.st_mode) ||
        st.st_size < 0 || st.st_size > 64 * 1024 * 1024) goto missing;
    size_t size = (size_t)st.st_size;
    body = malloc(size ? size : 1);
    if (!body) goto missing;
    size_t used = 0;
    while (used < size) {
        ssize_t n = read(file, body + used, size - used);
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) goto missing;
        if (!n) break;
        used += (size_t)n;
    }
    respond(fd, 200, "application/octet-stream", body, used);
    free(body); close(file); return;
missing:
    free(body);
    if (file >= 0) close(file);
    respond(fd, 404, "text/plain", NULL, 0);
}

static void save_file(int fd, const char *name, const unsigned char *body, size_t length) {
    int file = -1;
    if (files_directory < 0 || !valid_filename(name)) goto failure;
    file = openat(files_directory, name, O_WRONLY | O_CREAT | O_NOFOLLOW | O_NONBLOCK, 0666);
    struct stat st;
    if (file < 0 || fstat(file, &st) < 0 || !S_ISREG(st.st_mode) || ftruncate(file, 0) < 0)
        goto failure;
    size_t written = 0;
    while (written < length) {
        ssize_t n = write(file, body + written, length - written);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) goto failure;
        written += (size_t)n;
    }
    if (close(file) < 0) { file = -1; goto failure; }
    respond(fd, 201, "text/plain", NULL, 0);
    return;
failure:
    if (file >= 0) close(file);
    respond(fd, 404, "text/plain", NULL, 0);
}

static void handle_client(int fd) {
    Buffer buffer = {0};
    char *head = NULL;
    while (!buffer.data || !strstr((char *)buffer.data, "\r\n\r\n")) {
        if (buffer.length >= HEADER_LIMIT || receive_more(fd, &buffer) < 0) goto done;
    }
    size_t header_length = (size_t)(strstr((char *)buffer.data, "\r\n\r\n") - (char *)buffer.data) + 4;
    head = malloc(header_length + 1);
    if (!head) goto done;
    memcpy(head, buffer.data, header_length); head[header_length] = 0;
    char *line_end = strstr(head, "\r\n");
    const char *headers = line_end + 2;
    *line_end = 0;
    char *save = NULL;
    char *method = strtok_r(head, " ", &save);
    char *path = strtok_r(NULL, " ", &save);
    char *length_text = header_value(headers, "Content-Length");
    if (!length_text) goto done;
    size_t body_length = 0;
    for (char *digit = length_text; *digit; digit++) {
        if (*digit < '0' || *digit > '9' || body_length > BODY_LIMIT / 10) {
            free(length_text); respond(fd, 400, "text/plain", NULL, 0); goto done;
        }
        body_length = body_length * 10 + (size_t)(*digit - '0');
    }
    free(length_text);
    if (body_length > BODY_LIMIT) { respond(fd, 400, "text/plain", NULL, 0); goto done; }
    while (buffer.length < header_length + body_length) {
        if (receive_more(fd, &buffer) < 0) goto done;
    }
    if (method && path) {
        if (strcmp(path, "/") == 0) {
            respond(fd, 200, "text/plain", NULL, 0);
        } else if (strncmp(path, "/echo/", 6) == 0) {
            const unsigned char *body = (unsigned char *)path + 6;
            respond(fd, 200, "text/plain", body, strlen((char *)body));
        } else if (strcmp(method, "POST") == 0 && strncmp(path, "/files/", 7) == 0) {
            save_file(fd, path + 7, buffer.data + header_length, body_length);
        } else if (strcmp(method, "GET") == 0 && strncmp(path, "/files/", 7) == 0) {
            return_file(fd, path + 7);
        } else if (strcmp(path, "/user-agent") == 0) {
            char *agent = header_value(headers, "User-Agent");
            if (agent) respond(fd, 200, "text/plain", (unsigned char *)agent, strlen(agent));
            free(agent);
        } else {
            respond(fd, 404, "text/plain", NULL, 0);
        }
    }
done:
    free(head);
    free(buffer.data);
}

static void *worker(void *argument) {
    int fd = *(int *)argument;
    free(argument);
    handle_client(fd);
    close(fd);
    return NULL;
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--directory") == 0 && i + 1 < argc) {
            files_directory = open(argv[++i], O_RDONLY | O_DIRECTORY);
            if (files_directory < 0) { perror("directory"); return 1; }
        } else { fprintf(stderr, "Usage: %s [--directory PATH]\n", argv[0]); return 1; }
    }
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
        int *owned_fd = malloc(sizeof(*owned_fd));
        if (!owned_fd) { close(client); continue; }
        *owned_fd = client;
        pthread_t thread;
        if (pthread_create(&thread, NULL, worker, owned_fd) != 0) {
            free(owned_fd); close(client); continue;
        }
        pthread_detach(thread);
    }
    close(listener);
    return 1;
}
