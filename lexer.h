#ifndef LEXER_H
#define LEXER_H
 
#include <string>
#include <vector>
 
using namespace std;

enum class TokenType {
    // Keywords
    RULE,   // "rule"
    IF,     // "if"
    THEN,   // "then"
    AND,    // "AND"
    STATE,  // "State:"
 
    // Símbolos
    COLON,  // ":"
    GT,     // ">"
    LT,     // "<"
    EQ,     // "="
 
    // Datos
    ID,     // identificador: temp, alert, fan_on, r1...
    NUMBER, // número entero: 30, 35, 20...
 
    // Fin de archivo
    END     // indica que no hay más tokens
};

// ESTRUCTURA DE UN TOKEN
// Cada token tiene un tipo y un valor
// Ejemplo: token de "temp" → {ID, "temp"}
//          token de "30"   → {NUMBER, "30"}
//          token de ">"    → {GT, ">"}

struct Token {
    TokenType type;
    string value;
};

// FUNCIÓN PRINCIPAL DEL LEXER
// Recibe el texto crudo completo
// Devuelve la lista de tokens

vector<Token> tokenize(string input);
 
#endif