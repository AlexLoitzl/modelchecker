#include <iostream>
#include <fstream>
#include <set>
#include "transition_system.h"
#include "traverser.h"
#include "MiniSat-p_v1.14/Proof.h"
#include "MiniSat-p_v1.14/Solver.h"
#include "MiniSat-p_v1.14/File.h"
#include "util.h"

using namespace std;

// Bounded model checking procedure
// Return true iff property is not violated up to bound k
bool bmc(TransitionSystem t, int k, int verbosity) {
  Solver s;
  s.proof = new Proof();
  vec<vec<Lit>> clauses;

  // Populate "clauses" by initial state, k transitions and bad property 
  
  t.initial_cnf(clauses);
  t.bad_cnf(clauses, 0, k);

  for(int j = 0; j < k; j++)
    t.transition_cnf(clauses ,j);

  // Add clauses and check for satisfiability
  while( (k+1) * (t.max_index + 1) > s.nVars()) { s.newVar(); }
  
  for(int i = 0; i < clauses.size(); i++)
    s.addClause(clauses[i]);

  s.solve();
  
  if(s.okay()) {
    if(verbosity == 2) { print_model(s); }
    return false;
  }
  return true;
}



bool imc(TransitionSystem& t, int inner_bound, int outer_bound, int verbosity) {
  if(verbosity) { cout << "Running initial bmc" << endl; }
  // Check if there is an initial state that violates property
  if(!bmc(t, 0, verbosity))
    return false;
  
  // Unroll B partition outer_bound many times
  // If unspecified continue until either FAIL/OK
  for(int j=1; (outer_bound + 1) - j != 0;j++) {
    if(verbosity) cout << "Outer Loop: j=" << j << "\n Running bmc for k=" << j << endl;
    
    // First do a bmc run
    if(!bmc(t, j, verbosity))
      return false;
    
    // Construct B partition
    vec<vec<Lit>> b_partition;
    t.bad_cnf(b_partition, 1, j);
    for(int i=1; i<j; i++)
      t.transition_cnf(b_partition, i);

    // Compute set of variables shared between partitions
    set<Var> shared;
    Var lower_b = 2 * (t.max_index + 1);
    Var upper_b = (j+1) * (t.max_index + 1) - 1;

    if(j > 1)
      for(pair<Lit, Lit> latch : t.latches)
	shared.insert(var(latch.second) + (t.max_index + 1));
      
    shared.insert(var(t.output) + t.max_index + 1);

    if(verbosity == 2) {
      cout << "Shared variables: {";
      for(Var x : shared)
	cout << x << " ";
      cout << "}" << endl;

      print_cnf(b_partition, "B Partition");
    }
    
    // This is the first variable that can be used for labels for tseitinization
    Var next_free = (j+1) * (t.max_index+1);

    // Initialize first state with initial state of the transition system
    // The labels clause keeps track of the top level labels of tseitinized formulas
    vec<vec<Lit>> init;
    vec<Lit> labels;

    t.initial_tseitin(init, &next_free);
    labels.push(Lit(next_free - 1));

    // Run loop inner_bound many times
    // Continue until spurious counterexample/OK if no bound specified
    for(int p = 0; inner_bound - p != 0; p++) {
      if(verbosity) { cout << "Inner iteration: " << p << endl; }

      Solver s;
      Traverser trav;
      s.proof = new Proof(trav);
      while( next_free > s.nVars()) { s.newVar(); }

      // Initialize b_partition's interpolants to true
      trav.init = int_t;
      // Add B partiton to Solver
      for(int i=0; i < b_partition.size(); i++)
        s.addClause(b_partition[i]);

      // Preprocess b partition
      // Led to substantial improvement for small examples
      // See if this still holds true when improving interpolation
      s.solve();

      // Construct A partition    
      vec<vec<Lit>> a_partition;
      for(int i=0; i<init.size(); i++) {
        a_partition.push();
        init[i].copyTo(a_partition.last());
      }
      t.transition_cnf(a_partition, 0);
      a_partition.push(); labels.copyTo(a_partition.last());

      // Initialize a_partition's interpolants to true
      trav.init = int_f;
      // Add A partition to solver
      for(int i=0; i < a_partition.size(); i++)
        s.addClause(a_partition[i]);

      if(verbosity == 2) { print_cnf(a_partition, "A Partition"); }

      s.solve();

      // Check for spurious counterexample
      if(s.okay()) {
	if(verbosity) { cout << "Spurious counterexample found" << endl; }
	if(verbosity == 2) { print_model(s); }
	
	break;
      }
      
      // Compute Interpolant I
      if(verbosity) { cout << "Computing interpolant" << endl; } 
      shared_ptr<Node> proof_root = trav.forest.roots[trav.forest.roots.size()-1];
      vec<vec<Lit>> interpolant;
      trav.forest.compute_partial_interpolant(trav.forest.roots[trav.forest.roots.size()-1], &shared, lower_b, upper_b, trav.forest.roots.size()-1, &next_free, interpolant);
      if(verbosity) { cout << "Done, size: " << interpolant.size() << " clauses" << endl; }
      if(verbosity == 2) { print_cnf(interpolant, "Interpolant"); }
      
      // Check if fixpoint is reached (Interpolant => INIT)
      // For this check satisfiability (Interpolant & ~INIT)
      Solver fix;
      fix.proof = new Proof();
      while(fix.nVars() < next_free) { fix.newVar(); }
      
      // Add ~INIT
      for(int i = 0; i < init.size(); i++)
	fix.addClause(init[i]);

      for(int i = 0; i < labels.size(); i++)
	fix.addClause(vec<Lit>(1, ~labels[i]));

      // Note that the interpolant cannot be T/F as initial bmc run passed and we have a bad property
      // Shift interpolant before adding to solver
      for(int i = 0; i < interpolant.size(); i++)
	for(int k = 0; k < interpolant[i].size(); k++)
	  if(shared.count(var(interpolant[i][k])))
	    interpolant[i][k] = Lit(var(interpolant[i][k]) - t.max_index - 1, sign(interpolant[i][k]));

      if(shared.count(var(proof_root->label)))
	proof_root->label = Lit(var(proof_root->label) - t.max_index - 1, sign(proof_root->label));

      // Add interpolant
      for(int i = 0; i < interpolant.size(); i++)
	fix.addClause(interpolant[i]);

      fix.addClause(vec<Lit>(1,proof_root->label));

      fix.solve();

      if(!fix.okay())
	return true;
      
      // Add interpolant to new initial state
      for(int i = 0; i < interpolant.size(); i++) {
	init.push();
	interpolant[i].copyTo(init.last());
      }
      labels.push(Lit(proof_root->label));
    }
  }
  // This is only reachable if outer_bound was specified
  return true;
}

