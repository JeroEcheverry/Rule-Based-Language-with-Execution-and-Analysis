#include "parser.h"
#include <stdexcept>
#include <stack>

using namespace std;

static vector<Token> tokens;
static int pos;

// Devuelve el token actual sin avanzar
Token current() {
    return tokens[pos];
}

// Devuelve el token n posiciones adelante (para lookahead)
Token lookahead(int n) {
    int idx = pos + n;
    if (idx < (int)tokens.size()) return tokens[idx];
    return {TokenType::END, ""};
}

// Verifica que el token actual sea el esperado y avanza
Token consume(TokenType expected) {
    if (current().type != expected) {
        throw runtime_error(
            "Syntax error: unexpected token '" + current().value + "'"
        );
    }
    return tokens[pos++];
}

// Convierte TokenType a string para usar en la tabla
string tokenToString(TokenType type) {
    switch (type) {
        case TokenType::RULE:   return "rule";
        case TokenType::IF:     return "if";
        case TokenType::THEN:   return "then";
        case TokenType::AND:    return "AND";
        case TokenType::STATE:  return "State";
        case TokenType::COLON:  return "colon";
        case TokenType::GT:     return ">";
        case TokenType::LT:     return "<";
        case TokenType::EQ:     return "=";
        case TokenType::ID:     return "id";
        case TokenType::NUMBER: return "value";
        case TokenType::END:    return "$";
        default:                return "unknown";
    }
}

// ─────────────────────────────────────────────────────────────────
// TABLA DE PARSING PREDICTIVO
// tabla[NoTerminal][terminal] = nombre de la producción a usar
// ─────────────────────────────────────────────────────────────────

map<string, map<string, string>> buildParsingTable() {
    map<string, map<string, string>> table;

    // Program → RuleList
    // FIRST(RuleList) = { rule, ε } → FOLLOW(Program) = { $ }
    table["Program"]["rule"] = "RuleList";
    table["Program"]["$"]    = "RuleList";

    // RuleList → Rule RuleList   cuando el token es "rule"
    // RuleList → ε               cuando el token es en FOLLOW(RuleList) = { $ }
    table["RuleList"]["rule"] = "Rule RuleList";
    table["RuleList"]["$"]    = "eps";
    table["RuleList"]["State"] = "eps"; // State marca fin de reglas

    // Rule → rule id : if Cond then Action
    table["Rule"]["rule"] = "rule id colon if Cond then Action";

    // Cond → Atom CondRest
    // FIRST(Atom) = { id }
    table["Cond"]["id"] = "Atom CondRest";

    // CondRest → AND Atom CondRest   cuando el token es AND
    // CondRest → ε                   cuando el token es en FOLLOW(CondRest) = { then }
    table["CondRest"]["AND"]  = "AND Atom CondRest";
    table["CondRest"]["then"] = "eps";

    // Atom → id RelOp value   o   id
    // Se decide con lookahead: si después del id hay >, < o = → comparación
    // Si no → hecho. Ambas entran en la celda ["Atom"]["id"]
    table["Atom"]["id"] = "id_or_cmp"; // se resuelve con lookahead

    // RelOp → >  |  <  |  =
    table["RelOp"][">"] = ">";
    table["RelOp"]["<"] = "<";
    table["RelOp"]["="] = "=";

    // Action → id
    table["Action"]["id"] = "id";

    return table;
}

// ─────────────────────────────────────────────────────────────────
// FUNCIONES DE CONSTRUCCIÓN DEL AST
// Estas funciones construyen los nodos mientras el parser avanza
// ─────────────────────────────────────────────────────────────────

// Declaraciones adelantadas
CondNode* parseAtom();
CondNode* parseCondRest(CondNode* left);
CondNode* parseCond();
RuleNode* parseRule();
void      parseRuleList(ProgramNode* program);
void      parseState(map<string, int>& variables, set<string>& facts);

