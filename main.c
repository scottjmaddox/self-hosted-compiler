// Copyright © 2025 by Scott J Maddox. All rights reserved.
// https://github.com/scottjmaddox/single-pass-compiler

#define size_t unsigned long
#define ssize_t long

extern int open(char *path, int oflag, ...); // fcntl.h
extern int close(int fildes); // unistd.h
extern ssize_t read(int fildes, void *buf, size_t nbyte); // unistd.h
extern ssize_t write(int fildes, void *buf, size_t nbyte); // unistd.h
extern void perror(char *s); // stdio.h
extern void exit(int status); // stdlib.h
extern void *malloc(size_t size); // stdlib.h
extern void *realloc(void *ptr, size_t size); // stdlib.h
extern void free(void *ptr); // stdlib.h
extern int strncmp(const char *s1, const char *s2, size_t n); // string.h
extern char *strndup(const char *s1, size_t n); // string.h
extern size_t strlen(const char *s); // string.h

#define stdout          1
#define stderr          2
#define O_RDONLY        0x0000          /* open for reading only */

// ----------------------------------------------------------------------------
// Diagnostic Utilities
// ----------------------------------------------------------------------------

char *inpath;

void eprint_int(int n) {
    char buf[32];
    int i = 0;
    if (n == 0) {
        write(stderr, "0", 1);
        return;
    }
    if (n < 0) {
        write(stderr, "-", 1);
        n = -n;
    }
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    for (int j = i - 1; j >= 0; j--) {
        write(stderr, &buf[j], 1);
    }
}

void eprint_str(char *s) {
    write(stderr, s, strlen(s));
}

void eprint_loc(int col, int line) {
    eprint_str("\n at ");
    write(stderr, inpath, sizeof(inpath) - 1);
    write(stderr, ":", 1);
    eprint_int(line);
    write(stderr, ":", 1);
    eprint_int(col);
    eprint_str("\n");
}

// ----------------------------------------------------------------------------
// Buffered Input and Output
// ----------------------------------------------------------------------------

int fdin;
char *inbuf;
int inlen = 0;
int inidx = 0;

int fdout;
char *outbuf;
int outlen = 0;

void inshift(int n) {
    if (n > inlen) {
        write(stderr, "inshift: n > inlen\n", sizeof("inshift: n > inlen\n") - 1);
        exit(1);
    }
    if (n > inidx) {
        write(stderr, "inshift: n > inidx\n", sizeof("inshift: n > inidx\n") - 1);
        exit(1);
    }
    for (int i = 0; i < inlen - n; i++) {
        inbuf[i] = inbuf[i + n];
    }
    inlen -= n;
    inidx -= n;
}

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

