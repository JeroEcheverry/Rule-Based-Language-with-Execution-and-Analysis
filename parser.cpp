#include "parser.h"
#include "grammar.h"
#include <stdexcept>
#include <stack>
#include <iostream>

using namespace std;

//GRAMMAR AND LL(1) TABLE
// WHAT THIS SECTION DOES:
//   Builds the project grammar starting from the ORIGINAL form
//   (with left recursion), then automatically transforms it to
//   LL(1) using algorithms from the Dragon Book (Section 4.3).


// buildProjectGrammar()
//
// Builds the project grammar in three steps:
//
//   Step 1: Define the ORIGINAL grammar (with left recursion)
//     Cond → Cond AND Atom | Atom   ← left recursion
//     Atom → id RelOp value | id    ← common prefix
//
//   Step 2: Eliminate left recursion (Dragon Book Section 4.3)
//     Cond → Cond AND Atom | Atom
//     becomes:
//       Cond  → Atom Cond'
//       Cond' → AND Atom Cond' | eps
//
//   Step 3: Apply left factoring (Dragon Book Section 4.3)
//     Atom → id RelOp value | id
//     becomes:
//       Atom  → id Atom'
//       Atom' → RelOp value | eps

Grammar buildProjectGrammar() {

    // Step 1: Original grammar with left recursion
    Grammar original;
    original.startSymbol = "Program";

    original.nonTerminals = {
        "Program","RuleList","Rule",
        "Cond","Atom","RelOp","Action"
    };
    original.terminals = {
        "rule","if","then","AND",
        ":","State",">","<","=",
        "id","value","$"
    };

    // Program → RuleList
    original.productions.push_back({"Program",  {"RuleList"}});

    // RuleList → Rule RuleList | eps
    original.productions.push_back({"RuleList", {"Rule","RuleList"}});
    original.productions.push_back({"RuleList", {"eps"}});

    // Rule → rule id : if Cond then Action
    original.productions.push_back({"Rule",
        {"rule","id",":","if","Cond","then","Action"}});

    // Cond → Cond AND Atom | Atom  ← LEFT RECURSION
    // A=Cond, alpha=AND Atom, beta=Atom
    original.productions.push_back({"Cond", {"Cond","AND","Atom"}});
    original.productions.push_back({"Cond", {"Atom"}});

    // Atom → id RelOp value | id  ← COMMON PREFIX
    original.productions.push_back({"Atom", {"id","RelOp","value"}});
    original.productions.push_back({"Atom", {"id"}});

    // RelOp → > | < | =
    original.productions.push_back({"RelOp", {">"}});
    original.productions.push_back({"RelOp", {"<"}});
    original.productions.push_back({"RelOp", {"="}});

    // Action → id
    original.productions.push_back({"Action", {"id"}});

    // Step 2: Automatically eliminate left recursion
    Grammar noRecursion = eliminateLeftRecursion(original);

    // Step 3: Automatically apply left factoring
    Grammar transformed = leftFactoring(noRecursion);

    // Add State as a terminal so the validator handles it correctly
    transformed.terminals.insert("State");

    return transformed;
}

// tokenToGrammar()
// Converts a lexer token type to the string symbol used
// in the grammar. Needed to look up entries in the parsing table
// M[NonTerminal][terminal].

string tokenToGrammar(const Token& t) {
    switch (t.type) {
        case TokenType::RULE:   return "rule";
        case TokenType::IF:     return "if";
        case TokenType::THEN:   return "then";
        case TokenType::AND:    return "AND";
        case TokenType::STATE:  return "State";
        case TokenType::COLON:  return ":";
        case TokenType::GT:     return ">";
        case TokenType::LT:     return "<";
        case TokenType::EQ:     return "=";
        case TokenType::ID:     return "id";
        case TokenType::NUMBER: return "value";
        case TokenType::END:    return "$";
        default:                return "$";
    }
}

//LL(1) VALIDATION (Figure 4.20 + Panic Mode)
//
// WHAT THIS SECTION DOES:
//   Implements the predictive parsing algorithm from Figure 4.20
//   of the Dragon Book, with Panic Mode error recovery (Section 4.4.5).

// validateWithLL1()
//
// Implements Figure 4.20 of the Dragon Book.
// Runs SILENTLY — prints nothing, only returns true/false.
// Used internally by parse() to validate syntax.

