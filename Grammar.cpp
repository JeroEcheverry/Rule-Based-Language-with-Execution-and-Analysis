//
// Created by Sofía Marín Bustamante on 8/05/26.
//
#include "Grammar.h"

#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

// ─────────────────────────────────────────────
// ¿Es no-terminal? — empieza con mayúscula
// ─────────────────────────────────────────────
bool isNonTerminal(const string& s) {
    if (s.empty()) return false;
    return isupper(s[0]);
}

// ─────────────────────────────────────────────
// LEER GRAMÁTICA DESDE TEXTO
// Formato:
//   Program -> RuleList
//   RuleList -> Rule RuleList | eps
//   Cond -> Cond AND Cond | Atom
// ─────────────────────────────────────────────
Grammar parseGrammar(const string& input) {
    Grammar g;
    istringstream stream(input);
    string line;
    bool firstLine = true;

    while (getline(stream, line)) {
        // Ignorar líneas vacías
        if (line.empty()) continue;

        // Buscar "->"
        size_t arrow = line.find("->");
        if (arrow == string::npos) continue;

        // Lado izquierdo
        string lhs = line.substr(0, arrow);
        // Quitar espacios del lhs
        while (!lhs.empty() && lhs[0] == ' ') lhs = lhs.substr(1);
        while (!lhs.empty() && lhs.back() == ' ') lhs.pop_back();

        if (lhs.empty()) continue;

        // Primer no-terminal definido = símbolo inicial
        if (firstLine) {
            g.startSymbol = lhs;
            firstLine = false;
        }

        g.nonTerminals.insert(lhs);

        // Lado derecho — puede tener alternativas separadas por |
        string rhs = line.substr(arrow + 2);

        // Separar por |
        vector<string> alternatives;
        string current;
        for (char c : rhs) {
            if (c == '|') {
                alternatives.push_back(current);
                current = "";
            } else {
                current += c;
            }
        }
        alternatives.push_back(current);

        // Procesar cada alternativa
        for (string& alt : alternatives) {
            Production prod;
            prod.lhs = lhs;

            // Tokenizar la alternativa por espacios
            istringstream altStream(alt);
            string sym;
            while (altStream >> sym) {
                prod.rhs.push_back(sym);
                // Clasificar símbolo
                if (sym != "eps" && !isNonTerminal(sym)) {
                    g.terminals.insert(sym);
                }
            }

            if (prod.rhs.empty()) {
                prod.rhs.push_back("eps");
            }

            g.productions.push_back(prod);
        }
    }

    // Agregar $ como terminal especial
    g.terminals.insert("$");

    return g;
}

// ─────────────────────────────────────────────
// ELIMINAR RECURSIÓN IZQUIERDA INMEDIATA
// Si A → A α | β  →  A → β A' / A' → α A' | eps
// Funciona para CUALQUIER gramática
// ─────────────────────────────────────────────
Grammar eliminateLeftRecursion(const Grammar& g) {
    Grammar result;
    result.startSymbol = g.startSymbol;
    result.terminals = g.terminals;

    // Agrupar producciones por no-terminal
    map<string, vector<vector<string>>> prods;
    // Mantener orden de aparición
    vector<string> order;
    for (auto& p : g.productions) {
        if (prods.find(p.lhs) == prods.end()) {
            order.push_back(p.lhs);
        }
        prods[p.lhs].push_back(p.rhs);
    }

    for (auto& A : order) {
        auto& rhsList = prods[A];

        vector<vector<string>> alphas; // A → A α → guardamos α
        vector<vector<string>> betas;  // A → β (sin recursión)

        for (auto& rhs : rhsList) {
            if (!rhs.empty() && rhs[0] == A) {
                // Recursión izquierda: A → A α
                // α = todo menos el primer símbolo
                alphas.push_back(
                    vector<string>(rhs.begin() + 1, rhs.end())
                );
            } else {
                betas.push_back(rhs);
            }
        }

        if (alphas.empty()) {
            // Sin recursión → copiar igual
            result.nonTerminals.insert(A);
            for (auto& rhs : betas) {
                result.productions.push_back({A, rhs});
            }
        } else {
            // Hay recursión → transformar
            string Aprime = A + "'";
            result.nonTerminals.insert(A);
            result.nonTerminals.insert(Aprime);

            // A → β A'  para cada β
            for (auto& beta : betas) {
                vector<string> newRhs = beta;
                // Si beta es solo eps, el nuevo rhs es solo A'
                if (newRhs.size() == 1 && newRhs[0] == "eps") {
                    newRhs = {Aprime};
                } else {
                    newRhs.push_back(Aprime);
                }
                result.productions.push_back({A, newRhs});
            }

            // A' → α A'  para cada α
            for (auto& alpha : alphas) {
                vector<string> newRhs = alpha;
                newRhs.push_back(Aprime);
                result.productions.push_back({Aprime, newRhs});
            }

            // A' → eps
            result.productions.push_back({Aprime, {"eps"}});
        }
    }

    return result;
}

