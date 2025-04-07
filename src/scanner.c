#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "scanner.h"
#include "common.h"


Scanner scanner;

// Hash table size (should be a prime number for better distribution)
#define HASH_TABLE_SIZE 67

// Hash table for keywords
KeywordEntry keywordTable[HASH_TABLE_SIZE];

// Hash function (djb2 algorithm)
unsigned int hash(const char* str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }
    return hash % HASH_TABLE_SIZE;
}

void init_keyword_table() {
    // First, clear the table
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        keywordTable[i].keyword = NULL;
        keywordTable[i].type = TOKEN_ERROR;
    }

    const char* keywords[] = {"and",    "class", "else", "elif", "false", "for",
                              "fn",     "if",    "nil",  "or",    "print",
                              "return", "super", "this", "true",  "let",
                              "while",  "i32",   "u32",  "f64",   "bool",
                              NULL};
    TokenType types[] = {
        TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_ELIF, TOKEN_FALSE, TOKEN_FOR, TOKEN_FN,
        TOKEN_IF, TOKEN_NIL, TOKEN_OR, TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER,
        TOKEN_THIS, TOKEN_TRUE, TOKEN_LET, TOKEN_WHILE, TOKEN_INT, TOKEN_U32,
        TOKEN_F64, TOKEN_BOOL,
    };

    for (int i = 0; keywords[i] != NULL; i++) {
        unsigned int index = hash(keywords[i]);

        while (keywordTable[index].keyword != NULL) {
            index = (index + 1) % HASH_TABLE_SIZE; // Linear probing for collisions
        }
        keywordTable[index].keyword = keywords[i];
        keywordTable[index].type = types[i];
    }
}

void init_scanner(const char* source) {
    printf("DEBUG: Initializing scanner with source: '%s'\n", source);
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
    init_keyword_table();
    printf("DEBUG: Scanner initialized successfully\n");
}

// Check if a character is alphabetic
static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            c == '_';
}

// Check if a character is a digit
static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

// Check if the scanner has reached the end of the source
static bool is_at_end() {
    return *scanner.current == '\0';
}

// Advance the scanner and return the current character
static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

// Peek at the current character
static char peek() {
    return *scanner.current;
}

// Peek at the next character
static char peek_next() {
    if (is_at_end()) return '\0';
    return scanner.current[1];
}

