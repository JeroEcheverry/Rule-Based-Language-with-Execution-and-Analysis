#include "parser.h"
#include "grammar.h"
#include <stdexcept>
#include <stack>
#include <iostream>

using namespace std;

// ═══════════════════════════════════════════════════════════════
// SECCIÓN 1 — GRAMÁTICA Y TABLA LL(1)
// ═══════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────
// buildProjectGrammar()
// Construye la gramática ORIGINAL del proyecto
// (con recursión izquierda) y la transforma
// automáticamente a LL(1) usando los algoritmos
// del Dragon Book.
//
// Transformaciones aplicadas:
//   1. Eliminación de recursión izquierda (Sección 4.3)
//      Cond → Cond AND Atom | Atom
//      se convierte en:
//        Cond  → Atom Cond'
//        Cond' → AND Atom Cond' | ε
//
//   2. Left Factoring (Sección 4.3)
//      Atom → id RelOp value | id
//      se convierte en:
//        Atom  → id Atom'
//        Atom' → RelOp value | ε
// ─────────────────────────────────────────────
Grammar buildProjectGrammar() {

    // Gramática original del proyecto con recursión izquierda
    Grammar original;
    original.startSymbol = "Program";

    original.nonTerminals = {
        "Program","RuleList","Rule",
        "Cond","Atom","RelOp","Action"
    };
    original.terminals = {
        "rule","if","then","AND",
        ":","State",">","<","=",
        "id","value","$"
    };

    // Program → RuleList
    original.productions.push_back({"Program",  {"RuleList"}});

    // RuleList → Rule RuleList | ε
    original.productions.push_back({"RuleList", {"Rule","RuleList"}});
    original.productions.push_back({"RuleList", {"eps"}});

    // Rule → rule id : if Cond then Action
    original.productions.push_back({"Rule",
        {"rule","id",":","if","Cond","then","Action"}});

    // Cond → Cond AND Atom | Atom  ← recursión izquierda
    // A=Cond, alpha=AND Atom, beta=Atom
    original.productions.push_back({"Cond", {"Cond","AND","Atom"}});
    original.productions.push_back({"Cond", {"Atom"}});

    // Atom → id RelOp value | id  ← prefijo común
    original.productions.push_back({"Atom", {"id","RelOp","value"}});
    original.productions.push_back({"Atom", {"id"}});

    // RelOp → > | < | =
    original.productions.push_back({"RelOp", {">"}});
    original.productions.push_back({"RelOp", {"<"}});
    original.productions.push_back({"RelOp", {"="}});

    // Action → id
    original.productions.push_back({"Action", {"id"}});

    // Paso 2: Eliminar recursión izquierda automáticamente
    Grammar noRecursion = eliminateLeftRecursion(original);

    // Paso 3: Left Factoring automático
    Grammar transformed = leftFactoring(noRecursion);

    transformed.terminals.insert("State");

    return transformed;
}

// ─────────────────────────────────────────────
// Convierte un token del lexer al string
// que usa la gramática
// ─────────────────────────────────────────────
string tokenToGrammar(const Token& t) {
    switch (t.type) {
        case TokenType::RULE:   return "rule";
        case TokenType::IF:     return "if";
        case TokenType::THEN:   return "then";
        case TokenType::AND:    return "AND";
        case TokenType::STATE:  return "State";
        case TokenType::COLON:  return ":";
        case TokenType::GT:     return ">";
        case TokenType::LT:     return "<";
        case TokenType::EQ:     return "=";
        case TokenType::ID:     return "id";
        case TokenType::NUMBER: return "value";
        case TokenType::END:    return "$";
        default:                return "$";
    }
}

