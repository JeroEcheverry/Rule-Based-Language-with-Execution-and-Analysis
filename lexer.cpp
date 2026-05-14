#include "lexer.h"
#include <cctype>
#include <stdexcept>

// tokenize()
// WHAT IT DOES:
//   Performs lexical analysis on the raw input string.
//   Reads the text character by character and groups characters
//   into tokens — the smallest meaningful units of the language.

vector<Token> tokenize(string input) {
    vector<Token> tokens; // list of tokens being built
    int i = 0;            // current position in the input string

    while (i < input.size()) {

        // Skip whitespace (spaces, newlines, tabs)
        // Whitespace has no meaning in this language
        if (isspace(input[i])) {
            i++;
            continue;
        }

        // Letter: could be a keyword or an identifier
        if (isalpha(input[i])) {
            string word = "";

            // Keep reading while the character is a letter, digit, or underscore
            // This handles identifiers like: temp, fan_on, r1, humidity_level
            while (i < input.size() && (isalnum(input[i]) || input[i] == '_')) {
                word += input[i];
                i++;
            }

            // Check if the word is a reserved keyword
            // Keywords are case-sensitive as defined in the project spec (section 2.3)
            if      (word == "rule") tokens.push_back({TokenType::RULE,  word});
            else if (word == "if")   tokens.push_back({TokenType::IF,    word});
            else if (word == "then") tokens.push_back({TokenType::THEN,  word});
            else if (word == "AND")  tokens.push_back({TokenType::AND,   word});
            else if (word == "State" || word == "state") {
                // "State" marks the beginning of the initial state block.
                // It is always followed by ":" which we consume here
                // so the parser does not need to handle it separately.
                while (i < input.size() && input[i] == ':') i++;
                tokens.push_back({TokenType::STATE, "State"});
            }
            else {
                // Not a keyword → it is an identifier
                // Identifiers represent: variable names, fact names, rule names
                tokens.push_back({TokenType::ID, word});
            }

            continue;
        }

        //Digit: integer literal
        if (isdigit(input[i])) {
            string num = "";

            // Keep reading while the character is a digit
            // Only base-10 non-negative integers are supported (section 2.3)
            while (i < input.size() && isdigit(input[i])) {
                num += input[i];
                i++;
            }

            tokens.push_back({TokenType::NUMBER, num});
            continue;
        }

        //Single-character symbols
        // Each symbol maps directly to its token type
        if (input[i] == ':') { tokens.push_back({TokenType::COLON, ":"}); i++; continue; }
        if (input[i] == '>') { tokens.push_back({TokenType::GT,    ">"}); i++; continue; }
        if (input[i] == '<') { tokens.push_back({TokenType::LT,    "<"}); i++; continue; }
        if (input[i] == '=') { tokens.push_back({TokenType::EQ,    "="}); i++; continue; }

        //Unknown character → lexical error
        // The character is not part of the language alphabet
        throw runtime_error("Unrecognized character: " + string(1, input[i]));
    }

    // Mark the end of the token list
    // The parser uses this to know when the input is exhausted
    tokens.push_back({TokenType::END, ""});

    return tokens;
}