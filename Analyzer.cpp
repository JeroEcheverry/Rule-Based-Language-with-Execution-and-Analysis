#include "Analyzer.h"

using namespace std;

bool conditionsEqual(CondNode* a, CondNode* b) {
    if (a->type != b->type) return false; // different types → cannot be equal

    if (a->type == CondType::AND) {
        AndNode* andA = (AndNode*)a;
        AndNode* andB = (AndNode*)b;
        return conditionsEqual(andA->left,  andB->left) && // compare left subtrees
               conditionsEqual(andA->right, andB->right);  // compare right subtrees
    }

    if (a->type == CondType::CMP) {
        CmpNode* cmpA = (CmpNode*)a;
        CmpNode* cmpB = (CmpNode*)b;
        return cmpA->id    == cmpB->id    && // same variable name
               cmpA->op    == cmpB->op    && // same operator
               cmpA->value == cmpB->value;   // same integer value
    }

    if (a->type == CondType::FACT) {
        FactNode* factA = (FactNode*)a;
        FactNode* factB = (FactNode*)b;
        return factA->id == factB->id; // same fact name is enough
    }

    return false;
}

void collectFacts(CondNode* node, set<string>& needed) {
    if (node->type == CondType::AND) {
        AndNode* andNode = (AndNode*)node;
        collectFacts(andNode->left,  needed); // collect from left subtree
        collectFacts(andNode->right, needed); // collect from right subtree
        return;
    }
    if (node->type == CondType::FACT) {
        FactNode* factNode = (FactNode*)node;
        needed.insert(factNode->id); // add fact name to the needed set
    }
    // CMP nodes don't require facts, only variables
}

bool conditionSatisfiable(CondNode* node,
                          map<string, int>& variables,
                          set<string>& reachableFacts) {

    if (node->type == CondType::AND) {
        AndNode* andNode = (AndNode*)node;
        return conditionSatisfiable(andNode->left,  variables, reachableFacts) && // both sides must be satisfiable
               conditionSatisfiable(andNode->right, variables, reachableFacts);
    }

    if (node->type == CondType::CMP) {
        CmpNode* cmpNode = (CmpNode*)node;
        if (variables.count(cmpNode->id) == 0) return false; // variable not in state → never true
        int val = variables[cmpNode->id];
        if (cmpNode->op == ">") return val >  cmpNode->value;
        if (cmpNode->op == "<") return val <  cmpNode->value;
        if (cmpNode->op == "=") return val == cmpNode->value;
        return false;
    }

    if (node->type == CondType::FACT) {
        FactNode* factNode = (FactNode*)node;
        return reachableFacts.count(factNode->id) > 0; // fact must be reachable in the system
    }

    return false;
}

AnalysisResult analyze(ProgramNode* program,
                       map<string, int>& variables,
                       set<string>& initialFacts) {

    AnalysisResult result;

    // ── BLOCK 1: Detect conflicts ──────────────────────────────
    // A conflict occurs when two or more different rules activate the same fact
    map<string, vector<string>> actionToRules; // maps action → list of rules that activate it

    for (RuleNode* rule : program->rules) {
        actionToRules[rule->action].push_back(rule->name); // group rules by their action
    }

    for (auto& entry : actionToRules) {
        if (entry.second.size() > 1) { // more than one rule activates this action → conflict
            result.conflicts[entry.first] = entry.second;
        }
    }

    // ── BLOCK 2: Detect redundancies ──────────────────────────
    // Two rules are redundant if they have the same condition AND the same action
    int n = program->rules.size();

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) { // compare every pair of rules
            RuleNode* r1 = program->rules[i];
            RuleNode* r2 = program->rules[j];

            bool sameCondition = conditionsEqual(r1->condition, r2->condition); // compare condition trees
            bool sameAction    = (r1->action == r2->action);

            if (sameCondition && sameAction) {
                bool found = false; // check if r1 is already in a redundancy group
                for (auto& group : result.redundancies) {
                    for (string& name : group) {
                        if (name == r1->name) {
                            group.push_back(r2->name); // add r2 to the existing group
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }
                if (!found) {
                    result.redundancies.push_back({r1->name, r2->name}); // create new group
                }
            }
        }
    }

    // ── BLOCK 3: Detect potentially inactive rules ─────────────
    // A rule is potentially inactive if its condition can never be true
    set<string> reachableFacts = initialFacts; // start with initial facts
    for (RuleNode* rule : program->rules) {
        reachableFacts.insert(rule->action); // all rule actions are potentially reachable
    }

    for (RuleNode* rule : program->rules) {
        bool satisfiable = conditionSatisfiable(rule->condition, variables, reachableFacts);
        if (!satisfiable) {
            result.inactiveRules.push_back(rule->name); // rule can never fire
        }
    }

    return result;
}