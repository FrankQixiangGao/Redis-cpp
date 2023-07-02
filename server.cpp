#include <arpa/inet.h>  // for sockaddr_in and inet_ntoa()
#include <stdlib.h>     // for exit()
#include <string.h>     // for memset()
#include <sys/socket.h> // for socket(), bind(), and connect()
#include <unistd.h>     // for close()

void die(char *errorMessage) {
    // perror(errorMessage);
    exit(1);
}

void do_something(int connfd){
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if(n < 0) {
        die("read()");
    }
}

int main() {
    int val = 1;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        die("socket()");
    }

    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        die("setsockopt()");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    int rv = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if(rv < 0) {
        die("bind()");
    }

    rv = listen(fd, SOMAXCONN);
    if(rv < 0) {
        die("listen()");
    }

    while(true){
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
        if(connfd < 0) {
            continue; // error
        }
        do_something(connfd);
        close(connfd);
    }
    
    return 0;
}
