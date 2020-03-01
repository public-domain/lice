#ifndef LICE_LEXER_HDR
#define LICE_LEXER_HDR
/*
 * File: lexer.h
 *  Implements the interface for LICE's lexer
 */
#include <stdbool.h>

/*
 * Type: lexer_token_type_t
 *  Type to describe a tokens type.
 *
 *  Remarks:
 *   Implemented as a typedef of an enumeration, lexer_token_t
 *   is used to describe the current lexer token. The following
 *   tokens exist (as constants).
 *
 *  Tokens:
 *    LEXER_TOKEN_IDENTIFIER        - Identifier
 *    LEXER_TOKEN_PUNCT             - Language punctuation
 *    LEXER_TOKEN_CHAR              - Character literal
 *    LEXER_TOKEN_STRING            - String literal
 *    LEXER_TOKEN_NUMBER            - Number (of any type)
 *    LEXER_TOKEN_EQUAL             - Equal
 *    LEXER_TOKEN_LEQUAL            - Lesser-or-equal
 *    LEXER_TOKEN_GEQUAL            - Greater-or-equal
 *    LEXER_TOKEN_NEQUAL            - Not-equal
 *    LEXER_TOKEN_INCREMENT         - Pre/post increment
 *    LEXER_TOKEN_DECREMENT         - Pre/post decrement
 *    LEXER_TOKEN_ARROW             - Pointer arrow `->`
 *    LEXER_TOKEN_LSHIFT            - Left shift
 *    LEXER_TOKEN_RSHIFT            - Right shift
 *    LEXER_TOKEN_COMPOUND_ADD      - Compound-assignment addition
 *    LEXER_TOKEN_COMPOUND_SUB      - Compound-assignment subtraction
 *    LEXER_TOKEN_COMPOUND_MUL      - Compound-assignment multiplication
 *    LEXER_TOKEN_COMPOUND_DIV      - Compound-assignment division
 *    LEXER_TOKEN_COMPOUND_MOD      - Compound-assignment moduluas
 *    LEXER_TOKEN_COMPOUND_OR       - Compound-assignment bit-or
 *    LEXER_TOKEN_COMPOUND_XOR      - Compound-assignment bit-xor
 *    LEXER_TOKEN_COMPOUND_LSHIFT   - Compound-assignment left-shift
 *    LEXER_TOKEN_COMPOUND_RSHIFT   - Compound-assignment right-shift
 *    LEXER_TOKEN_AND               - Logical and
 *    LEXER_TOKEN_OR                - Logical or
 */
typedef enum {
    LEXER_TOKEN_IDENTIFIER,
    LEXER_TOKEN_PUNCT,
    LEXER_TOKEN_CHAR,
    LEXER_TOKEN_STRING,
    LEXER_TOKEN_NUMBER,
    LEXER_TOKEN_EQUAL,
    LEXER_TOKEN_LEQUAL,
    LEXER_TOKEN_GEQUAL,
    LEXER_TOKEN_NEQUAL,
    LEXER_TOKEN_INCREMENT,
    LEXER_TOKEN_DECREMENT,
    LEXER_TOKEN_ARROW,
    LEXER_TOKEN_LSHIFT,
    LEXER_TOKEN_RSHIFT,
    LEXER_TOKEN_COMPOUND_ADD,
    LEXER_TOKEN_COMPOUND_SUB,
    LEXER_TOKEN_COMPOUND_MUL,
    LEXER_TOKEN_COMPOUND_DIV,
    LEXER_TOKEN_COMPOUND_MOD,
    LEXER_TOKEN_COMPOUND_AND,
    LEXER_TOKEN_COMPOUND_OR,
    LEXER_TOKEN_COMPOUND_XOR,
    LEXER_TOKEN_COMPOUND_LSHIFT,
    LEXER_TOKEN_COMPOUND_RSHIFT,
    LEXER_TOKEN_AND,
    LEXER_TOKEN_OR
} lexer_token_type_t;

/*
 * Class: lexer_token_t
 *  Describes a token in the token stream
 */
typedef struct {
    /*
     * Variable: type
     *  The token type
     */
    lexer_token_type_t type;

    union {
        long  integer;
        int   punct;
        char *string;
        char  character;
    };
} lexer_token_t;

/*
 * Function: lexer_islong
 *  Checks for a given string if it's a long-integer-literal.
 *
 * Parameters:
 *  string  - The string to check
 *
 * Remarks:
 *  Returns `true` if the string is a long-literal,
 *  `false` otherwise.
 */
bool lexer_islong(char *string);

/*
 * Function: lexer_isint
 *  Checks for a given string if it's a int-integer-literal.
 *
 * Parameters:
 *  string  - The string to check
 *
 * Remarks:
 *  Returns `true` if the string is a int-literal,
 * `false` otherwise.
 */
bool lexer_isint (char *string);

/*
 * Function: lexer_isfloat
 *  Checks for a given string if it's a floating-point-literal.
 *
 * Parameters:
 *  string  - The string to check
 *
 * Remarks:
 *  Returns `true` if the string is floating-point-literal,
 * `false` otherwise.
 */
bool lexer_isfloat(char *string);

/*
 * Function: lexer_ispunct
 *  Checks if a given token is language punctuation and matches.
 *
 * Parameters:
 *  token   - The token to test
 *  c       - The punction to test if matches
 *
 * Remarks:
 *  Returns `true` if the given token is language punctuation and
 *  matches *c*.
 */
bool lexer_ispunct(lexer_token_t *token, int c);

/*
 * Function: lexer_unget
 *  Undo the given token in the token stream.
 *
 * Parameters:
 *  token   - The token to unget
 */
void lexer_unget(lexer_token_t *token);

/*
 * Function: lexer_next
 *  Get the next token in the token stream.
 *
 * Returns:
 *  The next token in the token stream or NULL
 *  on failure or EOF.
 */
lexer_token_t *lexer_next(void);

/*
 * Function: lexer_peek
 *  Look at the next token without advancing the stream.
 *
 * Returns:
 *  The next token without advancing the token stream or NULL on failure
 *  or EOF.
 *
 * Remarks:
 *  The function will peek ahead to see the next token in the stream
 *  without advancing the lexer state.
 */
lexer_token_t *lexer_peek(void);

/*
 * Function: lexer_token_string
 *  Convert a token to a human-readable representation
 *
 * Parameters:
 *  token   - The token to convert
 *
 * Returns:
 *  A string representation of the token or NULL on failure.
 */
char *lexer_token_string(lexer_token_t *token);

/*
 * Function: lexer_marker
 *  Get the line marker of where the lexer currently is.
 *
 * Remarks:
 *  Currently returns file.c:line, will later be extended to also include
 *  column marker. This is used in error reporting.
 */
char *lexer_marker(void);

#endif
