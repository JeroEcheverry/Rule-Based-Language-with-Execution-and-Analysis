//
// Created by Sofía Marín Bustamante on 8/05/26.
//

#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <iostream>
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
Grammar eliminateLeftRecursion(const Grammar& g) {
    Grammar result;
    result.startSymbol = g.startSymbol;
    result.terminals   = g.terminals;

    // Mantener orden de aparición de los no-terminales
    vector<string> order;
    map<string, vector<vector<string>>> prods;
    for (auto& p : g.productions) {
        if (!prods.count(p.lhs)) order.push_back(p.lhs);
        prods[p.lhs].push_back(p.rhs);
    }

    for (auto& A : order) {
        vector<vector<string>> alphas; // A → A α → guardamos α
        vector<vector<string>> betas;  // A → β  (sin recursión)

        for (auto& rhs : prods[A]) {
            if (!rhs.empty() && rhs[0] == A) {
                // Recursión izquierda: guardar α (todo menos el A inicial)
                alphas.push_back(vector<string>(rhs.begin()+1, rhs.end()));
            } else {
                betas.push_back(rhs);
            }
        }

        result.nonTerminals.insert(A);

        if (alphas.empty()) {
            // Sin recursión → copiar igual
            for (auto& rhs : betas) {
                result.productions.push_back({A, rhs});
            }
        } else {
            // Hay recursión → transformar
            string Ap = A + "'"; // nuevo no-terminal A'
            result.nonTerminals.insert(Ap);

            // A → β A'  para cada β
            for (auto& beta : betas) {
                vector<string> newRhs = beta;
                if (newRhs.size()==1 && newRhs[0]=="eps")
                    newRhs = {Ap};
                else
                    newRhs.push_back(Ap);
                result.productions.push_back({A, newRhs});
            }

            // A' → α A'  para cada α
            for (auto& alpha : alphas) {
                vector<string> newRhs = alpha;
                newRhs.push_back(Ap);
                result.productions.push_back({Ap, newRhs});
            }

            // A' → ε
            result.productions.push_back({Ap, {"eps"}});
        }
    }
    return result;
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
    Grammar result;
    result.startSymbol  = g.startSymbol;
    result.terminals    = g.terminals;
    result.nonTerminals = g.nonTerminals;

    vector<string> order;
    map<string, vector<vector<string>>> prods;
    for (auto& p : g.productions) {
        if (!prods.count(p.lhs)) order.push_back(p.lhs);
        prods[p.lhs].push_back(p.rhs);
    }

    int counter = 1;

    // Cola de no-terminales a procesar (puede crecer con nuevos A')
    vector<string> toProcess = order;
    set<string> processed;

    while (!toProcess.empty()) {
        string A = toProcess.front();
        toProcess.erase(toProcess.begin());

        if (processed.count(A)) continue;
        processed.insert(A);

        auto rhsList = prods[A];
        bool changed = true;

        while (changed) {
            changed = false;

            // Agrupar por primer símbolo
            map<string, vector<vector<string>>> byFirst;
            for (auto& rhs : rhsList) {
                string first = rhs.empty() ? "eps" : rhs[0];
                byFirst[first].push_back(rhs);
            }

            vector<vector<string>> newRhsList;

            for (auto& entry : byFirst) {
                auto& group = entry.second;

                if (group.size() == 1) {
                    // Solo una producción con este primer símbolo → no factorizar
                    newRhsList.push_back(group[0]);
                } else {
                    // Múltiples producciones con el mismo primer símbolo
                    // Encontrar el prefijo común MÁS LARGO
                    vector<string> prefix = group[0];
                    for (int k = 1; k < (int)group.size(); k++) {
                        vector<string> common;
                        int minLen = min(prefix.size(), group[k].size());
                        for (int j = 0; j < minLen; j++) {
                            if (prefix[j] == group[k][j])
                                common.push_back(prefix[j]);
                            else
                                break;
                        }
                        prefix = common;
                    }

                    if (prefix.empty()) {
                        // No hay prefijo común real → copiar sin cambios
                        for (auto& rhs : group) newRhsList.push_back(rhs);
                        continue;
                    }

                    // Factorizar el prefijo completo
                    changed = true;
                    string Ap = A + "'" + to_string(counter++);
                    result.nonTerminals.insert(Ap);

                    // A → prefix A'
                    vector<string> newProd = prefix;
                    newProd.push_back(Ap);
                    newRhsList.push_back(newProd);

                    // A' → resto de cada alternativa
                    vector<vector<string>> apRhsList;
                    for (auto& rhs : group) {
                        vector<string> rest(rhs.begin() + prefix.size(), rhs.end());
                        if (rest.empty()) rest = {"eps"};
                        apRhsList.push_back(rest);
                        result.productions.push_back({Ap, rest});
                    }

                    // Agregar A' a la cola para procesarlo también
                    prods[Ap] = apRhsList;
                    toProcess.push_back(Ap);
                }
            }
            rhsList = newRhsList;
        }

        for (auto& rhs : rhsList) {
            result.productions.push_back({A, rhs});
        }
    }
    return result;
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
// CONSTRUIR TABLA DE PARSING M[A][a]
// Algoritmo Dragon Book Sección 4.4.3
//
// Para cada producción A → α:
//   Para cada terminal a ∈ FIRST(α) - {ε}: M[A][a] = A → α
//   Si ε ∈ FIRST(α): para cada b ∈ FOLLOW(A): M[A][b] = A → α
// ─────────────────────────────────────────────
map<string, map<string, Production>> buildParsingTable(
    const Grammar& g,
    map<string, set<string>>& first,
    map<string, set<string>>& follow,
    bool& isLL1)  // ← retorna si es LL(1) o no
{
    map<string, map<string, Production>> table;
    isLL1 = true; // asumimos que es LL(1) hasta encontrar conflicto

    for (auto& prod : g.productions) {
        string A   = prod.lhs;
        auto&  rhs = prod.rhs;

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

        // M[A][a] para a ∈ FIRST(rhs) - {ε}
        for (auto& a : firstRhs) {
            if (a == "eps") continue;

            if (table[A].count(a)) {
                // ── CONFLICTO DETECTADO ──────────────────
                // Hay dos producciones para M[A][a]
                // Esto viola la condición LL(1)
                for (auto& s : table[A][a].rhs) cerr << " " << s;
                for (auto& s : prod.rhs) cerr << " " << s;
                isLL1 = false;
            }
            table[A][a] = prod; // última producción gana
        }

        // Si ε ∈ FIRST(rhs) → M[A][b] para b ∈ FOLLOW(A)
        if (firstRhs.count("eps")) {
            for (auto& b : follow[A]) {
                if (table[A].count(b)) {
                    for (auto& s : table[A][b].rhs) cerr << " " << s;
                    for (auto& s : prod.rhs) cerr << " " << s;
                    isLL1 = false;
                }
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
