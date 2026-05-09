#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "Analyzer.h"
#include "Grammar.h"

using namespace std;

void showGrammarTransformation() { // Muestra la transformación de la Gramática del Proyecto
    cout << endl;
    cout << "========================================" << endl;
    cout << "  GRAMMAR TRANSFORMATION" << endl;
    cout << "========================================" << endl;

    // ── 1. Gramática original ──
    cout << endl;
    cout << "1. ORIGINAL GRAMMAR (from project)" << endl;
    cout << "----------------------------------------" << endl;
    cout << "  Program  -> RuleList" << endl;
    cout << "  RuleList -> Rule RuleList | ε" << endl;
    cout << "  Rule     -> rule id : if Cond then Action" << endl;
    cout << "  Cond     -> Cond AND Cond | Atom" << endl;
    cout << "  Atom     -> id RelOp value | id" << endl;
    cout << "  RelOp    -> > | < | =" << endl;
    cout << "  Action   -> id" << endl;

    // ── 2. Problema: recursión izquierda ──
    cout << endl;
    cout << "2. PROBLEM: LEFT RECURSION" << endl; // Explica el problema
    cout << "----------------------------------------" << endl;
    cout << "  Cond -> Cond AND Cond" << endl;
    cout << " 'Cond' appears at the start of its own rule." << endl;
    cout << "  This causes an infinite loop in LL(1) parsers." << endl;

    // ── 3. Eliminación de recursión izquierda ──
    cout << endl;
    cout << "3. LEFT RECURSION ELIMINATION" << endl;
    cout << "----------------------------------------" << endl;
    cout << "  Formula:  A -> Aα | ß" << endl;
    cout << "  Becomes:  A  -> ßA'" << endl;
    cout << "            A' -> αA' | ε" << endl;
    cout << endl;
    cout << "  Applied To Production:" << endl;
    cout << "  Cond -> Cond AND Cond | Atom" << endl;
    cout << "  Becomes:" << endl;
    cout << "  Cond     -> Atom CondRest" << endl;
    cout << "  CondRest -> AND Atom CondRest | ε" << endl;

    // ── 4. Factorización por izquierda ──
    cout << endl;
    cout << "4. LEFT FACTORING" << endl;
    cout << "----------------------------------------" << endl;
    cout << "  Atom -> id RelOp value | id" << endl;
    cout << "  Both productions start with 'id' -> ambiguous." << endl;
    cout << "  Solution: " << endl;
    cout << "  Formula:   A → αβ₁ | αβ₂" << endl;
    cout << "  Becomes:   A -> αA'" << endl;
    cout << "             A'-> β₁ | β₂'" << endl;
    cout << "  Applied To Production:" << endl;
    cout << "  Atom -> id RelOp value | id" << endl;
    cout << "  Becomes:" << endl;
    cout << "  Atom     -> id Atom'" << endl;
    cout << "  Atom'    -> RelOp value | ε" << endl;

    // ── 5. Gramática transformada ──
    cout << endl;
    cout << "5. TRANSFORMED GRAMMAR (LL(1))" << endl;
    cout << "----------------------------------------" << endl;
    cout << "  Program  ->  RuleList" << endl;
    cout << "  RuleList ->  Rule RuleList | ε" << endl;
    cout << "  Rule     ->  rule id : if Cond then Action" << endl;
    cout << "  Cond     ->  Atom CondRest" << endl;
    cout << "  CondRest ->  AND Atom CondRest | ε" << endl;
    cout << "  Atom     ->  id Atom' " << endl;
    cout << "  Atom'    ->  RelOp value | ε" << endl;
    cout << "  RelOp    ->  > | < | =" << endl;
    cout << "  Action   ->  id" << endl;

    // ── 6. FIRST y FOLLOW ──
    cout << endl;
    cout << "6. FIRST AND FOLLOW SETS" << endl;
    cout << "----------------------------------------------------------" << endl;
    cout << "  Non-Terminal  |        FIRST         |      FOLLOW" << endl;
    cout << "  --------------|----------------------|------------------" << endl;
    cout << "  Program       | { rule, ε }          | { $ }" << endl;
    cout << "  RuleList      | { rule, ε }          | { $ }" << endl;
    cout << "  Rule          | { rule }             | { rule, $ }" << endl;
    cout << "  Cond          | { id }               | { then }" << endl;
    cout << "  CondRest      | { AND, ε }           | { then }" << endl;
    cout << "  Atom          | { id }               | { AND, then }" << endl;
    cout << "  Atom'         | { >, <, =, ε }       | { AND, then }" << endl;
    cout << "  RelOp         | { >, <, = }          | { value }" << endl;
    cout << "  Action        | { id }               | { rule, $ }" << endl;

    // ── 7. Tabla de parsing ──
    cout << endl;
    cout << "7. PREDICTIVE PARSING TABLE" << endl;
    cout << "----------------------------------------" << endl;
    cout << "  M[A, a] = production to use when non-terminal is A" << endl;
    cout << "            and current token is a" << endl;
    cout << endl;
    cout << "  Non-Terminal  | rule          | id            | AND               | then  | >   | <   | =   | $" << endl;
    cout << "  --------------|---------------|---------------|-------------------|-------|-----|-----|-----|--------" << endl;
    cout << "  Program       | RuleList      |               |                   |       |     |     |     | RuleList" << endl;
    cout << "  RuleList      | Rule RuleList |               |                   |       |     |     |     | eps" << endl;
    cout << "  Rule          | rule id:if..  |               |                   |       |     |     |     |" << endl;
    cout << "  Cond          |               | Atom CondRest |                   |       |     |     |     |" << endl;
    cout << "  CondRest      |               |               | AND Atom CondRest |   ε   |     |     |     |" << endl;
    cout << "  Atom          |               | id (lookahead)|                   |       |     |     |     |" << endl;
    cout << "  RelOp         |               |               |                   |       |  >  |  <  |  =  |" << endl;
    cout << "  Action        |               | id            |                   |       |     |     |     |" << endl;
    cout << endl;
}

