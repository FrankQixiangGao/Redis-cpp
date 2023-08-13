#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#define k_max_msg 4096

static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg) {
    int err = errno;
    char err_msg[256];
    if(strerror_r(err, err_msg, sizeof(err_msg)) == 0) {
        fprintf(stderr, "Error %d: %s - %s\n", err, err_msg, msg);
    } else {
        fprintf(stderr, "Error %d: %s\n", err, msg);
    }
    abort();
}

static int32_t read_full(int fd, char *buf, size_t n) {
    if (buf == NULL) {
        return -1;
    }

    ssize_t rv;
    while (n > 0) {
        rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error, or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n) {
    if (buf == NULL) {
        return -1;
    }

    ssize_t rv;
    while (n > 0) {
        rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}


static int32_t one_request(int connfd) {
    // 4 bytes header
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("Message length exceeds limit");
        return -1;
    }

    // request body
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // do something
    rbuf[4 + len] = '\0';
    printf("client says: %s\n", &rbuf[4]);

    // reply using the same protocol
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    // uint32_t nlen = htonl(len);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    // this is needed for most server applications
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);    // wildcard address 0.0.0.0
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("bind()");
    }

    // listen
    rv = listen(fd, SOMAXCONN);
    if (rv) {
        die("listen()");
    }

    while (true) {
        // accept
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
        if (connfd < 0) {
            continue;   // error
        }

        while (true) {
            // here the server only serves one client connection at once
            int32_t err = one_request(connfd);
            if (err) {
                break;
            }
        }
        close(connfd);
    }

    return 0;
}

// static void fd_set_nb(int fd) {
//     errno = 0;
//     int flags = fcntl(fd, F_GETFL, 0);
//     if (errno) {
//         die("fcntl error");
//         return;
//     }

//     flags |= O_NONBLOCK;

//     errno = 0;
//     (void)fcntl(fd, F_SETFL, flags);
//     if (errno) {
//         die("fcntl error");
//     }
// }

enum {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,
};

struct Conn {
    int fd = -1;
    uint32_t state = 0 // either STATE_REQ or STATE_RES
    size_t rbuf_size = 0;
    uint8_t rbuf[4 + k_max_msg + 1];
    // buffer for writing
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + k_max_msg + 1];
}

