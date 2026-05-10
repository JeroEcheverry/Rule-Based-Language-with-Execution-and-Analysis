#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include "grammar.h"
#include <map>
#include <set>

using namespace std;

// ParseResult agrupa todo lo que el parser produce
struct ParseResult {
    ProgramNode*       program;   // AST con todas las reglas
    map<string, int>   variables; // temp=35, humidity=40
    set<string>        facts;     // hechos activos iniciales
};

// Función principal — recibe tokens, retorna AST + estado
ParseResult parse(vector<Token> tokens);

// Función de validación — Algoritmo Figura 4.20 del Dragon Book
// Retorna true si el input es sintácticamente correcto
// Imprime errores con Panic Mode (sección 4.4.5)
bool validateWithLL1(
    vector<Token>& tokens,
    Grammar& grammar,
    map<string, map<string, Production>>& table,
    map<string, set<string>>& follow
);

// Construye y retorna la gramática LL(1) del proyecto
Grammar buildProjectGrammar();

#endif