int main(int argc, char* argv[]) {
  int opt;
  int outer_loop_bound = -1;
  int inner_loop_bound = -1;
  int verbosity = 0;

  // Parse optional parameters
  while ((opt = getopt(argc, argv, "b:a:vV")) != -1) {
    switch (opt)
      {
      case 'o':
	if(verbosity) cout << "B partition will be expanded at most " << optarg << " many times" << endl;
	outer_loop_bound = stoi(optarg);
	break;
      case 'a':
	if(verbosity) cout << "A partition will be expanded at most " << optarg << " many times" << endl;
	inner_loop_bound = stoi(optarg);
	break;
      case 'v':
	verbosity = 1;
	break;
      case 'V':
        verbosity = 2;
	break;
      case '?':
        cout << "Unknown option: " << optarg << endl;
	cout << "Aborting." << endl;
	return 1;
	break;
      case ':':
	cout << "Unknown option: " << optarg << endl;
	cout << "Aborting." << endl;
	return 1;
	break;
   }
  }
  int k = -1;
  // Parse non-option arguments
  if (argc - optind == 2) {
    // Expecting first argument k, second argument file_name for bounded model checking
    k = stoi(argv[optind++]);
  } else if (argc - optind != 1) {
    cout << "Cannot parse input" << endl;
    cout << "Aborting." << endl;
    return 1;
  }

  TransitionSystem t;
  t.parse(argv[optind]);

  if(k != -1) {
    cout << (bmc(t,k, verbosity)?"OK":"FAIL") << endl;
  } else {
    cout << (imc(t,inner_loop_bound, outer_loop_bound, verbosity)?"OK":"FAIL") << endl;
  }
  
  return 0;
}





