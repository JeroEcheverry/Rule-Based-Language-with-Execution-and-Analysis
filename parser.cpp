#include "parser.h"
#include "grammar.h"
#include <stdexcept>
#include <stack>
#include <iostream>

using namespace std;

// SECTION 1 — GRAMMAR AND LL(1) TABLE

Grammar buildProjectGrammar() {
    // Step 1: define the original grammar with left recursion
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

    original.productions.push_back({"Program",  {"RuleList"}});

    original.productions.push_back({"RuleList", {"Rule","RuleList"}});
    original.productions.push_back({"RuleList", {"eps"}});

    original.productions.push_back({"Rule",
        {"rule","id",":","if","Cond","then","Action"}});

    // Cond → Cond AND Atom | Atom  ← left recursion: A=Cond, alpha=AND Atom, beta=Atom
    original.productions.push_back({"Cond", {"Cond","AND","Atom"}});
    original.productions.push_back({"Cond", {"Atom"}});

    // Atom → id RelOp value | id  ← common prefix: both start with id
    original.productions.push_back({"Atom", {"id","RelOp","value"}});
    original.productions.push_back({"Atom", {"id"}});

    original.productions.push_back({"RelOp", {">"}});
    original.productions.push_back({"RelOp", {"<"}});
    original.productions.push_back({"RelOp", {"="}});

    original.productions.push_back({"Action", {"id"}});

    // Step 2: automatically eliminate left recursion (Dragon Book Section 4.3)
    Grammar noRecursion = eliminateLeftRecursion(original);

    // Step 3: automatically apply left factoring (Dragon Book Section 4.3)
    Grammar transformed = leftFactoring(noRecursion);

    transformed.terminals.insert("State"); // needed for the validator to handle State correctly
    return transformed;
}

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


// SECTION 2 — LL(1) VALIDATION (Figure 4.20 + Panic Mode)

bool validateWithLL1(
    vector<Token>& tokens,
    Grammar& grammar,
    map<string, map<string, Production>>& table,
    map<string, set<string>>& follow)
{
    bool hasErrors = false;

    stack<string> stk;
    stk.push("$");                    // end marker at the bottom
    stk.push(grammar.startSymbol);    // start symbol on top

    int    pos = 0;
    string a   = tokenToGrammar(tokens[pos]); // current input symbol

    while (stk.top() != "$") {
        string X = stk.top();

        if (X == a) { // CASE 1: match → pop stack and advance token
            stk.pop();
            pos++;
            while (pos < (int)tokens.size() && tokens[pos].type == TokenType::STATE) pos++; // skip STATE tokens
            a = tokenToGrammar(tokens[pos]);
            continue;
        }

        if (a == "State" || a == "$") break; // reached State block → stop validation silently

        if (grammar.terminals.count(X)) { // CASE 2: terminal mismatch → Panic Mode heuristic 5
            hasErrors = true;
            stk.pop(); // pop terminal, assume it was mistakenly inserted
            continue;
        }

        if (!table.count(X) || !table.at(X).count(a)) { // CASE 3: empty table entry → Panic Mode heuristic 1
            hasErrors = true;
            while (a != "$" && a != "State" && follow.count(X) && !follow.at(X).count(a)) {
                pos++; // skip tokens until a synchronizing token in FOLLOW(X) is found
                a = (pos < (int)tokens.size()) ? tokenToGrammar(tokens[pos]) : "$";
            }
            stk.pop();
            continue;
        }

        // CASE 4: apply production from table → pop X, push RHS in reverse order
        Production prod = table.at(X).at(a);
        stk.pop();
        if (prod.rhs[0] != "eps") { // eps means empty string → push nothing
            for (int i = (int)prod.rhs.size()-1; i >= 0; i--) stk.push(prod.rhs[i]);
        }
    }

    return !hasErrors;
}

// SECTION 3 — AST CONSTRUCTION (Recursive Descent)

