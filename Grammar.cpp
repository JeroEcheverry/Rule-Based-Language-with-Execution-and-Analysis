//
// Created by Sofía Marín Bustamante on 8/05/26.
//

#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include "Grammar.h"
using namespace std;

// ─────────────────────────────────────────────
// ESTRUCTURAS BASE
// Una producción es: lhs → rhs[0] rhs[1] ... rhs[n]
// "eps" representa ε (cadena vacía)
// ─────────────────────────────────────────────

// ─────────────────────────────────────────────
// ¿Es no-terminal?
// Convención del Dragon Book: empieza con mayúscula
// ─────────────────────────────────────────────
bool isNonTerminal(const string& s) {
    if (s.empty()) return false;
    // Atom' también es no-terminal (tiene mayúscula al inicio)
    return isupper((unsigned char)s[0]);
}

// ─────────────────────────────────────────────
// LEER GRAMÁTICA DESDE TEXTO
//
// Formato esperado:
//   Program -> RuleList
//   RuleList -> Rule RuleList | eps
//   Cond -> Cond AND Cond | Atom
//
// Reglas del formato:
//   - No-terminales: empiezan con MAYÚSCULA  (Program, Cond, Atom)
//   - Terminales:    empiezan con minúscula  (rule, if, then)
//                    o son símbolos          (>, <, =, :)
//   - Epsilon:       escribir como           eps
//   - Separador:     usar ->
//   - Alternativas:  usar |
//   - Símbolo inicial: primera línea definida
// ─────────────────────────────────────────────
Grammar parseGrammarFromText(const string& input) {
    Grammar g;
    istringstream stream(input);
    string line;
    bool firstSymbol = true;

    while (getline(stream, line)) {
        // Ignorar líneas vacías
        if (line.empty()) continue;

        // Buscar "->"
        size_t arrow = line.find("->");
        if (arrow == string::npos) continue;

        // Lado izquierdo — quitar espacios
        string lhs = line.substr(0, arrow);
        while (!lhs.empty() && lhs.front() == ' ') lhs.erase(0,1);
        while (!lhs.empty() && lhs.back()  == ' ') lhs.pop_back();
        if (lhs.empty()) continue;

        // El primer no-terminal definido es el símbolo inicial
        if (firstSymbol) {
            g.startSymbol = lhs;
            firstSymbol = false;
        }
        g.nonTerminals.insert(lhs);

        // Lado derecho — separar por '|'
        string rhs = line.substr(arrow + 2);
        vector<string> alternatives;
        string cur;
        for (char c : rhs) {
            if (c == '|') {
                alternatives.push_back(cur);
                cur = "";
            } else {
                cur += c;
            }
        }
        alternatives.push_back(cur);

        // Procesar cada alternativa
        for (string& alt : alternatives) {
            Production prod;
            prod.lhs = lhs;

            istringstream altStream(alt);
            string sym;
            while (altStream >> sym) {
                prod.rhs.push_back(sym);
                if (sym != "eps" && !isNonTerminal(sym)) {
                    g.terminals.insert(sym);
                }
            }
            if (prod.rhs.empty()) prod.rhs.push_back("eps");
            g.productions.push_back(prod);
        }
    }

    g.terminals.insert("$"); // siempre existe el fin de entrada
    return g;
}

// ─────────────────────────────────────────────
// ELIMINAR RECURSIÓN IZQUIERDA INMEDIATA
// Algoritmo del Dragon Book Sección 4.3
//
// Para cada A con producciones A → A α | β:
//   Se reemplaza por:
//     A  → β A'
//     A' → α A' | ε
//
// Funciona para CUALQUIER gramática
// ─────────────────────────────────────────────
void eliminateDirectLeftRecursion(const string& A,
                                 map<string, vector<vector<string>>>& prods,
                                 Grammar& res) {
    vector<vector<string>> alphas;
    vector<vector<string>> betas;

    for (auto& rhs : prods[A]) {
        if (!rhs.empty() && rhs[0] == A) {
            alphas.push_back(vector<string>(rhs.begin() + 1, rhs.end()));
        } else {
            betas.push_back(rhs);
        }
    }

    if (alphas.empty()) {
        for (auto& rhs : betas) res.productions.push_back({A, rhs});
    } else {
        string Ap = A + "'";
        res.nonTerminals.insert(Ap);

        // A -> beta A'
        for (auto& beta : betas) {
            vector<string> newRhs = (beta.size() == 1 && beta[0] == "eps") ? vector<string>{Ap} : beta;
            if (newRhs.back() != Ap) newRhs.push_back(Ap);
            res.productions.push_back({A, newRhs});
        }
        // A' -> alpha A' | eps
        for (auto& alpha : alphas) {
            vector<string> newRhs = alpha;
            newRhs.push_back(Ap);
            res.productions.push_back({Ap, newRhs});
        }
        res.productions.push_back({Ap, {"eps"}});

        // Actualizar el mapa interno para que el algoritmo generalizado vea los cambios
        prods[A].clear();
        prods[Ap] = { {"eps"} }; // Simplificación para el flujo
    }
}