// ─────────────────────────────────────────────
// LEFT FACTORING
// Si A → α β₁ | α β₂  →  A → α A' / A' → β₁ | β₂
// Funciona para CUALQUIER gramática
// ─────────────────────────────────────────────
Grammar leftFactoring(const Grammar& g) {
    Grammar result;
    result.startSymbol = g.startSymbol;
    result.terminals = g.terminals;
    result.nonTerminals = g.nonTerminals;

    // Agrupar por no-terminal manteniendo orden
    map<string, vector<vector<string>>> prods;
    vector<string> order;
    for (auto& p : g.productions) {
        if (prods.find(p.lhs) == prods.end()) {
            order.push_back(p.lhs);
        }
        prods[p.lhs].push_back(p.rhs);
    }

    int counter = 1; // para nombrar A'1, A'2 si hay varios

    for (auto& A : order) {
        auto rhsList = prods[A];

        // Buscar prefijo común entre producciones
        bool factored = true;
        while (factored) {
            factored = false;

            // Agrupar producciones por primer símbolo
            map<string, vector<vector<string>>> byFirst;
            for (auto& rhs : rhsList) {
                string first = rhs.empty() ? "eps" : rhs[0];
                byFirst[first].push_back(rhs);
            }

            vector<vector<string>> newRhsList;

            for (auto& entry : byFirst) {
                string firstSym = entry.first;
                auto& group = entry.second;

                if (group.size() == 1) {
                    // Sin conflicto — copiar tal cual
                    newRhsList.push_back(group[0]);
                } else {
                    // Hay prefijo común — factorizar
                    factored = true;
                    string Aprime = A + "'" + to_string(counter++);
                    result.nonTerminals.insert(Aprime);

                    // A → firstSym Aprime
                    newRhsList.push_back({firstSym, Aprime});

                    // Aprime → resto de cada alternativa
                    for (auto& rhs : group) {
                        vector<string> rest(rhs.begin() + 1, rhs.end());
                        if (rest.empty()) rest = {"eps"};
                        result.productions.push_back({Aprime, rest});
                    }
                }
            }

            rhsList = newRhsList;
        }

        // Agregar las producciones finales de A
        for (auto& rhs : rhsList) {
            result.productions.push_back({A, rhs});
        }
    }

    return result;
}

// ─────────────────────────────────────────────
// CALCULAR FIRST
// ─────────────────────────────────────────────
map<string, set<string>> computeFirst(const Grammar& g) {
    map<string, set<string>> first;

    // FIRST de terminales = él mismo
    for (auto& t : g.terminals) first[t].insert(t);
    first["eps"].insert("eps");

    // Inicializar no-terminales vacíos
    for (auto& nt : g.nonTerminals) first[nt] = {};

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& prod : g.productions) {
            string A = prod.lhs;
            auto& rhs = prod.rhs;

            if (rhs[0] == "eps") {
                if (first[A].insert("eps").second) changed = true;
                continue;
            }

            bool allEps = true;
            for (auto& Y : rhs) {
                for (auto& f : first[Y]) {
                    if (f != "eps") {
                        if (first[A].insert(f).second) changed = true;
                    }
                }
                if (!first[Y].count("eps")) {
                    allEps = false;
                    break;
                }
            }
            if (allEps) {
                if (first[A].insert("eps").second) changed = true;
            }
        }
    }
    return first;
}