// Match the current character with an expected character
static bool match(char expected) {
    if (is_at_end()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

// Create a token
static Token make_token(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

// Create an error token
static Token error_token(const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)(strlen(message));
    token.line = scanner.line;
    return token;
}

// Skip whitespace and comments
static void skip_whitespace() {
    printf("DEBUG: Skipping whitespace\n");
    for (;;) {
        char c = peek();
        printf("DEBUG: Checking character '%c' (code %d)\n", c, (int)c);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                printf("DEBUG: Skipping whitespace character\n");
                advance();
                break;
            case '\n':
                // Don't skip newlines, they're significant
                printf("DEBUG: Found newline, returning\n");
                return;
            case '/':
                printf("DEBUG: Found slash, checking for comment\n");
                if (peek_next() == '/') {
                    printf("DEBUG: Found single-line comment\n");
                    // Single-line comment
                    while (peek() != '\n' && !is_at_end()) {
                        printf("DEBUG: Skipping comment character '%c'\n", peek());
                        advance();
                    }
                    printf("DEBUG: End of single-line comment\n");
                } else if (peek_next() == '*') {
                    printf("DEBUG: Found block comment\n");
                    // Block comment
                    advance();
                    advance();
                    while (!is_at_end()) {
                        if (peek() == '*' && peek_next() == '/') {
                            printf("DEBUG: Found end of block comment\n");
                            advance();
                            advance();
                            break;
                        }
                        if (peek() == '\n') {
                            printf("DEBUG: Found newline in block comment\n");
                            // Return newline token even in comments
                            return;
                        }
                        advance();
                    }
                    if (is_at_end()) {
                        printf("DEBUG: Unterminated block comment\n");
                        error_token("Unterminated block comment.");
                    }
                } else {
                    printf("DEBUG: Not a comment, returning\n");
                    return;
                }
                break;
            default:
                printf("DEBUG: Non-whitespace character, returning\n");
                return;
        }
    }
    printf("DEBUG: Finished skipping whitespace\n");
}

static TokenType get_keyword_type(const char* start, int length) {
    char temp[length + 1];
    memcpy(temp, start, length);
    temp[length] = '\0';

    unsigned int index = hash(temp);

    while (keywordTable[index].keyword != NULL) {
        if (strncmp(keywordTable[index].keyword, start, length) == 0 &&
            keywordTable[index].keyword[length] == '\0') {
            return keywordTable[index].type;
        }
        index = (index + 1) % HASH_TABLE_SIZE;
    }

    return TOKEN_IDENTIFIER;
}

static Token identifier() {
    while (is_alpha(peek()) || is_digit(peek())) {
        advance();
    }

    int length = (int)(scanner.current - scanner.start);
    TokenType type = get_keyword_type(scanner.start, length);

    return make_token(type);
}

// Scan a number literal
static Token number() {
    bool has_underscore = false;

    // Process the integer part
    while (is_digit(peek()) || peek() == '_') {
        if (peek() == '_') {
            has_underscore = true;
            advance();
            if (!is_digit(peek())) {
                return error_token("Invalid underscore placement in number.");
            }
        } else {
            advance();
        }
    }

    // Process the fractional part
    if (peek() == '.' && is_digit(peek_next())) {
        advance();  // Consume the dot
        while (is_digit(peek()) || peek() == '_') {
            if (peek() == '_') {
                has_underscore = true;
                advance();
                if (!is_digit(peek())) {
                    return error_token(
                        "Invalid underscore placement in number.");
                }
            } else {
                advance();
            }
        }
    }

    // Process the exponent part
    if (peek() == 'e' || peek() == 'E') {
        advance();  // Consume 'e' or 'E'

        if (peek() == '-' || peek() == '+') {
            advance();  // Consume the sign
        }

        if (!is_digit(peek())) {
            return error_token(
                "Invalid scientific notation: Expected digit after 'e' or "
                "'E'.");
        }

        while (is_digit(peek()) || peek() == '_') {
            if (peek() == '_') {
                has_underscore = true;
                advance();
                if (!is_digit(peek())) {
                    return error_token(
                        "Invalid underscore placement in number.");
                }
            } else {
                advance();
            }
        }
    }

    // ✅ Use has_underscore to enforce a rule or log a message
    if (has_underscore) {
        printf("Info: Number contains underscores.\n");  // Optional logging
    }

    return make_token(TOKEN_NUMBER);
}

// Scan a string literal
static Token string() {
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') scanner.line++;
        if (peek() == '\\') {
            advance();
            switch (peek()) {
                case 'n': advance();  // New line
                    break;
                default:
                    return error_token("Invalid escape sequence.");
            }
        } else {
            advance();
        }
    }

    if (is_at_end()) return error_token("Unterminated string.");
    advance();  // Consume the closing quote
    return make_token(TOKEN_STRING);
}

Token scan_token() {
    skip_whitespace();
    scanner.start = scanner.current;

    if (is_at_end()) {
        return make_token(TOKEN_EOF);
    }

    char c = advance();

    // Handle newline as a token
    if (c == '\n') {
        scanner.line++;
        printf("DEBUG: Scanner found newline at line %d\n", scanner.line);
        Token token = make_token(TOKEN_NEWLINE);
        printf("DEBUG: Created newline token with type %d\n", token.type);
        return token;
    }

    if (is_alpha(c)) {
        Token token = identifier();
        return token;
    }
    if (is_digit(c)) return number();

    switch (c) {
        case '(': return make_token(TOKEN_LEFT_PAREN);
        case ')': return make_token(TOKEN_RIGHT_PAREN);
        case '{': return make_token(TOKEN_LEFT_BRACE);
        case '}': return make_token(TOKEN_RIGHT_BRACE);
        case ';': return make_token(TOKEN_SEMICOLON);
        case ',': return make_token(TOKEN_COMMA);
        case '.': return make_token(TOKEN_DOT);
        case '-': return make_token(TOKEN_MINUS);
        case '+': return make_token(TOKEN_PLUS);
        case '/': return make_token(TOKEN_SLASH);
        case '%': return make_token(TOKEN_MODULO);
        case '*': return make_token(TOKEN_STAR);
        case '!':
            return make_token(
                match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return make_token(
                match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return make_token(
                match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return make_token(
                match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"': return string();
        case ':': return make_token(TOKEN_COLON);
    }

    return error_token("Unexpected character.");
}
