#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "Analyzer.h"

using namespace std;

// ─────────────────────────────────────────────────────────────────
// OPCIÓN 3 DEL MENÚ: Transformación de gramática
// Muestra paso a paso:
//   1. La gramática original del PDF
//   2. La eliminación de recursión izquierda
//   3. La factorización por izquierda
//   4. Los conjuntos FIRST y FOLLOW
//   5. La tabla de parsing predictivo
// ─────────────────────────────────────────────────────────────────

void showGrammarTransformation() {
    cout << endl;
    cout << "========================================" << endl;
    cout << "  GRAMMAR TRANSFORMATION" << endl;
    cout << "========================================" << endl;

    // ── 1. Gramática original ──
    cout << endl;
    cout << "1. ORIGINAL GRAMMAR (from project spec)" << endl;
    cout << "----------------------------------------" << endl;
    cout << "  Program  -> RuleList" << endl;
    cout << "  RuleList -> Rule RuleList | epsilon" << endl;
    cout << "  Rule     -> rule id : if Cond then Action" << endl;
    cout << "  Cond     -> Cond AND Cond | Atom" << endl;
    cout << "  Atom     -> id RelOp value | id" << endl;
    cout << "  RelOp    -> > | < | =" << endl;
    cout << "  Action   -> id" << endl;

    // ── 2. Problema: recursión izquierda ──
    cout << endl;
    cout << "2. PROBLEM: LEFT RECURSION" << endl;
    cout << "----------------------------------------" << endl;
    cout << "  Cond -> Cond AND Cond" << endl;
    cout << "  'Cond' appears at the start of its own rule." << endl;
    cout << "  This causes an infinite loop in LL(1) parsers." << endl;

    // ── 3. Eliminación de recursión izquierda ──
    cout << endl;
    cout << "3. LEFT RECURSION ELIMINATION" << endl;
    cout << "----------------------------------------" << endl;
    cout << "  Formula:  A -> A alpha | beta" << endl;
    cout << "  Becomes:  A  -> beta A'" << endl;
    cout << "            A' -> alpha A' | epsilon" << endl;
    cout << endl;
    cout << "  Applied:" << endl;
    cout << "  Cond -> Cond AND Cond | Atom" << endl;
    cout << "  Becomes:" << endl;
    cout << "  Cond     -> Atom CondRest" << endl;
    cout << "  CondRest -> AND Atom CondRest | epsilon" << endl;

    // ── 4. Factorización por izquierda ──
    cout << endl;
    cout << "4. LEFT FACTORING" << endl;
    cout << "----------------------------------------" << endl;
    cout << "  Atom -> id RelOp value | id" << endl;
    cout << "  Both productions start with 'id' -> ambiguous." << endl;
    cout << "  Solution: use lookahead(1) to decide:" << endl;
    cout << "    if next token is >, < or = -> Atom -> id RelOp value" << endl;
    cout << "    if next token is AND or then -> Atom -> id" << endl;

    // ── 5. Gramática transformada ──
    cout << endl;
    cout << "5. TRANSFORMED GRAMMAR (LL(1))" << endl;
    cout << "----------------------------------------" << endl;
    cout << "  Program  -> RuleList" << endl;
    cout << "  RuleList -> Rule RuleList | epsilon" << endl;
    cout << "  Rule     -> rule id : if Cond then Action" << endl;
    cout << "  Cond     -> Atom CondRest" << endl;
    cout << "  CondRest -> AND Atom CondRest | epsilon" << endl;
    cout << "  Atom     -> id RelOp value | id  (resolved by lookahead)" << endl;
    cout << "  RelOp    -> > | < | =" << endl;
    cout << "  Action   -> id" << endl;

    // ── 6. FIRST y FOLLOW ──
    cout << endl;
    cout << "6. FIRST AND FOLLOW SETS" << endl;
    cout << "----------------------------------------" << endl;
    cout << "  Non-Terminal  | FIRST               | FOLLOW" << endl;
    cout << "  --------------|---------------------|------------------" << endl;
    cout << "  Program       | { rule, eps }        | { $ }" << endl;
    cout << "  RuleList      | { rule, eps }        | { $ }" << endl;
    cout << "  Rule          | { rule }             | { rule, $ }" << endl;
    cout << "  Cond          | { id }               | { then }" << endl;
    cout << "  CondRest      | { AND, eps }         | { then }" << endl;
    cout << "  Atom          | { id }               | { AND, then }" << endl;
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
    cout << "  CondRest      |               |               | AND Atom CondRest | eps   |     |     |     |" << endl;
    cout << "  Atom          |               | id (lookahead)|                   |       |     |     |     |" << endl;
    cout << "  RelOp         |               |               |                   |       |  >  |  <  |  =  |" << endl;
    cout << "  Action        |               | id            |                   |       |     |     |     |" << endl;
    cout << endl;
}

// ─────────────────────────────────────────────────────────────────
// FUNCIÓN PRINCIPAL DE PROCESAMIENTO
// Conecta todos los módulos: lexer → parser → interpreter → analyzer
// ─────────────────────────────────────────────────────────────────

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