// ═══════════════════════════════════════════════════════════════
// SECCIÓN 2 — ALGORITMO FIGURA 4.20 + PANIC MODE
// Implementación del Dragon Book
// ═══════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────
// validateWithLL1()
//
// Implementa el algoritmo de la Figura 4.20.
// Corre en SILENCIO — no imprime nada.
// Solo retorna true/false para uso interno.
//
// Manejo de errores — Sección 4.4.5 Panic Mode:
//   Heurística 1: si M[X,a] vacío → saltar tokens
//                 hasta encontrar algo en FOLLOW(X)
//   Heurística 5: si terminal X no coincide con a
//                 → pop el terminal (asumir inserción)
// ─────────────────────────────────────────────
bool validateWithLL1(
    vector<Token>& tokens,
    Grammar& grammar,
    map<string, map<string, Production>>& table,
    map<string, set<string>>& follow)
{
    bool hasErrors = false;

    stack<string> stk;
    stk.push("$");
    stk.push(grammar.startSymbol);

    int    pos = 0;
    string a   = tokenToGrammar(tokens[pos]);

    while (stk.top() != "$") {
        string X = stk.top();

        // CASO 1: match
        if (X == a) {
            stk.pop();
            pos++;
            while (pos < (int)tokens.size() &&
                   tokens[pos].type == TokenType::STATE) {
                pos++;
            }
            a = tokenToGrammar(tokens[pos]);
            continue;
        }

        // Llegamos al bloque State → terminar validacion silenciosamente
        if (a == "State" || a == "$") break;

        // CASO 2: terminal que no coincide → Panic Mode heurística 5
        if (grammar.terminals.count(X)) {
            hasErrors = true;
            stk.pop();
            continue;
        }

        // CASO 3: M[X][a] vacío → Panic Mode heurística 1
        if (!table.count(X) || !table.at(X).count(a)) {
            hasErrors = true;
            while (a != "$" && a != "State" &&
                   follow.count(X) && !follow.at(X).count(a)) {
                pos++;
                if (pos < (int)tokens.size())
                    a = tokenToGrammar(tokens[pos]);
                else
                    a = "$";
            }
            stk.pop();
            continue;
        }

        // CASO 4: aplicar producción de la tabla
        Production prod = table.at(X).at(a);
        stk.pop();
        if (prod.rhs[0] != "eps") {
            for (int i = (int)prod.rhs.size()-1; i >= 0; i--) {
                stk.push(prod.rhs[i]);
            }
        }
    }

    return !hasErrors;
}

// ═══════════════════════════════════════════════════════════════
// SECCIÓN 3 — CONSTRUCCIÓN DEL AST (Descenso Recursivo)
// Guiado por la tabla LL(1) como referencia conceptual
// ═══════════════════════════════════════════════════════════════

static vector<Token> tokens;
static int pos;

Token current() {
    return tokens[pos];
}

Token consume(TokenType expected) {
    if (current().type != expected) {
        throw runtime_error(
            "Syntax error near '" + current().value + "'"
        );
    }
    return tokens[pos++];
}

CondNode* parseAtom();
CondNode* parseCondRest(CondNode* left);
CondNode* parseCond();
RuleNode* parseRule();
void      parseRuleList(ProgramNode* program);
void      parseState(map<string,int>& vars, set<string>& facts);

// ─────────────────────────────────────────────
// parseAtom()
// Gramática: Atom → id Atom'
//            Atom' → RelOp value | ε
//
// Tabla:
//   M[Atom][id]    = Atom → id Atom'
//   M[Atom'][>/<==] = Atom' → RelOp value
//   M[Atom'][AND]  = Atom' → ε
//   M[Atom'][then] = Atom' → ε
// ─────────────────────────────────────────────
CondNode* parseAtom() {
    string idName = current().value;
    consume(TokenType::ID);

    TokenType next = current().type;
    if (next == TokenType::GT ||
        next == TokenType::LT ||
        next == TokenType::EQ) {
        string op = current().value;
        pos++;
        int val = stoi(current().value);
        consume(TokenType::NUMBER);
        return new CmpNode(idName, op, val);
    }
    return new FactNode(idName);
}