// ─────────────────────────────────────────────
// ELIMINAR RECURSIÓN IZQUIERDA GENERALIZADA
// Algoritmo Dragon Book 4.3.3 (Cubre indirecta)
// ─────────────────────────────────────────────
Grammar eliminateLeftRecursion(const Grammar& g) {
    Grammar result;
    result.startSymbol = g.startSymbol;
    result.terminals   = g.terminals;
    result.nonTerminals = g.nonTerminals;

    vector<string> order;
    map<string, vector<vector<string>>> prods;

    // Preservar orden original de los no-terminales
    for (auto& p : g.productions) {
        if (find(order.begin(), order.end(), p.lhs) == order.end()) {
            order.push_back(p.lhs);
        }
        prods[p.lhs].push_back(p.rhs);
    }

    for (int i = 0; i < (int)order.size(); i++) {
        for (int j = 0; j < i; j++) {
            string Ai = order[i];
            string Aj = order[j];

            vector<vector<string>> newAiProds;
            for (auto& rhsAi : prods[Ai]) {
                if (!rhsAi.empty() && rhsAi[0] == Aj) {
                    // Reemplazar Ai -> Aj gamma por Ai -> delta1 gamma | delta2 gamma...
                    vector<string> gamma(rhsAi.begin() + 1, rhsAi.end());
                    for (auto& rhsAj : prods[Aj]) {
                        vector<string> combined = rhsAj;
                        if (combined.size() == 1 && combined[0] == "eps") combined.clear();
                        combined.insert(combined.end(), gamma.begin(), gamma.end());
                        if (combined.empty()) combined.push_back("eps");
                        newAiProds.push_back(combined);
                    }
                } else {
                    newAiProds.push_back(rhsAi);
                }
            }
            prods[Ai] = newAiProds;
        }
        // Al final del ciclo interno, eliminar recursión directa de Ai
        eliminateDirectLeftRecursion(order[i], prods, result);
    }
    return result;
}

// ─────────────────────────────────────────────
// CONSTRUIR TABLA DE PARSING (Sin "rojo" de cerr)
// ─────────────────────────────────────────────
bool firstFollowConflict = false;

map<string, map<string, Production>> buildParsingTable(
    const Grammar& g,
    map<string, set<string>>& first,
    map<string, set<string>>& follow,
    bool& isLL1)
{
    map<string, map<string, Production>> table;
    isLL1 = true;
    firstFollowConflict = false; // Resetear bandera en cada construcción

    for (auto& prod : g.productions) {
        string A = prod.lhs;
        set<string> firstRhs;
        bool allEps = true;

        // 1. Calcular FIRST(derecha de la producción)
        if (prod.rhs[0] == "eps") {
            firstRhs.insert("eps");
        } else {
            for (auto& Y : prod.rhs) {
                // Si Y es terminal, su FIRST es él mismo. Si es NT, usamos el mapa.
                if (first.count(Y)) {
                    for (auto& f : first.at(Y)) if (f != "eps") firstRhs.insert(f);
                    if (!first.at(Y).count("eps")) { allEps = false; break; }
                } else {
                    // Es un terminal que no estaba en el mapa (caso raro)
                    firstRhs.insert(Y);
                    allEps = false;
                    break;
                }
            }
            if (allEps) firstRhs.insert("eps");
        }

        // 2. Regla A: Para cada terminal 'a' en FIRST(rhs), añadir A -> rhs a M[A, a]
        for (auto& a : firstRhs) {
            if (a == "eps") continue;

            // Si la casilla ya está ocupada, hay un conflicto
            if (table[A].count(a)) {
                isLL1 = false;
            }
            table[A][a] = prod;
        }

        // 3. Regla B: Si 'eps' está en FIRST(rhs), para cada terminal 'b' en FOLLOW(A)
        if (firstRhs.count("eps")) {
            for (auto& b : follow.at(A)) {

                // DETECCIÓN DE CONFLICTO FIRST/FOLLOW:
                // Si intentamos meter la producción por ser anulable (Regla B),
                // pero esa casilla ya fue llenada por una producción que empieza
                // con ese mismo terminal (Regla A).
                if (table[A].count(b)) {
                    isLL1 = false;
                    firstFollowConflict = true; // Activamos la razón específica
                }
                table[A][b] = prod;
            }
        }
    }
    return table;
}

