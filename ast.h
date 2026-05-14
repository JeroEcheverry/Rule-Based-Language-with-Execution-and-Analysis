#ifndef AST_H
#define AST_H

#include <string>
#include <vector>

using namespace std;

// Identifies the type of condition node so the interpreter knows how to evaluate it
enum class CondType {
    AND,   // conjunction: Cond AND Cond
    CMP,   // comparison: id > value | id < value | id = value
    FACT   // active fact: id (true if the fact is in the active set)
};

struct CondNode {
    CondType type;
    virtual ~CondNode() {} // virtual destructor ensures correct memory cleanup with inheritance
};

struct AndNode : public CondNode {
    CondNode* left;  // left side of the AND
    CondNode* right; // right side of the AND

    AndNode(CondNode* l, CondNode* r) {
        type  = CondType::AND;
        left  = l;
        right = r;
    }
};

struct CmpNode : public CondNode {
    string id;    // variable name being compared
    string op;    // relational operator: ">", "<", or "="
    int    value; // integer value to compare against

    CmpNode(string i, string o, int v) {
        type  = CondType::CMP;
        id    = i;
        op    = o;
        value = v;
    }
};

struct FactNode : public CondNode {
    string id; // fact name — true if present in the active facts set

    FactNode(string i) {
        type = CondType::FACT;
        id   = i;
    }
};

struct RuleNode {
    string    name;      // rule identifier: r1, r2, ...
    CondNode* condition; // pointer to the condition tree (any CondNode type)
    string    action;    // fact to activate if condition is true

    RuleNode(string n, CondNode* c, string a) {
        name      = n;
        condition = c;
        action    = a;
    }
};

struct ProgramNode {
    vector<RuleNode*> rules; // list of all rules in the program
};

#endif