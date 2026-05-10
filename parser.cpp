#include "parser.h"
#include "grammar.h"
#include <stdexcept>
#include <stack>
#include <iostream>

using namespace std;

// ═══════════════════════════════════════════════════════════════
// SECCIÓN 1 — GRAMÁTICA Y TABLA LL(1)
// Todo lo teórico que pide el profesor explícitamente
// ═══════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────
// Gramática LL(1) transformada del proyecto
// Ya tiene aplicadas:
//   1. Eliminación de recursión izquierda:
//      Cond → Cond AND Cond  se convierte en
//      Cond → Atom CondRest / CondRest → AND Atom CondRest | ε
//   2. Left Factoring:
//      Atom → id RelOp value | id  se convierte en
//      Atom → id Atom' / Atom' → RelOp value | ε
// ─────────────────────────────────────────────
Grammar buildProjectGrammar() {
    Grammar g;
    g.startSymbol = "Program";

    g.nonTerminals = {
        "Program","RuleList","Rule",
        "Cond","CondRest",
        "Atom","Atom'",
        "RelOp","Action"
    };
    g.terminals = {
        "rule","if","then","AND",
        ":","State",
        ">","<","=",
        "id","value","$"
    };

    // Program → RuleList
    g.productions.push_back({"Program", {"RuleList"}});

    // RuleList → Rule RuleList | ε
    g.productions.push_back({"RuleList", {"Rule","RuleList"}});
    g.productions.push_back({"RuleList", {"eps"}});

    // Rule → rule id : if Cond then Action
    g.productions.push_back({"Rule",
        {"rule","id",":","if","Cond","then","Action"}});

    // Cond → Atom CondRest
    g.productions.push_back({"Cond", {"Atom","CondRest"}});

    // CondRest → AND Atom CondRest | ε
    g.productions.push_back({"CondRest", {"AND","Atom","CondRest"}});
    g.productions.push_back({"CondRest", {"eps"}});

    // Atom → id Atom'
    g.productions.push_back({"Atom", {"id","Atom'"}});

    // Atom' → RelOp value | ε
    g.productions.push_back({"Atom'", {"RelOp","value"}});
    g.productions.push_back({"Atom'", {"eps"}});

    // RelOp → > | < | =
    g.productions.push_back({"RelOp", {">"}});
    g.productions.push_back({"RelOp", {"<"}});
    g.productions.push_back({"RelOp", {"="}});

    // Action → id
    g.productions.push_back({"Action", {"id"}});

    return g;
}

// ─────────────────────────────────────────────
// Convierte un token del lexer al string
// que usa la gramática — necesario para
// consultar la tabla M[X][a]
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
// Implementación EXACTA del Dragon Book
// ═══════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────
// validateWithLL1()
//
// Implementa el algoritmo de la Figura 4.20:
//
//   let a = primer símbolo del input
//   let X = tope de la pila
//   while (X ≠ $) {
//     if (X == a) pop y avanzar
//     else if (X es terminal) error()       ← Panic Mode heurística 5
//     else if (M[X,a] es vacío) error()     ← Panic Mode heurística 1
//     else {                                ← aplicar producción
//       pop X
//       push Y_k ... Y_1
//     }
//     X = tope de la pila
//   }
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

    // Inicialización — pila con $ y símbolo inicial
    stack<string> stk;
    stk.push("$");
    stk.push(grammar.startSymbol); // "Program" en el tope

    int    pos = 0;
    string a   = tokenToGrammar(tokens[pos]); // primer símbolo

    // Saltamos tokens STATE al validar las reglas
    // (State: es leído por parseState separadamente)
    // Cuando llegamos a STATE en la validación, paramos
    // las reglas y dejamos el resto para parseState

    while (stk.top() != "$") {
        string X = stk.top();

        // ── CASO 1: X == a → MATCH ──────────────
        // El tope es un terminal que coincide — avanzar
        if (X == a) {
            stk.pop();
            pos++;
            // Saltar tokens STATE — no son parte de la gramática de reglas
            while (pos < (int)tokens.size() &&
                   tokens[pos].type == TokenType::STATE) {
                pos++;
            }
            a = tokenToGrammar(tokens[pos]);
            continue;
        }

        // Llegamos al bloque State → terminar validación de reglas
        if (a == "State") {
            // El resto (State y sus tokens) lo maneja parseState
            break;
        }

        // ── CASO 2: X es terminal ≠ a → ERROR ───
        // Panic Mode, Heurística 5 del libro:
        // "pop el terminal — asumir que fue insertado"
        if (grammar.terminals.count(X)) {
            cerr << "[Syntax Error] Expected '" << X
                 << "' but found '"
                 << tokens[pos].value << "'" << endl;
            hasErrors = true;
            stk.pop(); // pop del terminal — asumir inserción
            continue;
        }

        // ── CASO 3: M[X][a] vacío → ERROR ───────
        // Panic Mode, Heurística 1 del libro:
        // "saltar tokens hasta FOLLOW(X)"
        if (!table.count(X) || !table.at(X).count(a)) {
            cerr << "[Syntax Error] No rule for ["
                 << X << "][" << a << "]"
                 << " — skipping token '"
                 << tokens[pos].value << "'" << endl;
            hasErrors = true;

            // Saltar tokens hasta encontrar algo en FOLLOW(X)
            // o hasta llegar a $ — synchronizing tokens
            while (a != "$" && a != "State" &&
                   !follow.at(X).count(a)) {
                pos++;
                a = tokenToGrammar(tokens[pos]);
            }

            // Si encontramos FOLLOW(X) → pop X y continuar
            // Si llegamos a $ → pop X también para no quedar en loop
            stk.pop();
            continue;
        }

        // ── CASO 4: M[X][a] = X → Y1 Y2...Yk ───
        // Aplicar producción de la tabla
        Production prod = table.at(X).at(a);
        stk.pop();

        // Push en orden INVERSO (Y1 queda en el tope)
        if (prod.rhs[0] != "eps") {
            for (int i = (int)prod.rhs.size()-1; i >= 0; i--) {
                stk.push(prod.rhs[i]);
            }
        }
        // Si es ε → no pushear nada
    }

    return !hasErrors;
}

