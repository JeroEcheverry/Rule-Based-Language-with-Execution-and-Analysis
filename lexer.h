#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

using namespace std;

// All possible token types in the language alphabet (section 2.2 of the project spec)
enum class TokenType {
    // Keywords — reserved words that cannot be used as identifiers
    RULE,   // "rule"
    IF,     // "if"
    THEN,   // "then"
    AND,    // "AND" — case sensitive
    STATE,  // "State:" or "state:" — marks the beginning of the initial state block

    // Symbols
    COLON,  // ":"
    GT,     // ">"
    LT,     // "<"
    EQ,     // "="

    // Data tokens
    ID,     // identifier: variable name, fact name, or rule name (e.g. temp, alert, r1)
    NUMBER, // integer literal (e.g. 30, 35, 0)

    END     // end of input — signals the parser that there are no more tokens
};

struct Token {
    TokenType type;  // what kind of token this is
    string    value; // the actual text from the input (e.g. "temp", "30", ">")
};

// Reads the raw input string character by character and returns a list of tokens.
// Throws runtime_error if an unrecognized character is found.
vector<Token> tokenize(string input);

#endif