// ─────────────────────────────────────────────
// LEFT FACTORING
// Algoritmo del Dragon Book Sección 4.3
//
// Para cada A con producciones A → α β₁ | α β₂:
//   Se reemplaza por:
//     A  → α A'
//     A' → β₁ | β₂
//
// Funciona para CUALQUIER gramática
// ─────────────────────────────────────────────
Grammar leftFactoring(const Grammar& g) {
    Grammar current = g;
    bool changed = true;
    int counter = 1;

    while (changed) {
        changed = false;
        Grammar next;
        next.startSymbol = current.startSymbol;
        next.terminals = current.terminals;
        next.nonTerminals = current.nonTerminals;

        map<string, vector<vector<string>>> prodsByLhs;
        for (auto& p : current.productions) prodsByLhs[p.lhs].push_back(p.rhs);

        for (auto& entry : prodsByLhs) {
            string A = entry.first;
            vector<vector<string>> alts = entry.second;

            // 1. Eliminar duplicados exactos
            sort(alts.begin(), alts.end());
            alts.erase(unique(alts.begin(), alts.end()), alts.end());

            // 2. Buscar el prefijo común más largo entre cualquier par de producciones
            vector<string> longestPrefix;
            for (size_t i = 0; i < alts.size(); ++i) {
                for (size_t j = i + 1; j < alts.size(); ++j) {
                    if (alts[i][0] == "eps" || alts[j][0] == "eps") continue;

                    vector<string> common;
                    for (size_t k = 0; k < min(alts[i].size(), alts[j].size()); ++k) {
                        if (alts[i][k] == alts[j][k]) common.push_back(alts[i][k]);
                        else break;
                    }
                    if (common.size() > longestPrefix.size()) longestPrefix = common;
                }
            }

            if (!longestPrefix.empty()) {
                changed = true;
                string NewNT = A + "p" + to_string(counter++);
                next.nonTerminals.insert(NewNT);

                // A -> prefix NewNT
                vector<string> head = longestPrefix;
                head.push_back(NewNT);
                next.productions.push_back({A, head});

                // Producciones que no tenían el prefijo se mantienen
                // Producciones con el prefijo se van al NewNT
                for (auto& alt : alts) {
                    bool match = (alt.size() >= longestPrefix.size());
                    for (size_t k = 0; k < longestPrefix.size() && match; ++k) {
                        if (alt[k] != longestPrefix[k]) match = false;
                    }

                    if (match) {
                        vector<string> rest(alt.begin() + longestPrefix.size(), alt.end());
                        if (rest.empty()) rest = {"eps"};
                        next.productions.push_back({NewNT, rest});
                    } else {
                        next.productions.push_back({A, alt});
                    }
                }
            } else {
                for (auto& alt : alts) next.productions.push_back({A, alt});
            }
        }
        current = next;
    }
    return current;
}

// ─────────────────────────────────────────────
// CALCULAR FIRST
// Algoritmo Dragon Book Sección 4.4.2
//
// FIRST(X) = conjunto de terminales con los que
// puede comenzar cualquier cadena derivada de X
// ─────────────────────────────────────────────
map<string, set<string>> computeFirst(const Grammar& g) {
    map<string, set<string>> first;

    // FIRST de terminales = él mismo
    for (auto& t : g.terminals) first[t].insert(t);
    first["eps"].insert("eps");

    // Inicializar no-terminales vacíos
    for (auto& nt : g.nonTerminals) first[nt] = {};

    // Punto fijo: repetir hasta que no haya cambios
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& prod : g.productions) {
            string A   = prod.lhs;
            auto&  rhs = prod.rhs;

            if (rhs[0] == "eps") {
                // A → ε → agregar ε a FIRST(A)
                if (first[A].insert("eps").second) changed = true;
                continue;
            }

            // Para Y1 Y2 ... Yk:
            // agregar FIRST(Yi) - {ε} a FIRST(A)
            // si ε ∈ FIRST(Yi), continuar con Yi+1
            bool allEps = true;
            for (auto& Y : rhs) {
                for (auto& f : first[Y]) {
                    if (f != "eps")
                        if (first[A].insert(f).second) changed = true;
                }
                if (!first[Y].count("eps")) {
                    allEps = false;
                    break;
                }
            }
            // Si todos los Yi pueden derivar ε → ε ∈ FIRST(A)
            if (allEps)
                if (first[A].insert("eps").second) changed = true;
        }
    }
    return first;
}

