#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../include/scanner.h"
#include "../../include/common.h"

/**
 * @file scanner.c
 * @brief Lexical scanner for the Orus language.
 *
 * The scanner converts raw source code into a stream of tokens that the
 * parser can consume. It recognises keywords, literals and punctuation while
 * tracking line information for diagnostics.
 */

Scanner scanner;

// Hash table size (should be a prime number for better distribution)
#define HASH_TABLE_SIZE 67

// Hash table for keywords
KeywordEntry keywordTable[HASH_TABLE_SIZE];

/**
 * Simple hash function used for the keyword table.
 *
 * @param str Null terminated string to hash.
 * @return Hash value suitable for indexing the keyword table.
 */
unsigned int hash(const char* str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }
    return hash % HASH_TABLE_SIZE;
}

/**
 * Populate the keyword lookup table used by the scanner.
 *
 * This table maps language keywords to their corresponding token types and is
 * generated once when the scanner is initialised.
 */
void init_keyword_table() {
    // First, clear the table
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        keywordTable[i].keyword = NULL;
        keywordTable[i].type = TOKEN_ERROR;
    }

    const char* keywords[] = {"and",   "break",  "continue", "else",  "elif", "false", "for",
                              "fn",    "if",     "nil",      "or",    "not",  "print",
                              "return", "true",   "let", "mut", "const",
                              "while", "try",    "catch",    "i32", "i64",  "u32", "u64",  "f64",  "bool", "in",
                              "struct", "impl",   "import",   "use",   "as",   "match", "pub", "static", "enum",
                              NULL};
    TokenType types[] = {
        TOKEN_AND,  TOKEN_BREAK, TOKEN_CONTINUE, TOKEN_ELSE, TOKEN_ELIF, TOKEN_FALSE, TOKEN_FOR, TOKEN_FN,
        TOKEN_IF,   TOKEN_NIL,   TOKEN_OR,       TOKEN_NOT,  TOKEN_PRINT, TOKEN_RETURN,
        TOKEN_TRUE, TOKEN_LET,   TOKEN_MUT, TOKEN_CONST, TOKEN_WHILE,    TOKEN_TRY,  TOKEN_CATCH,
        TOKEN_INT, TOKEN_I64, TOKEN_U32, TOKEN_U64,   TOKEN_F64,      TOKEN_BOOL, TOKEN_IN,
        TOKEN_STRUCT, TOKEN_IMPL, TOKEN_IMPORT, TOKEN_USE, TOKEN_AS, TOKEN_MATCH, TOKEN_PUB, TOKEN_STATIC, TOKEN_ENUM,
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

/**
 * Initialise the global scanner state for a new source buffer.
 *
 * @param source Pointer to the null terminated string containing the program
 *               source code.
 */
void init_scanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.source = source; // store the beginning of the entire source
    scanner.line = 1;
    init_keyword_table();
}

/**
 * Determine whether a character can start or continue an identifier.
 *
 * @param c Character to test.
 * @return true if the character is alphabetic or an underscore.
 */
static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            c == '_';
}

/**
 * Test if a character is a decimal digit.
 *
 * @param c Character to examine.
 * @return true when the character is '0'..'9'.
 */
static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

/**
 * Test if a character is valid in a hexadecimal literal.
 *
 * @param c Character to examine.
 * @return true when the character is 0-9, a-f or A-F.
 */
