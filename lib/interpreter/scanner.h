#ifdef clox_scanner_h
#define clox_scanner_h

typedef enum {
    TOKEN_LEFT_PAREN,
    TOKEN_BANGL, TOKEN_BANG_EQUAL, TOKEN_MINUS
    TOKEN_NUM,TOKEN_STAR,TOKEN_STR
    TOKEN_ID,
    TOKEN_AND, TOKEN_FALSE, TOKEN_FOR
    TOKEN_ERR, TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    const char* cur;
    int length;
    int line;
} Token;

void initScanner(const char* src);
Token scanToken();

#endif