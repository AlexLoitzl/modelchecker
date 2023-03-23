#include "MiniSat-p_v1.14/Proof.h"
#include "MiniSat-p_v1.14/Sort.h"
#include "util.h"
#include "traverser.h"
#include <iostream>
#include <vector>
#include <set>
#include <memory>

using namespace std;

bool Traverser::resolve(vec<Lit>& main, vec<Lit>& other, Var x) {
  Lit p;

  for (int i = 0; i < main.size(); i++) {
    if(var(main[i]) == x) {
      p = main[i];
      main[i] = main.last();
      main.pop();
    }
  }

  for (int i = 0; i < other.size(); i++) {
    if (var(other[i]) != x)
      main.push(other[i]);
  }

  sortUnique(main);
  return sign(p);
}

void Traverser::root(const vec<Lit>& c) {
  clauses.push();
  c.copyTo(clauses.last());

  shared_ptr<Node> root = make_shared<Node>();
  root->clause_id = forest.roots.size();
  forest.roots.push_back(root);
  root->int_trivial = init;
}
  
void Traverser::chain(const vec<ClauseId>& cs, const vec<Var>& xs) {
  clauses.push();
  vec<Lit>& resolvent = clauses.last();
  clauses[cs[0]].copyTo(resolvent);

  shared_ptr<Node> node1, node2, parent;
  node1 = make_shared<Node>();
  node1->clause_id=cs[0];
  for (int i = 0; i < xs.size(); i++) {
    node2 = make_shared<Node>();
    node2->clause_id=cs[i+1];

    bool sign = resolve(resolvent, clauses[cs[i+1]], xs[i]);

    parent = make_shared<Node>();
    parent->resolved_on = xs[i];
    parent->negative = (sign?node1:node2);
    parent->positive = (sign?node2:node1);

    node1 = parent;
  }
  node1->clause_id=forest.roots.size();
  forest.roots.push_back(node1);
}

void ResolutionForest::print_subtree(shared_ptr<Node> it) {
  if(it == NULL)
    return;
  it->print();
  cout << "Left subtree:" << endl;
  print_subtree(it->negative);
  cout << "Right subtree:" << endl;
  print_subtree(it->positive);
}
  
void ResolutionForest::print() {
  for(size_t i = 0; i < roots.size(); i++) {
    cout << "Tree " << i << ": " << endl;
    print_subtree(roots[i]);
  }
}

// Computes interpolant for a node recursively
// Assumes that all leaves are initialized to true/false
// Uses tseitin translation: We keep track of labels in the nodes and aggregate all produced clauses in the result vector
void ResolutionForest::compute_partial_interpolant(shared_ptr<Node> node, set<Var> *shared, Var lowest_b, Var highest_b, int index, Var *next_free, vec<vec<Lit>>& result){
  // "array" index parameter does not match the clause id of the current node, i.e.
  // we are  currently in a resolution chain at "index" and need interpolant of "clause_id"
  // we compute the interpolant of the node at "clause_id" and then populate "node"
  if(node->clause_id >= 0 && node->clause_id != index) {
    compute_partial_interpolant(roots[node->clause_id], shared, lowest_b, highest_b, node->clause_id, next_free, result);
    node->int_trivial = roots[node->clause_id]->int_trivial;
    node->label=roots[node->clause_id]->label;
  }
  // If a node has a trivial interpolant or label, we are done
  else if(node->int_trivial == int_undef){
    // Check if we already have an interpolant
    if(node->label == lit_Undef) {
      compute_partial_interpolant(node->negative, shared, lowest_b, highest_b, index, next_free, result);
      compute_partial_interpolant(node->positive, shared, lowest_b, highest_b, index, next_free, result);

      if(node->resolved_on >= lowest_b && node->resolved_on <= highest_b) {
	// Resolved on B
	resolve_B(node, next_free, result);
      } else if (shared->count(node->resolved_on)) {
	// Resolved on shared
	resolve_shared(node, next_free, result);
      } else {
	// Resolved on A
	resolve_A(node, next_free, result);
      }
    }
  }
}