static bool is_hex_digit(char c) {
    return (c >= '0' && c <= '9') ||
           (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

/**
 * Determine if the scanner has consumed all input.
 *
 * @return true when the current position is at the terminating null byte.
 */
static bool is_at_end() {
    return *scanner.current == '\0';
}

/**
 * Consume the next character from the source.
 *
 * @return The consumed character.
 */
static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

/**
 * Peek at the current character without consuming it.
 */
static char peek() {
    return *scanner.current;
}

/**
 * Look ahead one character without advancing.
 */
static char peek_next() {
    if (is_at_end()) return '\0';
    return scanner.current[1];
}

/**
 * Conditionally consume a specific character.
 *
 * @param expected Character to match.
 * @return true if the character was consumed.
 */
static bool match(char expected) {
    if (is_at_end()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

/**
 * Construct a token object from the current lexeme.
 *
 * @param type Token type to assign.
 */
static Token make_token(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

/**
 * Create a token representing a lexical error.
 *
 * @param message Static error message.
 */
static Token error_token(const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)(strlen(message));
    token.line = scanner.line;
    return token;
}

/**
 * Consume whitespace and comments, stopping at newlines.
 */
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

/**
 * Look up a token type for a potential keyword.
 *
 * @param start  Pointer to the start of the identifier.
 * @param length Length of the identifier.
 * @return Keyword token type or TOKEN_IDENTIFIER.
 */
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

/**
 * Scan an identifier or keyword token.
 *
 * @return The constructed token with appropriate type.
 */
static Token identifier() {
    while (is_alpha(peek()) || is_digit(peek())) {
        advance();
    }

    int length = (int)(scanner.current - scanner.start);
    TokenType type = get_keyword_type(scanner.start, length);

    return make_token(type);
}

// Scan a number literal
/**
 * Scan an integer or floating point literal.
 *
 * Handles hexadecimal prefixes, underscores and optional unsigned suffixes.
 * @return Token for the numeric literal.
 */
static Token number() {
    // Check for hexadecimal prefix 0x or 0X
    if (scanner.start[0] == '0' && (peek() == 'x' || peek() == 'X')) {
        advance(); // consume 'x' or 'X'
        if (!is_hex_digit(peek())) {
            return error_token("Invalid hexadecimal literal.");
        }
        while (is_hex_digit(peek()) || peek() == '_') {
            if (peek() == '_') {
                advance();
                if (!is_hex_digit(peek())) {
                    return error_token(
                        "Invalid underscore placement in number.");
                }
            } else {
                advance();
            }
        }
        // Handle type suffixes for hexadecimal numbers too
        if (peek() == 'u' || peek() == 'U') {
            advance();
            // Check for specific unsigned suffixes (u32, u64)
            if (peek() == '3' && peek_next() == '2') {
                advance(); // '3'
                advance(); // '2'
            } else if (peek() == '6' && peek_next() == '4') {
                advance(); // '6'
                advance(); // '4'
            }
        } else if (peek() == 'i') {
            advance(); // 'i'
            // Check for specific signed suffixes (i32, i64)
            if (peek() == '3' && peek_next() == '2') {
                advance(); // '3'
                advance(); // '2'
            } else if (peek() == '6' && peek_next() == '4') {
                advance(); // '6'
                advance(); // '4'
            } else {
                // Just 'i' is not a valid suffix, back up
                scanner.current--;
            }
        }
        return make_token(TOKEN_NUMBER);
    }

    // Process the integer part for decimal numbers
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

    // Optional type suffix (u, i32, i64, u32, u64, f64)
    if (peek() == 'u' || peek() == 'U') {
        advance();
        // Check for specific unsigned suffixes (u32, u64)
        if (peek() == '3' && peek_next() == '2') {
            advance(); // '3'
            advance(); // '2'
        } else if (peek() == '6' && peek_next() == '4') {
            advance(); // '6'
            advance(); // '4'
        }
    } else if (peek() == 'i') {
        advance(); // 'i'
        // Check for specific signed suffixes (i32, i64)
        if (peek() == '3' && peek_next() == '2') {
            advance(); // '3'
            advance(); // '2'
        } else if (peek() == '6' && peek_next() == '4') {
            advance(); // '6'
            advance(); // '4'
        } else {
            // Just 'i' is not a valid suffix, back up
            scanner.current--;
        }
    } else if (peek() == 'f' && peek_next() == '6') {
        // Check if we have enough characters left and the third character is '4'
        if (!is_at_end() && scanner.current[1] != '\0' && scanner.current[2] == '4') {
            advance(); // 'f'
            advance(); // '6'
            advance(); // '4'
        }
    }

    return make_token(TOKEN_NUMBER);
}

// Scan a string literal
/**
 * Scan a quoted string literal handling escape sequences.
 *
 * @return Token for the parsed string.
 */
static Token string() {
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') scanner.line++;
        if (peek() == '\\') {
            advance();
            switch (peek()) {
                case 'n':
                case 't':
                case '\\':
                case '"':
                    advance();
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

/**
 * Retrieve the next lexical token from the input stream.
 *
 * @return The next token describing the lexeme.
 */
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
        case '?':
            return make_token(TOKEN_QUESTION);
        case '-':
            if (peek() == '>') {
                advance();
                return make_token(TOKEN_ARROW);
            }
            if (match('=')) return make_token(TOKEN_MINUS_EQUAL);
            return make_token(TOKEN_MINUS);
        case '+':
            if (match('=')) return make_token(TOKEN_PLUS_EQUAL);
            return make_token(TOKEN_PLUS);
        case '/':
            if (match('=')) return make_token(TOKEN_SLASH_EQUAL);
            return make_token(TOKEN_SLASH);
        case '%':
            if (match('=')) return make_token(TOKEN_MODULO_EQUAL);
            return make_token(TOKEN_MODULO);
        case '*':
            if (match('=')) return make_token(TOKEN_STAR_EQUAL);
            return make_token(TOKEN_STAR);
        case '!':
            if (match('=')) {
                return make_token(TOKEN_BANG_EQUAL);
            }
            return make_token(TOKEN_BIT_NOT);
        case '=':
            return make_token(
                match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            if (match('<')) return make_token(TOKEN_SHIFT_LEFT);
            return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            if (peek() == '>' && peek_next() != '{' && peek_next() != '>') {
                advance();
                return make_token(TOKEN_SHIFT_RIGHT);
            }
            return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '&':
            return make_token(TOKEN_BIT_AND);
        case '|':
            return make_token(TOKEN_BIT_OR);
        case '^':
            return make_token(TOKEN_BIT_XOR);
        case '"': return string();
        case ':':
            if (match(':')) return make_token(TOKEN_DOUBLE_COLON);
            return make_token(TOKEN_COLON);
    }

    return error_token("Unexpected character.");
}
