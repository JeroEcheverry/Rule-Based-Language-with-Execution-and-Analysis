#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <string>
#include <vector>
#include <map>
#include <set>

using namespace std;
struct Production {
    string         lhs; // lado izquierdo:  "Cond"
    vector<string> rhs; // lado derecho:    {"Atom","CondRest"}
};

struct Grammar {
    vector<Production> productions; // todas las producciones en orden
    set<string>        nonTerminals;
    set<string>        terminals;
    string             startSymbol;
};

Grammar parseGrammarFromText(const string& input);

Grammar eliminateLeftRecursion(const Grammar& g);

Grammar leftFactoring(const Grammar& g);

map<string, set<string>> computeFirst(const Grammar& g);

map<string, set<string>> computeFollow(
    const Grammar& g,
    map<string, set<string>>& first);

map<string, map<string, Production>> buildLL1Table(
    const Grammar& g,
    map<string, set<string>>& first,
    map<string, set<string>>& follow);

void printGrammar(const Grammar& g);

void printFirstFollow(
    const Grammar& g,
    map<string, set<string>>& first,
    map<string, set<string>>& follow);

void printParsingTable(
    const Grammar& g,
    map<string, map<string, Production>>& table);

#endif