#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "../../include/scanner.h"
#include "../../include/common.h"


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

    const char* keywords[] = {"and", "break", "continue", "else", "elif", "false", "for",
                              "fn",     "if",    "nil",  "or",    "print",
                              "return", "true",  "let",
                              "while",  "try",   "catch", "i32",   "u32",  "f64",   "bool", "in",
                              "struct", "impl", "import",
                              NULL};
    TokenType types[] = {
        TOKEN_AND, TOKEN_BREAK, TOKEN_CONTINUE, TOKEN_ELSE, TOKEN_ELIF, TOKEN_FALSE, TOKEN_FOR, TOKEN_FN,
        TOKEN_IF, TOKEN_NIL, TOKEN_OR, TOKEN_PRINT, TOKEN_RETURN,
        TOKEN_TRUE, TOKEN_LET, TOKEN_WHILE, TOKEN_TRY, TOKEN_CATCH,
        TOKEN_INT, TOKEN_U32, TOKEN_F64, TOKEN_BOOL, TOKEN_IN,
        TOKEN_STRUCT, TOKEN_IMPL, TOKEN_IMPORT,
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
    scanner.start = source;
    scanner.current = source;
    scanner.source = source; // store the beginning of the entire source
    scanner.line = 1;
    init_keyword_table();
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
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                // Don't skip newlines, they're significant
                return;
            case '/':
                if (peek_next() == '/') {
                    // Single-line comment
                    while (peek() != '\n' && !is_at_end()) {
                        advance();
                    }
                } else if (peek_next() == '*') {
                    // Block comment
                    advance();
                    advance();
                    while (!is_at_end()) {
                        if (peek() == '*' && peek_next() == '/') {
                            advance();
                            advance();
                            break;
                        }
                        if (peek() == '\n') {
                            // Return newline token even in comments
                            return;
                        }
                        advance();
                    }
                    if (is_at_end()) {
                        error_token("Unterminated block comment.");
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
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
    // Process the integer part
    while (is_digit(peek()) || peek() == '_') {
        if (peek() == '_') {
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

    // Number contains underscores, which is allowed

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
        return make_token(TOKEN_NEWLINE);
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
        case '[': return make_token(TOKEN_LEFT_BRACKET);
        case ']': return make_token(TOKEN_RIGHT_BRACKET);
        case ';': return make_token(TOKEN_SEMICOLON);
        case ',': return make_token(TOKEN_COMMA);
        case '.':
            if (peek() == '.') {
                advance();
                return make_token(TOKEN_DOT_DOT);
            }
            return make_token(TOKEN_DOT);
        case '-':
            if (peek() == '>') {
                advance();
                return make_token(TOKEN_ARROW);
            }
            return make_token(TOKEN_MINUS);
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
