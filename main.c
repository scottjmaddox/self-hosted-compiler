#define size_t unsigned long
#define ssize_t long

extern int open(char *path, int oflag, ...); // fcntl.h
extern int close(int fildes); // unistd.h
extern ssize_t read(int fildes, void *buf, size_t nbyte); // unistd.h
extern ssize_t write(int fildes, void *buf, size_t nbyte); // unistd.h
extern void perror(char *s); // stdio.h
extern void exit(int status); // stdlib.h
extern void *malloc(size_t size); // stdlib.h
extern void free(void *ptr); // stdlib.h

#define stdout          1
#define stderr          2
#define O_RDONLY        0x0000          /* open for reading only */

// ----------------------------------------------------------------------------

int fdin;
char *inbuf;
int inlen = 0;

int fdout;
char *outbuf;
int outlen = 0;

// ----------------------------------------------------------------------------

#define BUFSIZE 32768

void infill(void) {
    char *buf = inbuf;
    int read_bytes = 0;
    while (inlen < BUFSIZE) {
        read_bytes = read(fdin, buf, BUFSIZE - inlen);
        if (read_bytes == -1) {
            perror("fdin read");
            exit(1);
        }
        if (read_bytes == 0) {
            break; // EOF
        }
        buf += read_bytes;
        inlen += read_bytes;
    }
}

void outflush(void) {
    char *buf = outbuf;
    int remaining = outlen;
    int written = 0;
    while (remaining > 0) {
        written = write(fdout, buf, remaining);
        if (written == -1) {
            perror("fdout write");
            exit(1);
        }
        buf += written;
        remaining -= written;
    }
    outlen = 0;
}

void compile(void) {
    // TODO
}

int main(int argc, char **argv) {
    if (argc != 2) {
        write(stderr, "usage: cc FILE\n", sizeof("usage: cc FILE\n") - 1);
        exit(1);
    }
    fdin = open(argv[1], O_RDONLY);
    if (fdin == -1) {
        perror("fdin open");
        exit(1);
    }
    inbuf = malloc(BUFSIZE);
    if (!inbuf) {
        perror("inbuf malloc");
        exit(1);
    }
    fdout = stdout;
    outbuf = malloc(BUFSIZE);
    if (!outbuf) {
        perror("outbuf malloc");
        exit(1);
    }
    compile();
    outflush();
    // Cleanup handled by OS on exit:
    // free(outbuf);
    // free(inbuf);
    // close(fdin);
    return 0;
}
