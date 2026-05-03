#ifndef AST_H
#define AST_H
 
#include <string>
#include <vector>
 
using namespace std;

// Tipos de condición (para identificar en el intérprete)
enum class CondType { // con enum luego se puede tratar como comparación el tipo para saber si cumple
    AND,   // Cond AND Cond
    CMP,   // id > value  |  id < value  |  id = value
    FACT   // id  (verdadero si el hecho está activo)
};
 
// Nodo base de condición
// Todos los tipos de condición heredan de este
struct CondNode {
    CondType type;
    virtual ~CondNode() {} // destructor virtual para liberar en memoria
};
 
// Nodo AND: representa "condición izquierda AND condición derecha"
// Ejemplo: temp > 30 AND humidity < 50
struct AndNode : public CondNode {
    CondNode* left;   // condición izquierda
    CondNode* right;  // condición derecha
 
    AndNode(CondNode* l, CondNode* r) {
        type  = CondType::AND;
        left  = l;
        right = r;
    }
};

// Nodo CMP: representa una comparación entre variable y valor entero
// Ejemplo: temp > 30   →   CmpNode("temp", ">", 30)
struct CmpNode : public CondNode {
    string id;   // nombre de la variable
    string op;   // operador: ">", "<", "="
    int value;   // valor entero con el que se compara
 
    CmpNode(string i, string o, int v) {
        type  = CondType::CMP;
        id    = i;
        op    = o;
        value = v;
    }
};
 
// Nodo FACT: representa un hecho simple
// Ejemplo: alert   →   verdadero si "alert" está en el conjunto de hechos activos
struct FactNode : public CondNode {
    string id;  // nombre del hecho
 
    FactNode(string i) {
        type = CondType::FACT;
        id   = i;
    }
};

struct RuleNode {
    string name;         // nombre de la regla: r1, r2, etc.
    CondNode* condition; // árbol de condición
    string action;       // hecho que se activa si la condición es verdadera
 
    RuleNode(string n, CondNode* c, string a) {
        name      = n;
        condition = c;
        action    = a;
    }
};

struct ProgramNode {
    vector<RuleNode*> rules; // lista de todas las reglas
};
 
#endif