// ─────────────────────────────────────────────
// CALCULAR FOLLOW
// ─────────────────────────────────────────────
map<string, set<string>> computeFollow(
    const Grammar& g,
    map<string, set<string>>& first)
{
    map<string, set<string>> follow;
    for (auto& nt : g.nonTerminals) follow[nt] = {};
    follow[g.startSymbol].insert("$");

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& prod : g.productions) {
            string A = prod.lhs;
            auto& rhs = prod.rhs;

            for (int i = 0; i < (int)rhs.size(); i++) {
                string B = rhs[i];
                if (!g.nonTerminals.count(B)) continue;

                // Lo que viene después de B
                bool betaAllEps = true;
                for (int j = i + 1; j < (int)rhs.size(); j++) {
                    string beta = rhs[j];
                    for (auto& f : first[beta]) {
                        if (f != "eps") {
                            if (follow[B].insert(f).second) changed = true;
                        }
                    }
                    if (!first[beta].count("eps")) {
                        betaAllEps = false;
                        break;
                    }
                }
                // Si β puede ser ε o B está al final
                if (betaAllEps) {
                    for (auto& f : follow[A]) {
                        if (follow[B].insert(f).second) changed = true;
                    }
                }
            }
        }
    }
    return follow;
}

// ─────────────────────────────────────────────
// CONSTRUIR TABLA DE PARSING M[A][a]
// ─────────────────────────────────────────────
map<string, map<string, Production>> buildParsingTable(
    const Grammar& g,
    map<string, set<string>>& first,
    map<string, set<string>>& follow)
{
    map<string, map<string, Production>> table;

    for (auto& prod : g.productions) {
        string A = prod.lhs;
        auto& rhs = prod.rhs;

        // Calcular FIRST(rhs)
        set<string> firstRhs;
        bool allEps = true;

        if (rhs[0] == "eps") {
            firstRhs.insert("eps");
        } else {
            for (auto& Y : rhs) {
                for (auto& f : first[Y]) {
                    if (f != "eps") firstRhs.insert(f);
                }
                if (!first[Y].count("eps")) {
                    allEps = false;
                    break;
                }
            }
            if (allEps) firstRhs.insert("eps");
        }

        // M[A][a] = prod para cada a ∈ FIRST(rhs) - {ε}
        for (auto& a : firstRhs) {
            if (a != "eps") {
                table[A][a] = prod;
            }
        }

        // Si ε ∈ FIRST(rhs) → M[A][b] para cada b ∈ FOLLOW(A)
        if (firstRhs.count("eps")) {
            for (auto& b : follow[A]) {
                table[A][b] = prod;
            }
        }
    }

    return table;
}

// ─────────────────────────────────────────────
// IMPRIMIR GRAMÁTICA
// ─────────────────────────────────────────────
void printGrammar(const Grammar& g) {
    // Mantener orden: primero el símbolo inicial
    set<string> printed;
    // Imprimir en orden de aparición
    string currentLhs = "";
    for (auto& prod : g.productions) {
        if (prod.lhs != currentLhs) {
            if (!printed.count(prod.lhs)) {
                currentLhs = prod.lhs;
                cout << "  " << prod.lhs << " -> ";
                printed.insert(prod.lhs);
                // Imprimir todas las alternativas de este no-terminal
                bool first = true;
                for (auto& p : g.productions) {
                    if (p.lhs == prod.lhs) {
                        if (!first) cout << " | ";
                        for (int i = 0; i < (int)p.rhs.size(); i++) {
                            cout << p.rhs[i];
                            if (i < (int)p.rhs.size()-1) cout << " ";
                        }
                        first = false;
                    }
                }
                cout << endl;
            }
        }
    }
}

// ─────────────────────────────────────────────
// IMPRIMIR TABLA FIRST Y FOLLOW
// ─────────────────────────────────────────────
void printFirstFollow(
    const Grammar& g,
    map<string, set<string>>& first,
    map<string, set<string>>& follow) {
    cout << left;
    cout << "  " << setw(12) << "Non-Terminal"
         << " | " << setw(25) << "FIRST"
         << " | " << "FOLLOW" << endl;
    cout << "  " << string(12,'-')
         << "-+-" << string(25,'-')
         << "-+-" << string(20,'-') << endl;

    for (auto& nt : g.nonTerminals) {
        string f = "{ ";
        for (auto& s : first[nt]) f += s + " ";
        f += "}";

        string fo = "{ ";
        for (auto& s : follow[nt]) fo += s + " ";
        fo += "}";

        cout << "  " << std::setw(12) << nt
             << " | " << setw(25) << f
             << " | " << fo << endl;
    }
}