// ─────────────────────────────────────────────────────────────────
// FUNCIÓN PRINCIPAL DE PROCESAMIENTO
// Conecta todos los módulos: lexer → parser → interpreter → analyzer
// ─────────────────────────────────────────────────────────────────

void showGrammarTool() {
    cout << endl;
    cout << "============================================" << endl;
    cout << "  GRAMMAR TRANSFORMATION TOOL              " << endl;
    cout << "============================================" << endl;
    cout << endl;
    cout << "FORMAT TO WRITE YOUR GRAMMAR:" << endl;
    cout << "  - Non-terminals : start with UPPERCASE   (Program, Cond)" << endl;
    cout << "  - Terminals     : start with lowercase   (rule, if, id)" << endl;
    cout << "                    or symbols             (>, <, =, :)" << endl;
    cout << "  - Epsilon       : write as               eps" << endl;
    cout << "  - Productions   : use ->  between sides" << endl;
    cout << "  - Alternatives  : use |   to separate" << endl;
    cout << "  - Start symbol  : must be the first line" << endl;
    cout << endl;
    cout << "EXAMPLE:" << endl;
    cout << "  Program -> RuleList" << endl;
    cout << "  RuleList -> Rule RuleList | eps" << endl;
    cout << "  Cond -> Cond AND Cond | Atom" << endl;
    cout << "  Atom -> id RelOp value | id" << endl;
    cout << "  RelOp -> > | < | =" << endl;
    cout << endl;
    cout << "Write your grammar. When done, write --- on a new line:" << endl;
    cout << "--------------------------------------------" << endl;

    string line;
    stringstream buffer;
    while (getline(cin, line)) {
        if (line == "---") break;
        buffer << line << "\n";
    }

    string grammarInput = buffer.str();
    if (grammarInput.empty()) {
        cout << "No grammar entered." << endl;
        return;
    }

    // 1. Parsear la gramática original
    Grammar original = parseGrammar(grammarInput);

    cout << endl;
    cout << "1. ORIGINAL GRAMMAR:" << endl;
    printGrammar(original);

    // 2. Eliminar recursión izquierda
    Grammar noLeftRec = eliminateLeftRecursion(original);

    cout << endl;
    cout << "2. AFTER LEFT RECURSION ELIMINATION:" << endl;
    printGrammar(noLeftRec);

    // 3. Left Factoring
    Grammar factored = leftFactoring(noLeftRec);

    cout << endl;
    cout << "3. AFTER LEFT FACTORING:" << endl;
    printGrammar(factored);

    // 4. FIRST y FOLLOW
    auto first  = computeFirst(factored);
    auto follow = computeFollow(factored, first);

    cout << endl;
    cout << "4. FIRST AND FOLLOW SETS:" << endl;
    printFirstFollow(factored, first, follow);

    // 5. Tabla de parsing
    auto table = buildParsingTable(factored, first, follow);

    cout << endl;
    cout << "5. PARSING TABLE M[A][a]:" << endl;
    // Recopilar todos los terminales que aparecen en la tabla
    set<string> usedTerminals;
    for (auto& row : table) {
        for (auto& col : row.second) {
            usedTerminals.insert(col.first);
        }
    }

    cout << "  " << setw(12) << "Non-Terminal";
    for (auto& t : usedTerminals) cout << " | " << setw(10) << t;
    cout << endl;

    for (auto& nt : factored.nonTerminals) {
        cout << "  " << setw(12) << nt;
        for (auto& t : usedTerminals) {
            if (table[nt].count(t)) {
                // Mostrar la producción
                string prod = nt + "->";
                for (auto& s : table[nt][t].rhs) prod += s + " ";
                cout << " | " << setw(10) << prod;
            } else {
                cout << " | " << setw(10) << "";
            }
        }
        cout << endl;
    }
}

