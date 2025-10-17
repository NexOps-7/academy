#ifdef clox_scanner_h
#define clox_scanner_h

typedef enum {
    TOKEN_LEFT_PAREN,
    TOKEN_BANG_EQUAL, TOKEN_BANG,
    TOKEN_NUMBER,
    TOKEN_AND,
    TOKEN_ERROR, TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

void initScanner(const char* src);
Token scanToken();

#endif