// ─────────────────────────────────────────────
// parseCondRest()
// Gramática: CondRest → AND Atom CondRest | ε
//
// Tabla:
//   M[CondRest][AND]  = AND Atom CondRest
//   M[CondRest][then] = ε
// ─────────────────────────────────────────────
CondNode* parseCondRest(CondNode* left) {
    if (current().type == TokenType::AND) {
        consume(TokenType::AND);
        CondNode* right = parseAtom();
        AndNode*  node  = new AndNode(left, right);
        return parseCondRest(node);
    }
    return left;
}

// ─────────────────────────────────────────────
// parseCond()
// Gramática: Cond → Atom CondRest
//
// Tabla:
//   M[Cond][id] = Atom CondRest
// ─────────────────────────────────────────────
CondNode* parseCond() {
    CondNode* left = parseAtom();
    return parseCondRest(left);
}

// ─────────────────────────────────────────────
// parseRule()
// Gramática: Rule → rule id : if Cond then Action
//
// Tabla:
//   M[Rule][rule] = rule id : if Cond then Action
// ─────────────────────────────────────────────
RuleNode* parseRule() {
    consume(TokenType::RULE);
    string name = current().value;
    consume(TokenType::ID);
    consume(TokenType::COLON);
    consume(TokenType::IF);
    CondNode* cond = parseCond();
    consume(TokenType::THEN);
    string action = current().value;
    consume(TokenType::ID);
    return new RuleNode(name, cond, action);
}

// ─────────────────────────────────────────────
// parseRuleList()
// Gramática: RuleList → Rule RuleList | ε
//
// Tabla:
//   M[RuleList][rule]  = Rule RuleList
//   M[RuleList][$]     = ε
//   M[RuleList][State] = ε
// ─────────────────────────────────────────────
void parseRuleList(ProgramNode* program) {
    if (current().type == TokenType::RULE) {
        RuleNode* rule = parseRule();
        program->rules.push_back(rule);
        parseRuleList(program);
    }
    // STATE o END → ε
}

// ─────────────────────────────────────────────
// parseState()
// Lee el bloque State: con variables y hechos
// ─────────────────────────────────────────────
void parseState(map<string,int>& variables, set<string>& facts) {
    if (current().type != TokenType::STATE) return;
    consume(TokenType::STATE);

    while (current().type != TokenType::END) {
        string name = current().value;
        consume(TokenType::ID);
        if (current().type == TokenType::EQ) {
            consume(TokenType::EQ);
            int val = stoi(current().value);
            consume(TokenType::NUMBER);
            variables[name] = val;
        } else {
            facts.insert(name);
        }
    }
}

// ═══════════════════════════════════════════════════════════════
// FUNCIÓN PRINCIPAL parse()
//
// Flujo:
//   1. Construir gramática original
//   2. Transformar automáticamente a LL(1)
//      (eliminación de recursión izquierda + left factoring)
//   3. Calcular FIRST y FOLLOW (Dragon Book 4.4.2)
//   4. Construir tabla M[X][a] (Dragon Book 4.4.3)
//   5. Validar con algoritmo Figura 4.20 + Panic Mode 4.4.5
//   6. Construir AST con descenso recursivo
// ═══════════════════════════════════════════════════════════════
ParseResult parse(vector<Token> toks) {

    // Pasos 1-4: gramática → transformación → tabla
    Grammar grammar = buildProjectGrammar();
    auto first  = computeFirst(grammar);
    auto follow = computeFollow(grammar, first);
    bool isLL1  = true;
    auto table  = buildParsingTable(grammar, first, follow, isLL1);

    // Paso 5: validar (silencioso)
    validateWithLL1(toks, grammar, table, follow);

    // Paso 6: construir AST
    tokens = toks;
    pos    = 0;

    ProgramNode*     program = new ProgramNode();
    map<string, int> variables;
    set<string>      facts;

    try {
        parseRuleList(program);
        parseState(variables, facts);
    } catch (runtime_error& e) {
        throw runtime_error(string(e.what()));
    }

    return {program, variables, facts};
}