static vector<Token> tokens;
static int pos;

Token current() { return tokens[pos]; }

Token consume(TokenType expected) {
    if (current().type != expected)
        throw runtime_error("Syntax error near '" + current().value + "'");
    return tokens[pos++];
}

CondNode* parseAtom();
CondNode* parseCondRest(CondNode* left);
CondNode* parseCond();
RuleNode* parseRule();
void      parseRuleList(ProgramNode* program);
void      parseState(map<string,int>& vars, set<string>& facts);

CondNode* parseAtom() {
    string idName = current().value;
    consume(TokenType::ID); // consume the identifier

    TokenType next = current().type;
    if (next == TokenType::GT || next == TokenType::LT || next == TokenType::EQ) {
        // M[Atom'][>/</ =] = Atom' → RelOp value → comparison node
        string op = current().value;
        pos++; // consume the operator
        int val = stoi(current().value);
        consume(TokenType::NUMBER);
        return new CmpNode(idName, op, val);
    }
    // M[Atom'][AND/then] = Atom' → eps → fact node
    return new FactNode(idName);
}

CondNode* parseCondRest(CondNode* left) {
    if (current().type == TokenType::AND) {
        // M[CondRest][AND] = AND Atom CondRest → more conditions follow
        consume(TokenType::AND);
        CondNode* right = parseAtom();
        AndNode*  node  = new AndNode(left, right);
        return parseCondRest(node); // recursively handle chained ANDs
    }
    return left; // M[CondRest][then] = eps → no more conditions
}

CondNode* parseCond() {
    // M[Cond][id] = Atom CondRest
    CondNode* left = parseAtom();
    return parseCondRest(left);
}

RuleNode* parseRule() {
    // M[Rule][rule] = rule id : if Cond then Action
    consume(TokenType::RULE);
    string name = current().value;
    consume(TokenType::ID);
    consume(TokenType::COLON);
    consume(TokenType::IF);
    CondNode* cond = parseCond();
    consume(TokenType::THEN);
    string action = current().value;
    consume(TokenType::ID);
    return new RuleNode(name, cond, action);
}

void parseRuleList(ProgramNode* program) {
    if (current().type == TokenType::RULE) {
        // M[RuleList][rule] = Rule RuleList → parse one rule then recurse
        RuleNode* rule = parseRule();
        program->rules.push_back(rule);
        parseRuleList(program);
    }
    // M[RuleList][State/$] = eps → stop
}

void parseState(map<string,int>& variables, set<string>& facts) {
    
    if (current().type != TokenType::STATE) return; // no State block → skip
    consume(TokenType::STATE);

    while (current().type != TokenType::END) {
        string name = current().value;
        consume(TokenType::ID);
        if (current().type == TokenType::EQ) {
            // id = integer → variable assignment
            consume(TokenType::EQ);
            int val = stoi(current().value);
            consume(TokenType::NUMBER);
            variables[name] = val;
        } else {
            facts.insert(name); // id only → active fact
        }
    }
}

// MAIN PARSE FUNCTION

ParseResult parse(vector<Token> toks) {

    tokens = toks;
    pos    = 0;
    
    Grammar grammar = buildProjectGrammar();        // Step 1-3: build + transform grammar
    auto first  = computeFirst(grammar);            // Step 4: compute FIRST sets
    auto follow = computeFollow(grammar, first);    // Step 4: compute FOLLOW sets
    bool isLL1  = true;
    auto table  = buildParsingTable(grammar, first, follow, isLL1); // Step 5: build table

    validateWithLL1(toks, grammar, table, follow);  // Step 6: validate silently

    ProgramNode*     program = new ProgramNode();
    map<string, int> variables;
    set<string>      facts;

    try {
        parseRuleList(program);
        parseState(variables, facts);
    } catch (runtime_error& e) {
        throw runtime_error(string(e.what()));
    }

    return {program, variables, facts};
}
