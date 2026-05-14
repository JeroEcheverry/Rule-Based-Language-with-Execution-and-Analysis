#ifndef ANALYZER_H
#define ANALYZER_H

#include "ast.h"
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

struct AnalysisResult {
    map<string, vector<string>> conflicts;  // action → list of rules that activate it
    vector<vector<string>>      redundancies; // groups of identical rules
    vector<string>              inactiveRules; // rules that can never fire
};

// Analyzes the AST without executing it.
// Detects conflicts, redundancies, and potentially inactive rules.
AnalysisResult analyze(
    ProgramNode* program,
    map<string, int>& variables,
    set<string>& initialFacts
);

#endif