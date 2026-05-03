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
    END     // Fin de archivo: indica que no hay más tokens
};


// Cada token tiene un tipo y un valor (token de "temp" → {ID, "temp"})
struct Token {
    TokenType type;
    string value;
};

vector<Token> tokenize(string input); //Recibe el texto crudo completo y devuelve la lista de tokens
 
#endif