void processAndShow(string input) {

    // ── Análisis léxico ──
    vector<Token> tokens;
    try {
        tokens = tokenize(input);
    } catch (runtime_error& e) {
        cerr << "Lexical error: " << e.what() << endl;
        return;
    }

    // ── Análisis sintáctico ──
    ParseResult parsed;
    try {
        parsed = parse(tokens);
    } catch (runtime_error& e) {
        cerr << "Syntax error: " << e.what() << endl;
        return;
    }

    // ── Ejecución ──
    set<string> activatedFacts = interpret(
        parsed.program,
        parsed.variables,
        parsed.facts
    );

    // ── Análisis estático ──
    AnalysisResult analysis = analyze(
        parsed.program,
        parsed.variables,
        parsed.facts
    );

    // ── Output — formato exacto del PDF sección 2.6 ──
    if (activatedFacts.empty()) {
        cout << "(no output)" << endl;
    } else {
        for (const string& fact : activatedFacts) {
            cout << fact << endl;
        }
    }

    // ── Análisis estático — mensajes ──
    bool hasAnalysis = !analysis.conflicts.empty()    ||
                       !analysis.redundancies.empty() ||
                       !analysis.inactiveRules.empty();

    if (hasAnalysis) {
        cout << "Analysis:" << endl;

        for (auto& entry : analysis.conflicts) {
            cout << "Action " << entry.first << " generated by ";
            vector<string>& rules = entry.second;
            for (int i = 0; i < (int)rules.size(); i++) {
                cout << rules[i];
                if (i < (int)rules.size() - 1) cout << ", ";
            }
            cout << endl;
        }

        for (auto& group : analysis.redundancies) {
            cout << "Redundant rules: ";
            for (int i = 0; i < (int)group.size(); i++) {
                cout << group[i];
                if (i < (int)group.size() - 1) cout << ", ";
            }
            cout << endl;
        }

        for (const string& ruleName : analysis.inactiveRules) {
            cout << "Potentially inactive rule: " << ruleName << endl;
        }
    }
}



// ─────────────────────────────────────────────────────────────────
// MENÚ PRINCIPAL
// ─────────────────────────────────────────────────────────────────

int main() {
    while (true) {
        string input;
        cout << endl;
        cout << "==========================================" << endl;
        cout << "  Rule-Based Language Interpreter" << endl;
        cout << "==========================================" << endl;
        cout << "  1. Type or paste a program" << endl;
        cout << "  2. Load from a .txt file" << endl;
        cout << "  3. Show grammar transformation" << endl;
        cout << "  4. Exit" << endl;
        cout << "Your choice: ";

        int option;
        cin >> option;
        cin.ignore();

        if (option == 1) {
            cout << endl;
            cout << "Write or copy the program." << endl;
            cout << "When finished, write '---' on a new line." << endl;
            string line;
            stringstream buffer;
            while (getline(cin, line)) {
                if (line == "---") break;
                buffer << line << "\n";
            }
            input = buffer.str();
            processAndShow(input);

        } else if (option == 2) {
            cout << "Enter the file path: ";
            string filepath;
            getline(cin, filepath);
            ifstream file(filepath);
            if (!file.is_open()) {
                cerr << "Error: could not open file '" << filepath << "'" << endl;
                continue;
            }
            stringstream buffer;
            buffer << file.rdbuf();
            input = buffer.str();
            file.close();
            processAndShow(input);

        } else if (option == 3) {
            showGrammarTransformation();

        } else if (option == 4) {
            cout << "Goodbye!" << endl;
            break;

        } else {
            cerr << "Invalid option. Please choose 1, 2, 3 or 4." << endl;
        }
    }

    return 0;
}