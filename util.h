#include "MiniSat-p_v1.14/Global.h"
#include "MiniSat-p_v1.14/SolverTypes.h"
#include "MiniSat-p_v1.14/Solver.h"
#include <string>

void tseitin_or(Lit label, Lit x1, Lit x2, vec<vec<Lit>>& result);

void tseitin_and(Lit label, Lit x1, Lit x2, vec<vec<Lit>>& result);

void tseitin_iff(Lit label, Lit x1, Lit x2, vec<vec<Lit>>& result);

void print_clause(vec<Lit>& clause);

void print_model(Solver &s);

void print_cnf(vec<vec<Lit>>& clauses, std::string name);

Lit shift_literal(Lit x, int offset); 

