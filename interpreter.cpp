//
// Created by Sofía Marín Bustamante on 3/05/26.
//
#include "interpreter.h"
#include <stdexcept>

using namespace std;

//Evaluacion de las condiciones segun el estado actual
bool evalCond(CondNode* node, map<string, int>& variables, set<string>& facts) {
    if (node->type == CondType::AND) {
        AndNode* andNode = (AndNode*)node; // cambiamos el "tipo" de nodo de un CondNode a un AndNode
        return evalCond(andNode->left,  variables, facts) &&
               evalCond(andNode->right, variables, facts);
    } // AND es verdadero si ambos lados lo son
    if (node->type == CondType::CMP) { // si es una comparacion
        CmpNode* cmpNode = (CmpNode*)node;
        if (variables.count(cmpNode->id) == 0) { // esto devuelve 1 si la variable existe en el estado y 0 si no
            return false; // variable no definida significa que la condición es falsa
        }
        int varValue = variables[cmpNode->id];
        if (cmpNode->op == ">") return varValue >  cmpNode->value;
        if (cmpNode->op == "<") return varValue <  cmpNode->value;
        if (cmpNode->op == "=") return varValue == cmpNode->value;

        return false; // operador desconocido
    }
    if (node->type == CondType::FACT) {
        FactNode* factNode = (FactNode*)node;
        return facts.count(factNode->id) > 0;
    } return false; // tipo desconocido
}

set<string> interpret(ProgramNode* program, map<string, int> variables, set<string> facts) {
    set<string> activatedFacts = facts; // hechos iniciales que da el usuario
    bool changed = true; // repetir hasta que no hayan cambios

    while (changed) {
        changed = false; // asumimos que no hay cambios
        for (RuleNode* rule : program->rules) { // evalua todas las reglas
            bool condResult = evalCond( // evalua si la regla es verdadera segun los hechos actuales
                rule->condition,
                variables,
                activatedFacts  // usamos los hechos actuales
            );
            if (condResult) {
                string newFact = rule->action;
                if (activatedFacts.count(newFact) == 0) { // revisa si el hecho ya esta activo
                    activatedFacts.insert(newFact); // si no esta activo (=0) lo inserta
                    changed = true; // hubo cambio, debe repetir el loop
                }
            }
        }
    }
    return activatedFacts;
}