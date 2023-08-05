#include <arpa/inet.h>  // for sockaddr_in and inet_ntoa()
#include <stdlib.h>     // for exit()
#include <string.h>     // for memset()
#include <sys/socket.h> // for socket(), bind(), and connect()
#include <unistd.h>     // for close()

const size_t k_max_msg = 4096;

// handle error cases and terminate the program
void die(char *errorMessage) {
    // perror(errorMessage);
    exit(1);
}

// handle the communication with the client
void do_something(int connfd){
    // Buffer to hold data read from the client
    char rbuf[64] = {};

    // Read data from client and handle error
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if(n < 0) {
        die("read()");
    }
}


int main() {
    // Initialize variable
    int val = 1;

    // create the socket and handle any error
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        die("socket()");
    }

    // Set socket options and handle any error
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        die("setsockopt()");
    }

    // Initialize and set up the server address structure
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    // Bind the socket to the address and handle any error
    int rv = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if(rv < 0) {
        die("bind()");
    }

    // Start listening on the socket and handle any error
    rv = listen(fd, SOMAXCONN);
    if(rv < 0) {
        die("listen()");
    }

    // Accept and handle incoming connections indefinitely
    while(true){
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);

        // Accept a client connection and handle any error
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
        if(connfd < 0) {
            continue; // error,
        }

        // Handle the client connection and then close it
        do_something(connfd);
        close(connfd);
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
    memcpy(&len, rbuf, 4); // assume little endian

    if (len > k_max_msg) {
        msg("message too long");
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
    char wbuf[$ + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);
}