package com.lox;

// error handling
public class Lox {
    static boolean hadError = false;
}
static void runFile() {
    run(new String(bytes, Charset.defaultCharset()));
    if (hadError) System.exit(65);
}
static void runPrompt() {
    run(line);
    hadError = false;
}

static void error(int line, String message) {
    report(line, "", message);
}
private static void report(int line, String where, String message) {
    System.err.println(
        "[line " + line + "] Error" + where + ": " + message);
    hadError = true;
}

// token type
enum TokenType {
    // single-character/two char tokens
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,
    BANG, BANG_EQUAL, EQUAL, EQUAL_EQUAL, GREATER, GREATER_EQUAL, LESS, LESS_EQUAL,
    // literals
    IDENTIFIER, STRING, NUMBER,
    // keywords
    AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR, PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,
    EOF
}
class Token {
    final TokenType type;
    final String lexeme;
    final Object literal;
    final int line;
    Token(TokenType type, String lexeme, Object literal, int line) {
        this.type = type;
        this.lexeme = lexeme;
        this.literal = literal;
        this.line = line;
    }
    public String toString() {
        return type + " " + lexeme + " " + literal;
    }
}

// scanner
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import static com.lox.TokenType.*;

class Scanner {
    private final String source;
    private final List<Token> tokens = new ArrayList<>();
    private int start = 0;
    private int current = 0;
    private int line = 1;
    private boolean isAtEnd() {
        return current >= source.length();
    }
    private char advance() {
        return source.charAt(current++);
    }
    private void addToken(TokenType type) {
        addToken(type, null);
    }
    private void addToken(TokenType type, Object literal) {
        String text = source.substring(start, current);
        tokens.add(new Token(type, text, literal, line));
    }
    private boolean match(char expected) {
        if (isAtEnd()) return false;
        if (source.charAt(current) != expected) return false;
        current ++;
        return true;
    }
    private char peek() {
        if (isAtEnd()) return '\0';
        return source.charAt(current);
    }
    private void scanToken() {
        char c = advance();
        switch(c) {
            case '(': addToken(LEFT_PAREN); break;
            case ',': addToken(COMMA); break;
            case '!':
                addToken(match('=') ? BANG_EQUAL : BANG);
                break;
            case '/':
                // the next -> comment
                if (match('/')) {
                    // comment->end of line \n, peek until the end, skip, lookahead
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else {
                    addToken(SLASH);
                }
                break;
            case '\t':
                // whitespace, ignore
                break;
            case '\n':
                line++;
                break;
            default:
                Lox.error(line, "unexpected char");
                break;
        }
    }
    Scanner(String source) {
        this.source = source;
    }
}
List<Token> scanTokens() {
    while (!isAtEnd()) {
        start = current;
        scanToken();
    }
    tokens.add(new Token(EOF, "", null, line));
    return tokens;
}
