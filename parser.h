#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include "grammar.h"
#include <map>
#include <set>

using namespace std;

struct ParseResult {
    ProgramNode*     program;   // complete AST with all rules
    map<string, int> variables; // variable assignments from State block (temp=35)
    set<string>      facts;     // initially active facts from State block
};

// Main parser function. Receives tokens from the lexer and returns the AST + initial state.
// Internally: builds grammar → transforms to LL(1) → computes FIRST/FOLLOW →
//             builds table → validates with Figure 4.20 → builds AST
ParseResult parse(vector<Token> tokens);

// Implements the predictive parsing algorithm from Figure 4.20 of the Dragon Book.
// Runs silently — prints nothing, only returns true if input is syntactically valid.
// Uses Panic Mode error recovery (Dragon Book Section 4.4.5).
bool validateWithLL1(
    vector<Token>& tokens,
    Grammar& grammar,
    map<string, map<string, Production>>& table,
    map<string, set<string>>& follow
);

// Builds the project grammar starting from the original form with left recursion,
// then automatically transforms it to LL(1) using eliminateLeftRecursion() and leftFactoring().
Grammar buildProjectGrammar();

#endif