void ResolutionForest::resolve_B(shared_ptr<Node> node, Var *next_free, vec<vec<Lit>>& result) {
  // I_1 /\ I_2
  switch(node->negative->int_trivial){
  case int_f: node->int_trivial=int_f; break;
  case int_t:
    if(node->positive->int_trivial != int_undef)
      node->int_trivial=node->positive->int_trivial;
    else
      node->label = node->positive->label;
    break;
  case int_undef:
    switch(node->positive->int_trivial){
    case int_f: node->int_trivial=int_f; break;
    case int_t:
      node->label = node->negative->label;
      break;
    case int_undef:
      node->label = Lit(*next_free); *next_free+=1;
      tseitin_and(node->label, node->negative->label, node->positive->label, result); 
      break;
    }
    break;
  }
}


void ResolutionForest::resolve_A(shared_ptr<Node> node, Var *next_free, vec<vec<Lit>>& result) {
  // I_1 \/ I_2
  switch(node->negative->int_trivial){
  case int_t: node->int_trivial=int_t; break;
  case int_f:
    if(node->positive->int_trivial != int_undef)
      node->int_trivial=node->positive->int_trivial;
    else
      node->label=node->positive->label;
    break;
  case int_undef:
    switch(node->positive->int_trivial){
    case int_t: node->int_trivial=int_t; break;
    case int_f:
      node->label=node->negative->label;
      break;
    case int_undef:	      
      node->label=Lit(*next_free); *next_free+=1;
      tseitin_or(node->label, node->negative->label, node->positive->label, result);
      break;
    }
    break;
  }
}

void ResolutionForest::resolve_shared(shared_ptr<Node> node, Var *next_free, vec<vec<Lit>>& result) {
  vec<Lit> lits;
  vec<vec<Lit>> clauses;
  
  // Handle clause containing positive resolvent
  switch(node->positive->int_trivial){
  case int_t:
    break;
  case int_f:
    lits.push(Lit(node->resolved_on));
    clauses.push(); lits.copyTo(clauses.last()); lits.clear();
    break;
  case int_undef:
    lits.push(Lit(node->resolved_on));
    lits.push(Lit(node->positive->label));
    clauses.push(); lits.copyTo(clauses.last()); lits.clear();
    break;
  }
  
  // Handle clause containing negative resolvent
  switch(node->negative->int_trivial){
  case int_t:
    break;
  case int_f:
    lits.push(~Lit(node->resolved_on));
    clauses.push(); lits.copyTo(clauses.last()); lits.clear();
    break;
  case int_undef:
    lits.push(~Lit(node->resolved_on));
    lits.push(Lit(node->negative->label));
    clauses.push(); lits.copyTo(clauses.last()); lits.clear();
    break;
  }
  // clauses contains computed interpolant. Now tseitenize
  if(clauses.size() == 0){
    // Both interpolants were true
    node->int_trivial=int_t;
  } else if(clauses.size() == 1) {
    if(clauses[0].size() == 1) {
      // Single literal, just propagate forward as label
      node->label=clauses[0][0];
    } else {
      // Single clause
      node->label = Lit(*next_free);
      *next_free += 1;
      tseitin_or(node->label, clauses[0][0], clauses[0][1], result);
    }
  } else {
    // Contains two clauses (potentially unit)
    Lit label1;
    Lit label2;
    if(clauses[0].size() == 1) 
      label1 = clauses[0][0];
    else {
      label1 = Lit(*next_free); *next_free += 1;
      tseitin_or(label1, clauses[0][0], clauses[0][1], result);
    }
    if(clauses[1].size() == 1) 
      label2 = clauses[1][0];
    else {
      label2 = Lit(*next_free); *next_free += 1;
      tseitin_or(label2, clauses[1][0], clauses[1][1], result);
    }
    node->label = Lit(*next_free); *next_free += 1;
    tseitin_and(node->label, label1, label2, result);
  }
}
