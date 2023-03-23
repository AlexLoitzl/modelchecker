#ifndef TRAVERSER_H
#define TRAVERSER_H

#include "MiniSat-p_v1.14/Proof.h"
#include "MiniSat-p_v1.14/Sort.h"
#include "util.h"
#include <iostream>
#include <vector>
#include <set>
#include <memory>

using namespace std;

// Used in interpolant computation to denote T/F interpolant
enum Trivial { int_t, int_f, int_undef };

// A node of a resolution proof
// Holds clause id and the literal resolved on
// If interpolant computed, either True or False, or int_trivial is int_undef and it holds a clause
// Additionally since we tseitinize the interpolant holds the corresponding label
class Node {
public:
  int clause_id=-1;
  Var resolved_on=var_Undef;
  shared_ptr<Node> negative=NULL, positive=NULL;

  // Label is top-level label of tseitin translation
  // Attention: We might just have a label if the interpolant is a unit clause
  Trivial int_trivial=int_undef;
  Lit label = lit_Undef;
  
  Node(){}
  Node(Var x) : resolved_on(x) {}
  void print() {
    cout << "  Node(Clause_Id:" << clause_id << ", Resolved on:" << resolved_on << "): ";
    cout << "  Interpolant: ";
    if(int_trivial==int_undef)
      cout << "Non trivial interpolant" << endl;
    else
      cout << (int_trivial==int_t?"True":"False") << endl;
  }
};

class ResolutionForest {
public:
  vector<shared_ptr<Node>> roots;

  void print();
  void print_subtree(shared_ptr<Node> it);
  void compute_partial_interpolant(shared_ptr<Node> node, set<Var> *shared, Var lowest_b, Var highest_b, int index, Var *next_free, vec<vec<Lit>>& result);
  void resolve_B(shared_ptr<Node> node, Var *next_free, vec<vec<Lit>>& result);
  void resolve_A(shared_ptr<Node> node, Var *next_free, vec<vec<Lit>>& result);
  void resolve_shared(shared_ptr<Node> node, Var *next_free, vec<vec<Lit>>& result);
};

struct Traverser : public ProofTraverser {
  Trivial init = int_undef;
  vec<vec<Lit>> clauses;

  ResolutionForest forest;

  bool resolve(vec<Lit>& main, vec<Lit>& other, Var x);
  void root(const vec<Lit>& c);
  void chain(const vec<ClauseId>& cs, const vec<Var>& xs);
  void deleted(ClauseId c) {
    clauses[c].clear();
  }
};
#endif
