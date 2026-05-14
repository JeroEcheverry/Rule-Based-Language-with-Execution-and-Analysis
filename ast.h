#ifndef AST_H
#define AST_H

#include <string>
#include <vector>

using namespace std;

// IMPORTANT: The AST does NOT execute anything. It only stores
// the structure of the program. The interpreter walks it later.
// CondType — Condition node type identifier
//
// Used by the interpreter to know what kind of condition
// AND  → conjunction:  Cond AND Cond
// CMP  → comparison:   id > value | id < value | id = value
// FACT → active fact:  id  (true if the fact is in the active set)

enum class CondType {
    AND,
    CMP,
    FACT
};

// CondNode — Base condition node
// All condition node types inherit from this struct.
// The parser returns CondNode* without knowing the specific type.
// The interpreter checks the 'type' field to decide how to evaluate.
//
// The virtual destructor ensures proper memory cleanup
// when deleting through a base class pointer.

struct CondNode {
    CondType type;
    virtual ~CondNode() {}
};

// AndNode — Conjunction node
// Represents: condition AND condition
// Example:    temp > 30 AND humidity < 50
//             → AndNode(CmpNode("temp",">",30), CmpNode("humidity","<",50))
//
// Both left and right are CondNode* so they can hold
// any condition type, including nested AND nodes.

struct AndNode : public CondNode {
    CondNode* left;   // left side of the AND
    CondNode* right;  // right side of the AND

    AndNode(CondNode* l, CondNode* r) {
        type  = CondType::AND;
        left  = l;
        right = r;
    }
};


// CmpNode — Comparison node
// Represents: identifier operator integer
// Example:    temp > 30  →  CmpNode("temp", ">", 30)
// Fields:
//   id    → name of the variable being compared
//   op    → relational operator: ">", "<", or "="
//   value → integer value to compare against

struct CmpNode : public CondNode {
    string id;
    string op;
    int value;

    CmpNode(string i, string o, int v) {
        type  = CondType::CMP;
        id    = i;
        op    = o;
        value = v;
    }
};

// FactNode — Fact node
// Represents: a single identifier used as a condition
// Example:    alert  →  FactNode("alert")
//
// Evaluated as true if 'id' is present in the active facts set.

struct FactNode : public CondNode {
    string id;

    FactNode(string i) {
        type = CondType::FACT;
        id   = i;
    }
};

// RuleNode — Complete rule node
// Represents a full rule of the language:
//   rule r1: if temp > 30 then alert
//   RuleNode("r1", CmpNode("temp",">",30), "alert")
// Fields:
//   name      → rule identifier (r1, r2, ...)
//   condition → pointer to the condition tree (any CondNode type)
//   action    → name of the fact to activate if condition is true

struct RuleNode {
    string name;
    CondNode* condition;
    string action;

    RuleNode(string n, CondNode* c, string a) {
        name      = n;
        condition = c;
        action    = a;
    }
};

// Represents the entire program as a list of rules.
// This is what the parser returns and what the interpreter
// and static analyzer receive as input.

struct ProgramNode {
    vector<RuleNode*> rules;
};

#endif