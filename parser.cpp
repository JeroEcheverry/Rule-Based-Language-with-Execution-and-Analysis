#include "parser.h"
#include <stdexcept>  // para runtime_error (errores de sintaxis)

using namespace std;

static vector<Token> tokens; // los tokens que dio el lexer
static int pos; //indice, son static porque solo le corresponden al parser

Token current() { //lee y devuelve el token del índice
    return tokens[pos];
}

Token lookahead(int n) { //devuelve lo que hay en la posicion pos+n (adelante) para saber que producción conviene usar
    int idx = pos + n;
    if (idx < (int)tokens.size()) return tokens[idx];
    return {TokenType::END, ""}; // si nos pasamos del final, END
}

void consume(TokenType expected) { // Verifica que el token actual sea del tipo esperado (cumple la gramática)
    if (current().type != expected) {
        throw runtime_error(
            "Error de sintaxis: se esperaba '" + current().value +
            "' de tipo distinto cerca de: " + current().value
        );
    }
    pos++; // mover el indice al siguiente token
}
//definición de funciones que se usan abajo
CondNode* parseCond();
CondNode* parseCondRest(CondNode* left);
CondNode* parseAtom();

// Gramática:
//   Atom → id RelOp value   (comparación)
//   Atom → id               (hecho)
CondNode* parseAtom() {
    string idName = current().value; //ambas producciones empiezan con id
    consume(TokenType::ID);
    TokenType next = current().type;
    if (next == TokenType::GT ||
        next == TokenType::LT ||
        next == TokenType::EQ) {
        string op = current().value; // guardamos ">", "<" o "="
        pos++;
        int val = stoi(current().value); // stoi cambia de string a int ("30"-> 30)
        consume(TokenType::NUMBER);
        return new CmpNode(idName, op, val);
    } else {
        return new FactNode(idName); //hecho simple
    }
}

// Gramática:
//   CondRest → AND Atom CondRest   (hay más condiciones)
//   CondRest → ε                   (no hay más, termina)

// Recibe el nodo izquierdo acumulado hasta ahora y decide si hay mas condiciones
CondNode* parseCondRest(CondNode* left) {
    if (current().type == TokenType::AND) { //hay mas condiciones
        consume(TokenType::AND); //consume el AND
        CondNode* right = parseAtom(); //parse el atomo de la derecha
        AndNode* node = new AndNode(left, right); // Construye el nodo AND combinando izquierda y derecha
        return parseCondRest(node); //pueden haber más AND
    }
    return left; // si no hay AND el token actual es "then" y la produccion es ε (retorna lo acumulado hasta aca)
}
// Gramática:
//    Cond → Atom CondRest
CondNode* parseCond() {
    CondNode* left = parseAtom();      // parsea el primer átomo
    return parseCondRest(left);        // maneja los AND que siguen
}

// Gramática:
//   Rule → rule id : if Cond then Action
RuleNode* parseRule() {
    consume(TokenType::RULE);// consume "rule"
    string name = current().value; // guarda el nombre de la regla ("r1", "r2")
    consume(TokenType::ID); // consume el nombre
    consume(TokenType::COLON); // consume ":"
    consume(TokenType::IF); // consume "if"

    CondNode* cond = parseCond();  // parsea toda la condición

    consume(TokenType::THEN);// consume "then"
    string action = current().value; // guarda la accion
    consume(TokenType::ID); // consume la acción

    return new RuleNode(name, cond, action); // devuelve el nodo
}

// Gramática:
//   RuleList → Rule RuleList | ε
void parseRuleList(ProgramNode* program) {
    if (current().type == TokenType::RULE) {
        RuleNode* rule = parseRule(); // parsear la regla
        program->rules.push_back(rule); // agregarla al programa

        parseRuleList(program); // busca más reglas recursivamente
    }
    // si es STATE o END lleva a la producción ε (no hace nada)
}

// Lee el bloque State: con variables y hechos activos
void parseState(map<string, int>& variables, set<string>& facts) {
    if (current().type != TokenType::STATE){return;} //si no hay state no hay estado inicial
    consume(TokenType::STATE); // consume el token "State"
    while (current().type != TokenType::END) { //lee toda la entrada
        string name = current().value;
        consume(TokenType::ID); // consume el identificador
        if (current().type == TokenType::EQ) { // es una variable del estilo id = número
            consume(TokenType::EQ);
            int val = stoi(current().value); // transforma a int el valor que estaba en string
            consume(TokenType::NUMBER);
            variables[name] = val; // guarda el el valor asociandolo al nombre
        } else {
            facts.insert(name); // Es un hecho activo: solo el id
        }
    }
}

//Recibe los tokens del lexer, retorna el AST + estado inicial
ParseResult parse(vector<Token> toks) {
    tokens = toks; // cargar los tokens en la variable global
    pos = 0;       // empezar desde el principio

    ProgramNode* program = new ProgramNode();
    parseRuleList(program);  // construir el AST de las reglas

    map<string, int> variables;
    set<string> facts;
    parseState(variables, facts); // leer el estado inicial

    return {program, variables, facts};
}