#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static volatile sig_atomic_t got_sig = 0;
void handler(int sig) {
    got_sig = sig;
}

ssize_t write_with_retry(int fd, const void *buf, size_t len) {
    ssize_t ret;
    do {
        ret = write(fd, buf, len);
    } while (ret < 0 && errno == EINTR);
    return ret;
}

int main(){
    struct sigaction sa = {.sa_handler = handler, .sa_flags = 0};
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    size_t size = 1024*1024*500; // 500 MB
    char *buf = malloc(size);
    memset(buf, 'A', size);

    printf("Start writing. Press Ctrl+C to send SIGINT and interrupt write().\n");

    ssize_t r = write(STDOUT_FILENO, buf, size);
    if (r < 0) {
        if (errno == EINTR) {
            printf("write() was interrupted by signal %d after writing %zd bytes\n", got_sig, r);
        } else {
            perror("write()");
        }
    } else {
        printf("write() completed successfully: %zd bytes\n", r);
    }
    free(buf);
    return 0;
}