// Atom → id RelOp value   |   id
// Usa lookahead para decidir cuál producción aplicar
// Corresponde a la celda tabla["Atom"]["id"] = "id_or_cmp"
CondNode* parseAtom() {
    string idName = current().value;
    consume(TokenType::ID);

    // lookahead: si el siguiente token es >, < o = → es comparación
    TokenType next = current().type;
    if (next == TokenType::GT ||
        next == TokenType::LT ||
        next == TokenType::EQ) {
        // Atom → id RelOp value
        string op = current().value;
        pos++; // consume el operador
        int val = stoi(current().value);
        consume(TokenType::NUMBER);
        return new CmpNode(idName, op, val);
    } else {
        // Atom → id  (hecho simple)
        return new FactNode(idName);
    }
}

// CondRest → AND Atom CondRest | ε
// Recibe el nodo izquierdo acumulado
CondNode* parseCondRest(CondNode* left) {
    // tabla["CondRest"]["AND"] = "AND Atom CondRest"
    if (current().type == TokenType::AND) {
        consume(TokenType::AND);
        CondNode* right = parseAtom();
        AndNode* node = new AndNode(left, right);
        return parseCondRest(node); // puede haber más AND
    }
    // tabla["CondRest"]["then"] = "eps" → no hace nada, retorna lo acumulado
    return left;
}

// Cond → Atom CondRest
// tabla["Cond"]["id"] = "Atom CondRest"
CondNode* parseCond() {
    CondNode* left = parseAtom();
    return parseCondRest(left);
}

// Rule → rule id : if Cond then Action
// tabla["Rule"]["rule"] = "rule id colon if Cond then Action"
RuleNode* parseRule() {
    consume(TokenType::RULE);   // consume "rule"
    string name = current().value;
    consume(TokenType::ID);     // consume el nombre de la regla
    consume(TokenType::COLON);  // consume ":"
    consume(TokenType::IF);     // consume "if"
    CondNode* cond = parseCond(); // parsea la condición completa
    consume(TokenType::THEN);   // consume "then"
    string action = current().value;
    consume(TokenType::ID);     // consume la acción
    return new RuleNode(name, cond, action);
}

// RuleList → Rule RuleList | ε
// tabla["RuleList"]["rule"] = "Rule RuleList"
// tabla["RuleList"]["$"]    = "eps"
void parseRuleList(ProgramNode* program) {
    // tabla["RuleList"]["rule"] → hay una regla, parsearla
    if (current().type == TokenType::RULE) {
        RuleNode* rule = parseRule();
        program->rules.push_back(rule);
        parseRuleList(program); // busca más reglas
    }
    // tabla["RuleList"]["$"] o ["State"] → producción ε, termina
}

// Lee el bloque State: con variables y hechos activos
void parseState(map<string, int>& variables, set<string>& facts) {
    if (current().type != TokenType::STATE) return;
    consume(TokenType::STATE);

    while (current().type != TokenType::END) {
        string name = current().value;
        consume(TokenType::ID);

        if (current().type == TokenType::EQ) {
            // variable: id = número
            consume(TokenType::EQ);
            int val = stoi(current().value);
            consume(TokenType::NUMBER);
            variables[name] = val;
        } else {
            // hecho activo: solo el id
            facts.insert(name);
        }
    }
}

// ─────────────────────────────────────────────────────────────────
// FUNCIÓN PRINCIPAL DEL PARSER
// Recibe los tokens del lexer
// Retorna el AST + estado inicial
// ─────────────────────────────────────────────────────────────────

ParseResult parse(vector<Token> toks) {
    tokens = toks;
    pos    = 0;

    // Construir la tabla de parsing (disponible para consulta)
    map<string, map<string, string>> parsingTable = buildParsingTable();

    // Construir el AST usando la tabla como guía
    ProgramNode* program = new ProgramNode();

    // Program → RuleList
    // tabla["Program"]["rule"] = "RuleList"
    // tabla["Program"]["$"]    = "RuleList"
    string currentToken = tokenToString(current().type);
    if (parsingTable["Program"].count(currentToken)) {
        parseRuleList(program);
    }

    // Leer el estado inicial
    map<string, int> variables;
    set<string>      facts;
    parseState(variables, facts);

    return {program, variables, facts};
}