#include "MiniSat-p_v1.14/Solver.h"
#include <iostream>
#include <string>
#include "util.h"

using namespace std;

void tseitin_or(Lit label, Lit x1, Lit x2, vec<vec<Lit>>& result) {
  vec<Lit> lits;
  // C1
  lits.push(label);
  lits.push(~x1);
  result.push(); lits.copyTo(result.last()); lits.clear();
  // C2
  lits.push(label);
  lits.push(~x2);
  result.push(); lits.copyTo(result.last()); lits.clear();
  // C3
  lits.push(~label);
  lits.push(x1);
  lits.push(x2);
  result.push(); lits.copyTo(result.last());
}

void tseitin_and(Lit label, Lit x1, Lit x2, vec<vec<Lit>>& result) {
  vec<Lit> lits;
  // C1
  lits.push(~label);
  lits.push(x1);
  result.push(); lits.copyTo(result.last()); lits.clear();
  // C2
  lits.push(~label);
  lits.push(x2);
  result.push(); lits.copyTo(result.last()); lits.clear();
  // C3
  lits.push(label);
  lits.push(~x1);
  lits.push(~x2);
  result.push(); lits.copyTo(result.last());
}

void tseitin_iff(Lit label, Lit x1, Lit x2, vec<vec<Lit>>& result) {
  vec<Lit> lits;
  //C1
  lits.push(~label);
  lits.push(~x1);
  lits.push(x2);
  result.push(); lits.copyTo(result.last()); lits.clear();
  // C2
  lits.push(~label);
  lits.push(x1);
  lits.push(~x2);
  result.push(); lits.copyTo(result.last()); lits.clear();
  // C3
  lits.push(label);
  lits.push(~x1);
  lits.push(~x2);
  result.push(); lits.copyTo(result.last()); lits.clear();
  // C4
  lits.push(label);
  lits.push(x1);
  lits.push(x2);
  result.push(); lits.copyTo(result.last()); lits.clear();  
}

void print_clause(vec<Lit>& clause) {
  cout << "(";
  for(int i = 0; i < clause.size(); i++) {
    cout << (sign(clause[i])?"~":"") << var(clause[i]) << ", ";
  }
  cout << ")" << endl;
}

void print_model(Solver &s) {
  if(s.okay()) {
    cout << "Model found:" << endl;
    for (int j = 0; j < s.nVars(); j++)
      if (s.model[j] != l_Undef) {
        cout << "x_" << j << ": " << ((s.model[j]==l_True)? 1 : 0) << endl;
      }
  } else {
    cout << "Unsat" << endl;
  }
}

void print_cnf(vec<vec<Lit>>& clauses, string name) {
  cout << name << endl;
  for(int i = 0; i < clauses.size(); i++) {
    cout << "  ";
    print_clause(clauses[i]);
  }
}

Lit shift_literal(Lit x, int offset) {
  return Lit(var(x) + offset, sign(x));
}
