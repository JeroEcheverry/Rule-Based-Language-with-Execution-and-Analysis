#ifndef PARSER_H
#define PARSER_H
#include "lexer.h"
#include "ast.h"
#include <map>
#include <set>

using namespace std;

struct ParseResult { // ParseResult agrupa todo lo que el parser produce: El AST completo (todas las reglas) y (variables y hechos)
    ProgramNode* program;        // árbol con todas las reglas
    map<string, int> variables;  // las variables
    set<string> facts;           // hechos activos iniciales: ej: {a}
};

ParseResult parse(vector<Token> tokens); // Recibe los tokens del lexer y retorna el AST + estado inicial

#endif