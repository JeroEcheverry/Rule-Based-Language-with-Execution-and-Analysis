
#include "analyzer.h"

using namespace std;

// Compara dos árboles de condición y si su estructura es igual devuelve true
bool conditionsEqual(CondNode* a, CondNode* b) {
    if (a->type != b->type) return false; // si los tipos son distintos ya se descarta que sean iguales

    if (a->type == CondType::AND) {
        AndNode* andA = (AndNode*)a;
        AndNode* andB = (AndNode*)b;
        return conditionsEqual(andA->left,  andB->left) &&
               conditionsEqual(andA->right, andB->right);
    } // ambos lados  son =

    if (a->type == CondType::CMP) { // Si son comparaciones
        CmpNode* cmpA = (CmpNode*)a;
        CmpNode* cmpB = (CmpNode*)b;
        return cmpA->id    == cmpB->id && // cada campo debe ser =
               cmpA->op    == cmpB->op &&
               cmpA->value == cmpB->value;
    }

    if (a->type == CondType::FACT) { // es suficiente con que coincida el nombre
        FactNode* factA = (FactNode*)a;
        FactNode* factB = (FactNode*)b;
        return factA->id == factB->id;
    }

    return false; // si no entro en ningún if entonces de entrada no son iguales
}
// Revisa y recolecta los hechos que se necesitan para cumplir una condicion
void collectFacts(CondNode* node, set<string>& needed) {
    if (node->type == CondType::AND) {
        AndNode* andNode = (AndNode*)node;
        collectFacts(andNode->left,  needed);
        collectFacts(andNode->right, needed);
        return;
    }
    if (node->type == CondType::FACT) { // si es en sí misma un hecho entonces la agregamos
        FactNode* factNode = (FactNode*)node;
        needed.insert(factNode->id);
    }
    // CMP no necesita hechos
}

// verifica si una condición puede ser verdadera
bool conditionSatisfiable(CondNode* node,
                          map<string, int>& variables,
                          set<string>& reachableFacts) {

    if (node->type == CondType::AND) {
        AndNode* andNode = (AndNode*)node;
        // Ambos lados deben poder ser verdaderos
        return conditionSatisfiable(andNode->left,  variables, reachableFacts) &&
               conditionSatisfiable(andNode->right, variables, reachableFacts);
    }

    if (node->type == CondType::CMP) {
        CmpNode* cmpNode = (CmpNode*)node;

        // Si la variable no existe en el estado no puede ser verdadera
        if (variables.count(cmpNode->id) == 0) return false;

        // Verificar si la comparación es verdadera con el valor actual: si con el valor dado ya es falsa, nunca será verdadera
        int val = variables[cmpNode->id];

        if (cmpNode->op == ">") return val >  cmpNode->value;
        if (cmpNode->op == "<") return val <  cmpNode->value;
        if (cmpNode->op == "=") return val == cmpNode->value;

        return false;
    }

    if (node->type == CondType::FACT) {
        FactNode* factNode = (FactNode*)node;
        // El hecho debe ser alcanzable en el sistema
        return reachableFacts.count(factNode->id) > 0;
    }

    return false;
}

AnalysisResult analyze(ProgramNode* program,
                       map<string, int>& variables,
                       set<string>& initialFacts) {

    AnalysisResult result;

    map<string, vector<string>> actionToRules; // acción y la lista de reglas que la activan

    for (RuleNode* rule : program->rules) { // por cada regla
        actionToRules[rule->action].push_back(rule->name); // agregamos la acción y el nombre de la regla
    }

    // Conflicto = acción activada por 2 o más reglas distintas
    for (auto& entry : actionToRules) { // por cada entrada de actionToRules
        if (entry.second.size() > 1) { // si hay mas de un elemento en los nombres de las reglas (second)
            result.conflicts[entry.first] = entry.second; // colocar los nombres de las reglas en el map de conflictos
        }
    }

    int n = program->rules.size();

    for (int i = 0; i < n; i++) { // compara cada par de reglas
        for (int j = i + 1; j < n; j++) {
            RuleNode* r1 = program->rules[i];
            RuleNode* r2 = program->rules[j];

            bool sameCondition = conditionsEqual(r1->condition, r2->condition); // compara los árboles de condicion
            bool sameAction    = (r1->action == r2->action);

            if (sameCondition && sameAction) {
                bool found = false; // verifica si esta en un grupo existente (conflictos, inactivas, etc)
                for (auto& group : result.redundancies) {
                    for (string& name : group) {
                        if (name == r1->name) {
                            group.push_back(r2->name);
                            found = true;
                            break;
                        }
                    }
                    if (found) break; // si no estaba en un grupo creamos otro nuevo
                }
                if (!found) {
                    result.redundancies.push_back({r1->name, r2->name});
                }
            }
        }
    }

    set<string> reachableFacts = initialFacts; // Hechos alcanzables = iniciales + acciones de todas las reglas
    for (RuleNode* rule : program->rules) {
        reachableFacts.insert(rule->action);
    }

    for (RuleNode* rule : program->rules) {
        bool satisfiable = conditionSatisfiable( // revisa si puede ser alguna vez activado o no
            rule->condition,
            variables,
            reachableFacts
        );

        if (!satisfiable){ // si no, es una regla inactiva
            result.inactiveRules.push_back(rule->name);
        }
    }

    return result;
}