// PARAMETERS:
//   tokens  → token list from the lexer
//   grammar → the LL(1) grammar (already transformed)
//   table   → predictive parsing table M[NonTerminal][terminal]
//   follow  → FOLLOW sets for each non-terminal

bool validateWithLL1(
    vector<Token>& tokens,
    Grammar& grammar,
    map<string, map<string, Production>>& table,
    map<string, set<string>>& follow)
{
    bool hasErrors = false;

    // Initialize the stack with $ (end marker) and the start symbol
    stack<string> stk;
    stk.push("$");
    stk.push(grammar.startSymbol); // startSymbol on top

    int    pos = 0;
    string a   = tokenToGrammar(tokens[pos]); // current input symbol

    while (stk.top() != "$") {
        string X = stk.top(); // top of stack

        // CASE 1: X matches current token → successful match
        if (X == a) {
            stk.pop();
            pos++;
            // Skip STATE tokens — handled separately by parseState()
            while (pos < (int)tokens.size() &&
                   tokens[pos].type == TokenType::STATE) {
                pos++;
            }
            a = tokenToGrammar(tokens[pos]);
            continue;
        }

        // Stop silently when we reach the State block
        // The initial state is parsed separately by parseState()
        if (a == "State" || a == "$") break;

        // CASE 2: X is a terminal that doesn't match → Panic Mode heuristic 5
        // Assume the terminal was mistakenly inserted → pop it
        if (grammar.terminals.count(X)) {
            hasErrors = true;
            stk.pop();
            continue;
        }

        // CASE 3: M[X][a] is empty → Panic Mode heuristic 1
        // Skip tokens until we find a synchronizing token in FOLLOW(X)
        if (!table.count(X) || !table.at(X).count(a)) {
            hasErrors = true;
            while (a != "$" && a != "State" &&
                   follow.count(X) && !follow.at(X).count(a)) {
                pos++;
                if (pos < (int)tokens.size())
                    a = tokenToGrammar(tokens[pos]);
                else
                    a = "$";
            }
            stk.pop(); // pop X and continue
            continue;
        }

        // CASE 4: M[X][a] has a production → apply it
        // Pop X and push the production's RHS in reverse order
        // (so the leftmost symbol ends up on top of the stack)
        Production prod = table.at(X).at(a);
        stk.pop();
        if (prod.rhs[0] != "eps") {
            for (int i = (int)prod.rhs.size()-1; i >= 0; i--) {
                stk.push(prod.rhs[i]);
            }
        }
        // If production is eps → push nothing (epsilon = empty string)
    }

    return !hasErrors;
}

//AST CONSTRUCTION (Recursive Descent)
// WHAT THIS SECTION DOES:
//   Builds the AST using recursive descent parsing.
//   Each function corresponds to one production rule in the grammar.
//   Guided conceptually by the same LL(1) table from Section 2.


static vector<Token> tokens; // token list from the lexer
static int pos;              // current position in the token list

// Returns the current token without advancing
Token current() {
    return tokens[pos];
}

// Verifies the current token matches the expected type and advances.
// Throws a syntax error if the token does not match.
Token consume(TokenType expected) {
    if (current().type != expected) {
        throw runtime_error(
            "Syntax error near '" + current().value + "'"
        );
    }
    return tokens[pos++];
}

// Forward declarations — functions call each other recursively
CondNode* parseAtom();
CondNode* parseCondRest(CondNode* left);
CondNode* parseCond();
RuleNode* parseRule();
void      parseRuleList(ProgramNode* program);
void      parseState(map<string,int>& vars, set<string>& facts);

// parseAtom()
// Grammar: Atom  → id Atom'
//          Atom' → RelOp value | eps
// Table entries used:
//   M[Atom][id]    = Atom → id Atom'
//   M[Atom'][>]    = Atom' → RelOp value  → comparison node
//   M[Atom'][<]    = Atom' → RelOp value  → comparison node
//   M[Atom'][=]    = Atom' → RelOp value  → comparison node
//   M[Atom'][AND]  = Atom' → eps          → fact node
//   M[Atom'][then] = Atom' → eps          → fact node

