#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <string>
#include <vector>
#include <map>
#include <set>

using namespace std;

struct Production {
    string lhs;           // lado izquierdo: "Cond"
    vector<string> rhs;   // lado derecho: ["Atom", "CondRest"]
    // "eps" representa ε
};

struct Grammar {
    vector<Production> productions;
    set<string> nonTerminals;
    set<string> terminals;
    string startSymbol;
};

bool isNonTerminal(const string& s);
Grammar parseGrammar(const string& input);
Grammar eliminateLeftRecursion(const Grammar& g);
Grammar leftFactoring(const Grammar& g);
map<string, set<string>> computeFirst(const Grammar& g);

map<string, set<string>> computeFollow(
    const Grammar& g,
    map<string, set<string>>& first);

map<string, map<string, Production>> buildParsingTable(
    const Grammar& g,
    map<string, set<string>>& first,
    map<string, set<string>>& follow);

void printGrammar(const Grammar& g);
void printFirstFollow(
    const Grammar& g,
    map<string, set<string>>& first,
    map<string, set<string>>& follow);

#endif