// ─────────────────────────────────────────────
// CALCULAR FOLLOW
// Algoritmo Dragon Book Sección 4.4.2
//
// FOLLOW(A) = conjunto de terminales que pueden
// aparecer inmediatamente después de A
// ─────────────────────────────────────────────
map<string, set<string>> computeFollow(
    const Grammar& g,
    map<string, set<string>>& first)
{
    map<string, set<string>> follow;
    for (auto& nt : g.nonTerminals) follow[nt] = {};

    // Regla 1: $ ∈ FOLLOW del símbolo inicial
    follow[g.startSymbol].insert("$");

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& prod : g.productions) {
            string A   = prod.lhs;
            auto&  rhs = prod.rhs;

            for (int i = 0; i < (int)rhs.size(); i++) {
                string B = rhs[i];
                if (!g.nonTerminals.count(B)) continue;

                // Regla 2: A → α B β
                // FIRST(β) - {ε} ⊆ FOLLOW(B)
                bool betaAllEps = true;
                for (int j = i+1; j < (int)rhs.size(); j++) {
                    for (auto& f : first[rhs[j]]) {
                        if (f != "eps")
                            if (follow[B].insert(f).second) changed = true;
                    }
                    if (!first[rhs[j]].count("eps")) {
                        betaAllEps = false;
                        break;
                    }
                }

                // Regla 3: A → α B  o  β →* ε
                // FOLLOW(A) ⊆ FOLLOW(B)
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
// IMPRIMIR GRAMÁTICA
// ─────────────────────────────────────────────
void printGrammar(const Grammar& g) {
    set<string> printed;
    for (auto& prod : g.productions) {
        if (printed.count(prod.lhs)) continue;
        printed.insert(prod.lhs);
        cout << "  " << left << setw(12) << prod.lhs << " -> ";
        bool firstAlt = true;
        for (auto& p : g.productions) {
            if (p.lhs != prod.lhs) continue;
            if (!firstAlt) cout << " | ";
            for (int i = 0; i < (int)p.rhs.size(); i++) {
                if (i) cout << " ";
                cout << p.rhs[i];
            }
            firstAlt = false;
        }
        cout << "\n";
    }
}

// ─────────────────────────────────────────────
// IMPRIMIR TABLA FIRST Y FOLLOW
// ─────────────────────────────────────────────
void printFirstFollow(
    const Grammar& g,
    map<string, set<string>>& first,
    map<string, set<string>>& follow)
{
    cout << "  " << left
         << setw(14) << "Non-Terminal"
         << "| " << setw(28) << "FIRST"
         << "| FOLLOW\n";
    cout << "  " << string(14,'-') << "+-"
         << string(28,'-') << "+-"
         << string(20,'-') << "\n";

    // Imprimir en orden de aparición en las producciones
    vector<string> order;
    set<string> seen;
    for (auto& p : g.productions) {
        if (!seen.count(p.lhs)) {
            order.push_back(p.lhs);
            seen.insert(p.lhs);
        }
    }

    for (auto& nt : order) {
        string f = "{ ";
        for (auto& s : first.at(nt))  f  += s + " ";
        f += "}";
        string fo = "{ ";
        for (auto& s : follow.at(nt)) fo += s + " ";
        fo += "}";
        cout << "  " << setw(14) << nt
             << "| " << setw(28) << f
             << "| " << fo << "\n";
    }
}

// ─────────────────────────────────────────────
// IMPRIMIR TABLA DE PARSING M[A][a]
// ─────────────────────────────────────────────
void printParsingTable(
    const Grammar& g,
    map<string, map<string, Production>>& table)
{
    // Recopilar terminales usados en la tabla
    set<string> usedTerminals;
    for (auto& row : table)
        for (auto& col : row.second)
            usedTerminals.insert(col.first);

    // Cabecera
    cout << "  " << setw(14) << "Non-Terminal";
    for (auto& t : usedTerminals)
        cout << "| " << setw(18) << t;
    cout << "\n";
    cout << "  " << string(14,'-');
    for (size_t i=0; i<usedTerminals.size(); i++)
        cout << "+-" << string(18,'-');
    cout << "\n";

    // Filas — en orden de aparición
    vector<string> order;
    set<string> seen;
    for (auto& p : g.productions) {
        if (!seen.count(p.lhs)) {
            order.push_back(p.lhs);
            seen.insert(p.lhs);
        }
    }

    for (auto& nt : order) {
        cout << "  " << setw(14) << nt;
        for (auto& t : usedTerminals) {
            if (table.count(nt) && table.at(nt).count(t)) {
                auto& prod = table.at(nt).at(t);
                string cell = prod.lhs + "->";
                for (auto& s : prod.rhs) cell += s + " ";
                if (cell.size() > 17) cell = cell.substr(0,14) + "...";
                cout << "| " << setw(18) << cell;
            } else {
                cout << "| " << setw(18) << " ";
            }
        }
        cout << "\n";
    }
}