// ═══════════════════════════════════════════════════════════════
// SECCIÓN 3 — CONSTRUCCIÓN DEL AST (Descenso Recursivo)
// Guiado por la misma tabla LL(1) como referencia conceptual
// Construye los nodos que el intérprete necesita
// ═══════════════════════════════════════════════════════════════

static vector<Token> tokens; // tokens del lexer
static int pos;              // índice actual

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

// Declaraciones adelantadas
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
// La tabla dice:
//   M[Atom][id]   = Atom → id Atom'
//   M[Atom'][>]   = Atom' → RelOp value
//   M[Atom'][<]   = Atom' → RelOp value
//   M[Atom'][=]   = Atom' → RelOp value
//   M[Atom'][AND] = Atom' → ε
//   M[Atom'][then]= Atom' → ε
// ─────────────────────────────────────────────
CondNode* parseAtom() {
    // Atom → id Atom'
    string idName = current().value;
    consume(TokenType::ID);

    // Atom' → RelOp value | ε
    // M[Atom'][>/</ =] = RelOp value → comparación
    // M[Atom'][AND/then] = ε → hecho simple
    TokenType next = current().type;
    if (next == TokenType::GT ||
        next == TokenType::LT ||
        next == TokenType::EQ) {
        string op = current().value;
        pos++; // consume el operador
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
// La tabla dice:
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
    return left; // ε
}

// ─────────────────────────────────────────────
// parseCond()
// Gramática: Cond → Atom CondRest
//
// La tabla dice:
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
// La tabla dice:
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
// La tabla dice:
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
//   1. Construir gramática LL(1)
//   2. Calcular FIRST y FOLLOW (Dragon Book 4.4.2)
//   3. Construir tabla M[X][a]  (Dragon Book 4.4.3)
//   4. Validar con algoritmo Figura 4.20 + Panic Mode 4.4.5
//   5. Si válido → construir AST con descenso recursivo
// ═══════════════════════════════════════════════════════════════
ParseResult parse(vector<Token> toks) {

    // ── Paso 1: Gramática LL(1) ─────────────
    Grammar grammar = buildProjectGrammar();

    // ── Paso 2: FIRST y FOLLOW ──────────────
    auto first  = computeFirst(grammar);
    auto follow = computeFollow(grammar, first);

    // ── Paso 3: Tabla M[X][a] ───────────────
    bool isLL1 = true;
    auto table = buildParsingTable(grammar, first, follow, isLL1);

    // ── Paso 4: Validación Figura 4.20 ──────
    bool valid = validateWithLL1(toks, grammar, table, follow);

    if (!valid) {
        // Hubo errores de sintaxis — los mensajes ya se imprimieron
        // Intentamos continuar con el AST de todas formas
        // (comportamiento tolerante — útil para debugging)
        cerr << "[Parser] Syntax errors found. "
             << "Attempting to build AST anyway." << endl;
    }

    // ── Paso 5: Construir AST ───────────────
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