#ifndef clox_scanner_h
#define clox_scanner_h

typedef enum {
  // Single-character tokens.
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
  TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
  TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
  TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
  // One or two character tokens.
  TOKEN_BANG_EQUAL,
  TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER, TOKEN_GREATER_EQUAL,
  TOKEN_LESS, TOKEN_LESS_EQUAL, TOKEN_MODULO,
  TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL,
  TOKEN_STAR_EQUAL, TOKEN_SLASH_EQUAL,
  TOKEN_MODULO_EQUAL,
  TOKEN_DOT_DOT, // Range operator

  // function return
  TOKEN_ARROW,

  // Literals.
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
  // Keywords.
  TOKEN_AND, TOKEN_BREAK, TOKEN_CONTINUE, TOKEN_ELSE, TOKEN_ELIF, TOKEN_FALSE,
  TOKEN_FOR, TOKEN_FN, TOKEN_IF, TOKEN_NIL, TOKEN_OR, TOKEN_NOT,
  TOKEN_PRINT, TOKEN_PRINTLN, TOKEN_RETURN,
  TOKEN_TRUE, TOKEN_LET, TOKEN_MUT, TOKEN_WHILE, TOKEN_TRY, TOKEN_CATCH,
  TOKEN_INT, TOKEN_IN, TOKEN_BOOL,
  TOKEN_STRUCT, TOKEN_IMPL, TOKEN_IMPORT, TOKEN_USE, TOKEN_AS,
  TOKEN_MATCH, TOKEN_PUB,

  // Add type tokens
  TOKEN_U32,  // Add this
  TOKEN_F64,  // Add this

  TOKEN_ERROR, TOKEN_EOF,

  TOKEN_NEWLINE,

  TOKEN_COLON,      // Add this for type annotations
  TOKEN_DOUBLE_COLON,
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

typedef struct {
    const char* start;
    const char* current;
    const char* source;  // Pointer to the beginning of the entire source
    int line;
} Scanner;

typedef struct {
    const char* keyword;
    TokenType type;
} KeywordEntry;

void init_scanner(const char* source);
Token scan_token();

// Expose the global scanner instance so other modules (like the parser)
// can access the raw source when producing error messages.
extern Scanner scanner;


#endif