char peak_char(int offset) {
    if (inidx + offset < inlen) {
        return inbuf[inidx + offset];
    }
    inshift(inidx);
    infill();
    if (inidx + offset < inlen) {
        return inbuf[inidx + offset];
    }
    return '\0'; // EOF
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

// ----------------------------------------------------------------------------
// Lexical Analysis
// ----------------------------------------------------------------------------

enum TOKEN {
    TOKEN_EOF,
    TOKEN_EXCLAMATION,
    TOKEN_EXCLAMATION_EQUAL,
    TOKEN_PERCENT,
    TOKEN_AMPERSAND,
    TOKEN_AMPERSAND_AMPERSAND,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_ASTERISK,
    TOKEN_PLUS,
    TOKEN_COMMA,
    TOKEN_MINUS,
    TOKEN_SLASH,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_LESS_THAN,
    TOKEN_LESS_EQUAL,
    TOKEN_LESS_LESS,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER_THAN,
    TOKEN_GREATER_EQUAL,
    TOKEN_GREATER_GREATER,
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    TOKEN_CARET,
    TOKEN_LEFT_BRACE,
    TOKEN_BAR,
    TOKEN_BAR_BAR,
    TOKEN_RIGHT_BRACE,
    TOKEN_TILDE,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_WHILE,
    TOKEN_KEYWORD_BREAK,
    TOKEN_KEYWORD_EXTERN,
    TOKEN_KEYWORD_RETURN,
    TOKEN_KEYWORD_CONTINUE,
    TOKEN_IDENT,
    TOKEN_LITERAL_INT,
    TOKEN_LITERAL_CHAR,
    TOKEN_LITERAL_STR,
};

char *TOKEN_DISPLAY[] = {
    "TOKEN_EOF",
    "TOKEN_EXCLAMATION",
    "TOKEN_EXCLAMATION_EQUAL",
    "TOKEN_PERCENT",
    "TOKEN_AMPERSAND",
    "TOKEN_AMPERSAND_AMPERSAND",
    "TOKEN_LEFT_PAREN",
    "TOKEN_RIGHT_PAREN",
    "TOKEN_ASTERISK",
    "TOKEN_PLUS",
    "TOKEN_COMMA",
    "TOKEN_MINUS",
    "TOKEN_SLASH",
    "TOKEN_COLON",
    "TOKEN_SEMICOLON",
    "TOKEN_LESS_THAN",
    "TOKEN_LESS_EQUAL",
    "TOKEN_LESS_LESS",
    "TOKEN_EQUAL",
    "TOKEN_EQUAL_EQUAL",
    "TOKEN_GREATER_THAN",
    "TOKEN_GREATER_EQUAL",
    "TOKEN_GREATER_GREATER",
    "TOKEN_LEFT_BRACKET",
    "TOKEN_RIGHT_BRACKET",
    "TOKEN_CARET",
    "TOKEN_LEFT_BRACE",
    "TOKEN_BAR",
    "TOKEN_BAR_BAR",
    "TOKEN_RIGHT_BRACE",
    "TOKEN_TILDE",
    "TOKEN_KEYWORD_IF",
    "TOKEN_KEYWORD_ELSE",
    "TOKEN_KEYWORD_WHILE",
    "TOKEN_KEYWORD_BREAK",
    "TOKEN_KEYWORD_EXTERN",
    "TOKEN_KEYWORD_RETURN",
    "TOKEN_KEYWORD_CONTINUE",
    "TOKEN_IDENT",
    "TOKEN_LITERAL_INT",
    "TOKEN_LITERAL_CHAR",
    "TOKEN_LITERAL_STR",
};

int src_pos = 0;
int src_line = 1;
int src_col = 1;

enum TOKEN token;
int token_start_pos = 0;
int token_start_line = 1;
int token_start_col = 1;
int token_end_pos = 0;
int token_end_line = 1;
int token_end_col = 1;
char *token_ident;
int token_literal_int;

void set_token_start(void) {
    token_start_pos = src_pos;
    token_start_line = src_line;
    token_start_col = src_col;
}

void set_token_end(void) {
    token_end_pos = src_pos;
    token_end_line = src_line;
    token_end_col = src_col;
}

void eat_cols(int n) {
    inidx += n;
    src_pos += n;
    src_col += n;
}

void eat_line_comment(void) {
    if (peak_char(0) != '/' || peak_char(1) != '/') {
        eprint_str("eat_line_comment error");
        eprint_loc(src_col, src_line);
        exit(1);
    }
    eat_cols(2);
    char c;
    while ((c = peak_char(0))) {
        inidx += 1;
        src_pos += 1;
        src_col += 1;
        if (c == '\n') {
            src_line += 1;
            src_col = 1;
            break;
        }
    }
}

void eat_block_comment(void) {
    int col = src_col;
    int line = src_line;
    if (peak_char(0) != '/' || peak_char(1) != '*') {
        eprint_str("eat_block_comment error");
        eprint_loc(src_col, src_line);
        exit(1);
    }
    eat_cols(2);
    char c;
    while ((c = peak_char(0))) {
        if (c == '*' && peak_char(1) == '/') {
            eat_cols(2);
            return;
        }
        inidx += 1;
        src_pos += 1;
        src_col += 1;
        if (c == '\n') {
            src_line += 1;
            src_col = 1;
        }
    }
    eprint_str("unterminated block comment");
    eprint_loc(col, line);
    exit(1);
}

void eat_whitespace_and_comments(void) {
    char c;
    while ((c = peak_char(0))) {
        switch (c) {
        case ' ':
        case '\t':
            inidx += 1;
            src_pos += 1;
            src_col += 1;
            continue;
        case '\n':
            inidx += 1;
            src_pos += 1;
            src_line += 1;
            src_col = 1;
            continue;
        case '/':
            switch (peak_char(1)) {
            case '/': eat_line_comment(); continue;
            case '*': eat_block_comment(); continue;
            }
        }
        break;
    }
}

int peak_ident_len(void) {
    int len = 0;
    char c;
    while ((c = peak_char(len))) {
        switch (c) {
        case '_':
        case 'A' ... 'Z':
        case 'a' ... 'z':
        case '0' ... '9':
            len += 1;
            continue;
        }
        break;
    }
    return len;
}

void eat_ident(void) {
    char c;
    while ((c = peak_char(0))) {
        switch (c) {
        case '_':
        case 'A' ... 'Z':
        case 'a' ... 'z':
        case '0' ... '9':
            inidx += 1;
            src_pos += 1;
            src_col += 1;
            continue;
        }
        break;
    }
}

char *intern_ident(char *s, int len) {
    char *result = strndup(s, len);
    // TODO: implement interning logic
    return result;
}

void lex(void) {
    char c;
    int len;
    eat_whitespace_and_comments();
    set_token_start();
    switch (c = peak_char(0)) {
    case '\0': token = TOKEN_EOF; break;
    case '!':
        if (peak_char(1) == '=') { eat_cols(2); token = TOKEN_EXCLAMATION_EQUAL; break; }
        eat_cols(1); token = TOKEN_EXCLAMATION; break;
    // case '"': // TODO: string literal
    case '%': token = TOKEN_PERCENT; break;
    case '&':
        if (peak_char(1) == '&') { eat_cols(2); token = TOKEN_AMPERSAND_AMPERSAND; break; }
        eat_cols(1); token = TOKEN_AMPERSAND; break;
    // case '\'': // TODO: char literal
    case '(': eat_cols(1); token = TOKEN_LEFT_PAREN; break;
    case ')': eat_cols(1); token = TOKEN_RIGHT_PAREN; break;
    case '+': eat_cols(1); token = TOKEN_PLUS; break;
    case ',': eat_cols(1); token = TOKEN_COMMA; break;
    case '*': eat_cols(1); token = TOKEN_ASTERISK; break;
    case '-': eat_cols(1); token = TOKEN_MINUS; break;
    case '/': eat_cols(1); token = TOKEN_SLASH; break;
    case ':': eat_cols(1); token = TOKEN_COLON; break;
    case ';': eat_cols(1); token = TOKEN_SEMICOLON; break;
    case '<':
        switch (peak_char(1)) {
        case '=': eat_cols(2); token = TOKEN_LESS_EQUAL; break;
        case '<': eat_cols(2); token = TOKEN_LESS_LESS; break;
        default:  eat_cols(1); token = TOKEN_LESS_THAN; break;
        }
        break;
    case '=':
        if (peak_char(1) == '=') { eat_cols(2); token = TOKEN_EQUAL_EQUAL; break; }
        eat_cols(1); token = TOKEN_EQUAL; break;
    case '>':
        switch (peak_char(1)) {
        case '=': eat_cols(2); token = TOKEN_GREATER_EQUAL; break;
        case '>': eat_cols(2); token = TOKEN_GREATER_GREATER; break;
        default:  eat_cols(1); token = TOKEN_GREATER_THAN; break;
        }
        break;
    case '[': eat_cols(1); token = TOKEN_LEFT_BRACKET; break;
    case ']': eat_cols(1); token = TOKEN_RIGHT_BRACKET; break;
    case '^': eat_cols(1); token = TOKEN_CARET; break;
    case '{': eat_cols(1); token = TOKEN_LEFT_BRACE; break;
    case '|':
        if (peak_char(1) == '|') { eat_cols(2); token = TOKEN_BAR_BAR; break; }
        eat_cols(1); token = TOKEN_BAR; break;
    case '}': eat_cols(1); token = TOKEN_RIGHT_BRACE; break;
    case '~': eat_cols(1); token = TOKEN_TILDE; break;
    case '_':
    case 'A' ... 'Z':
    case 'a' ... 'z':
        token = TOKEN_IDENT;
        switch (len = peak_ident_len()) {
        case 2:
            if (strncmp(&inbuf[inidx], "if", len) == 0) { eat_cols(len); token = TOKEN_KEYWORD_IF; break; }
            break;
        case 4:
            if (strncmp(&inbuf[inidx], "else", len) == 0) { eat_cols(len); token = TOKEN_KEYWORD_ELSE; break; }
            break;
        case 5:
            if (strncmp(&inbuf[inidx], "while", len) == 0) { eat_cols(len); token = TOKEN_KEYWORD_WHILE; break; }
            if (strncmp(&inbuf[inidx], "break", len) == 0) { eat_cols(len); token = TOKEN_KEYWORD_BREAK; break; }
            break;
        case 6:
            if (strncmp(&inbuf[inidx], "extern", len) == 0) { eat_cols(len); token = TOKEN_KEYWORD_EXTERN; break; }
            if (strncmp(&inbuf[inidx], "return", len) == 0) { eat_cols(len); token = TOKEN_KEYWORD_RETURN; break; }
            break;
        case 8:
            if (strncmp(&inbuf[inidx], "continue", len) == 0) { eat_cols(len); token = TOKEN_KEYWORD_CONTINUE; break; }
            break;
        }
        if (token == TOKEN_IDENT) {
            token_ident = intern_ident(&inbuf[inidx], len);
            eat_cols(len);
        }
        break;
    case '0' ... '9':
        token = TOKEN_LITERAL_INT;
        token_literal_int = 0;
        while ((c = peak_char(0))) {
            switch (c) {
            case '0' ... '9':
                token_literal_int = token_literal_int * 10 + (c - '0');
                eat_cols(1);
                continue;
            }
        }
        break;
    default:
        eprint_str("lex: unrecognized character: ");
        write(stderr, &c, 1);
        eprint_loc(src_col, src_line);
        exit(1);
    }
    set_token_end();
}

// ----------------------------------------------------------------------------

void compile(void) {
    // NOTE: print out tokens for now
    while (1) {
        lex();
        if (token == TOKEN_EOF) break;
        char *str = TOKEN_DISPLAY[token];
        write(stdout, str, strlen(str));
        write(stdout, "\n", 1);
    }
}

// ----------------------------------------------------------------------------

int main(int argc, char **argv) {
    if (argc != 2) {
        write(stderr, "usage: cc FILE\n", sizeof("usage: cc FILE\n") - 1);
        exit(1);
    }
    inpath = argv[1];
    fdin = open(inpath, O_RDONLY);
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
