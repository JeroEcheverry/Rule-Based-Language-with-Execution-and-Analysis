//
// Created by Sofía Marín Bustamante on 3/05/26.
//

#ifndef RULE_BASED_LANGUAGE_WITH_EXECUTION_AND_ANALYSIS_INTERPRETER_H
#define RULE_BASED_LANGUAGE_WITH_EXECUTION_AND_ANALYSIS_INTERPRETER_H
#include "ast.h"
#include <map>
#include <set>
#include <string>

using namespace std;

set<string> interpret( // Recibe el AST, las variables y los hechos iniciales
    ProgramNode* program,
    map<string, int> variables,
    set<string> facts // Retorna el conjunto final de hechos activados
);

#endif //RULE_BASED_LANGUAGE_WITH_EXECUTION_AND_ANALYSIS_INTERPRETER_H