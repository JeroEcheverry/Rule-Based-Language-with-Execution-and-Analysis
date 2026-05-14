#ifndef RULE_BASED_LANGUAGE_WITH_EXECUTION_AND_ANALYSIS_INTERPRETER_H
#define RULE_BASED_LANGUAGE_WITH_EXECUTION_AND_ANALYSIS_INTERPRETER_H

#include "ast.h"
#include <map>
#include <set>
#include <string>

using namespace std;

// Executes the program using fixed-point evaluation.
// Repeatedly applies all rules until no new facts are produced.
// Returns only the facts activated by rules, not the initial ones.
set<string> interpret(
    ProgramNode* program,    // complete AST built by the parser
    map<string, int> variables, // initial variable assignments (temp=35)
    set<string> facts           // initially active facts from State block
);

#endif