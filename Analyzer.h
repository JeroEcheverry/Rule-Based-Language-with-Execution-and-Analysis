#ifndef ANALYZER_H
#define ANALYZER_H

#include "ast.h"
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

struct AnalysisResult {
    map<string, vector<string>> conflicts; // guarda la acción y la lista de reglas que la activan
    vector<vector<string>> redundancies; // grupos de reglas idénticas
    vector<string> inactiveRules; // reglas inactivas
};

AnalysisResult analyze(
    ProgramNode* program,
    map<string, int>& variables,
    set<string>& initialFacts
);

#endif