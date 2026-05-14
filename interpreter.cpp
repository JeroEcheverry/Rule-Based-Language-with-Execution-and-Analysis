#include "interpreter.h"
#include <stdexcept>

using namespace std;

bool evalCond(CondNode* node, map<string, int>& variables, set<string>& facts) {
    
    if (node->type == CondType::AND) {
        AndNode* andNode = (AndNode*)node; // cast from CondNode to AndNode to access left and right
        return evalCond(andNode->left,  variables, facts) && // both sides must be true
               evalCond(andNode->right, variables, facts);
    }
    
    if (node->type == CondType::CMP) {
        CmpNode* cmpNode = (CmpNode*)node;
        if (variables.count(cmpNode->id) == 0) return false; // variable not in state → false
        int varValue = variables[cmpNode->id]; // get the variable's current value
        if (cmpNode->op == ">") return varValue >  cmpNode->value;
        if (cmpNode->op == "<") return varValue <  cmpNode->value;
        if (cmpNode->op == "=") return varValue == cmpNode->value;
        return false; // unknown operator
    }

    if (node->type == CondType::FACT) {
        FactNode* factNode = (FactNode*)node;
        return facts.count(factNode->id) > 0; // true if fact is in the active set
    }
    
    return false;
}

set<string> interpret(ProgramNode* program, map<string, int> variables, set<string> facts) {
    set<string> activatedFacts = facts; // all active facts: initial + newly activated
    set<string> newFacts;               // only facts activated by rules (used for output)
    bool changed = true;                // fixed-point loop control

    while (changed) {
        changed = false; // assume no changes until a new fact is found

        for (RuleNode* rule : program->rules) {
            bool condResult = evalCond(rule->condition, variables, activatedFacts); // evaluate rule condition

            if (condResult) {
                string newFact = rule->action;
                if (activatedFacts.count(newFact) == 0) { // only activate if not already active
                    activatedFacts.insert(newFact); // add to all active facts for future evaluations
                    newFacts.insert(newFact);        // add to output set
                    changed = true;                  // a new fact was found → iterate again
                }
            }
        }
    }

    return newFacts; // return only rule-activated facts, not the initial ones
}