CondNode* parseAtom() {
    string idName = current().value;
    consume(TokenType::ID);

    // Look at the next token to decide which Atom' production to use
    TokenType next = current().type;
    if (next == TokenType::GT ||
        next == TokenType::LT ||
        next == TokenType::EQ) {
        // Atom' → RelOp value → build a comparison node
        string op = current().value;
        pos++; // consume the operator
        int val = stoi(current().value);
        consume(TokenType::NUMBER);
        return new CmpNode(idName, op, val);
    }
    // Atom' → eps → build a fact node
    return new FactNode(idName);
}

// parseCondRest()
// Grammar: CondRest → AND Atom CondRest | eps
//
// Table entries used:
//   M[CondRest][AND]  = AND Atom CondRest → more conditions follow
//   M[CondRest][then] = eps               → no more conditions


CondNode* parseCondRest(CondNode* left) {
    if (current().type == TokenType::AND) {
        consume(TokenType::AND);
        CondNode* right = parseAtom();
        AndNode*  node  = new AndNode(left, right);
        return parseCondRest(node); // handle possible additional ANDs
    }
    return left; // eps → return accumulated condition
}

// parseCond()
// Grammar: Cond → Atom CondRest
// Table entry used:
//   M[Cond][id] = Atom CondRest

// Parses the first atom, then delegates to parseCondRest()
// to handle any AND chains that follow.

CondNode* parseCond() {
    CondNode* left = parseAtom();      // parse first atom
    return parseCondRest(left);        // handle AND chains
}

// parseRule()
// Grammar: Rule → rule id : if Cond then Action
// Table entry used:
//   M[Rule][rule] = rule id : if Cond then Action

// Consumes each token in order and builds a RuleNode.

RuleNode* parseRule() {
    consume(TokenType::RULE);           // consume "rule"
    string name = current().value;      // save rule name
    consume(TokenType::ID);             // consume rule name
    consume(TokenType::COLON);          // consume ":"
    consume(TokenType::IF);             // consume "if"
    CondNode* cond = parseCond();       // parse the full condition tree
    consume(TokenType::THEN);           // consume "then"
    string action = current().value;    // save action name
    consume(TokenType::ID);             // consume action name
    return new RuleNode(name, cond, action);
}

// parseRuleList()

// Grammar: RuleList → Rule RuleList | eps

// Table entries used:
//   M[RuleList][rule]  = Rule RuleList → parse one rule, then recurse
//   M[RuleList][$]     = eps           → no more rules
//   M[RuleList][State] = eps           → initial state block follows

void parseRuleList(ProgramNode* program) {
    if (current().type == TokenType::RULE) {
        RuleNode* rule = parseRule();
        program->rules.push_back(rule);
        parseRuleList(program); // recursively parse remaining rules
    }
    // STATE or END token → eps production → stop
}

// Reads the initial state block that follows the rules:
//   State:
//   temp = 35       ← variable assignment
//   humidity = 40   ← variable assignment
//   alert           ← active fact (no value)

void parseState(map<string,int>& variables, set<string>& facts) {
    if (current().type != TokenType::STATE) return;
    consume(TokenType::STATE);

    while (current().type != TokenType::END) {
        string name = current().value;
        consume(TokenType::ID);
        if (current().type == TokenType::EQ) {
            // Variable assignment: id = integer
            consume(TokenType::EQ);
            int val = stoi(current().value);
            consume(TokenType::NUMBER);
            variables[name] = val;
        } else {
            // Active fact: just an identifier with no value
            facts.insert(name);
        }
    }
}


ParseResult parse(vector<Token> toks) {

    // Steps 1-4: build grammar, transform, compute sets, build table
    Grammar grammar = buildProjectGrammar();
    auto first  = computeFirst(grammar);
    auto follow = computeFollow(grammar, first);
    bool isLL1  = true;
    auto table  = buildParsingTable(grammar, first, follow, isLL1);

    // Step 5: validate syntax silently (no output)
    validateWithLL1(toks, grammar, table, follow);

    // Step 6: build AST using recursive descent
    tokens = toks;
    pos    = 0;

    ProgramNode*     program = new ProgramNode();
    map<string, int> variables;
    set<string>      facts;

    try {
        parseRuleList(program);     // build rule nodes
        parseState(variables, facts); // read initial state
    } catch (runtime_error& e) {
        throw runtime_error(string(e.what()));
    }

    return {program, variables, facts};
}