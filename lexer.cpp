#include "lexer.h"
#include <cctype>
#include <stdexcept>

// tokenize()
// Recorre el texto crudo carácter por carácter
// y agrupa los caracteres en tokens

vector<Token> tokenize(string input) {
    vector<Token> tokens; // lista de tokens que vamos construyendo
    int i = 0;            // posición actual en el texto
 
    while (i < input.size()) {
 
        // ── Ignorar espacios, saltos de línea y tabulaciones ──
        if (isspace(input[i])) {
            i++;
            continue;
        }
 
        // ── Letra: puede ser keyword o identificador ──
        if (isalpha(input[i])) {
            string word = "";
 
            // seguir leyendo mientras sea letra, dígito o guion bajo
            while (i < input.size() && (isalnum(input[i]) || input[i] == '_')) {
                word += input[i];
                i++;
            }
 
            // verificar si la palabra es un keyword
            if      (word == "rule") tokens.push_back({TokenType::RULE,  word});
            else if (word == "if")   tokens.push_back({TokenType::IF,    word});
            else if (word == "then") tokens.push_back({TokenType::THEN,  word});
            else if (word == "AND")  tokens.push_back({TokenType::AND,   word});
            else if (word == "State") {
                // "State" va seguido de ":" → lo tratamos como un token especial
                // que marca el inicio del estado inicial
                while (i < input.size() && input[i] == ':') i++; // consumir el ":"
                tokens.push_back({TokenType::STATE, "State"});
            }
            else {
                // no es keyword → es un identificador (variable, hecho, nombre de regla)
                tokens.push_back({TokenType::ID, word});
            }
 
            continue;
        }
 
        // ── Dígito: es un número entero ──
        if (isdigit(input[i])) {
            string num = "";
 
            // seguir leyendo mientras sea dígito
            while (i < input.size() && isdigit(input[i])) {
                num += input[i];
                i++;
            }
 
            tokens.push_back({TokenType::NUMBER, num});
            continue;
        }
 
        // ── Símbolos de un solo carácter ──
        if (input[i] == ':') { tokens.push_back({TokenType::COLON, ":"}); i++; continue; }
        if (input[i] == '>') { tokens.push_back({TokenType::GT,    ">"}); i++; continue; }
        if (input[i] == '<') { tokens.push_back({TokenType::LT,    "<"}); i++; continue; }
        if (input[i] == '=') { tokens.push_back({TokenType::EQ,    "="}); i++; continue; }
 
        // ── Carácter desconocido → error léxico ──
        throw runtime_error("Caracter no reconocido: " + string(1, input[i]));
    }
 
    // marcar el fin de la lista de tokens
    tokens.push_back({TokenType::END, ""});
 
    return tokens;
}