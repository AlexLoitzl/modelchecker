#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "MiniSat-p_v1.14/SolverTypes.h"

using namespace std;

class TransitionSystem {
public:
  // indices start from 0
  int max_index;
  int nr_inputs;
  int nr_latches;
  int nr_outputs;
  int nr_gates;
  int const_index;
  vector<pair<Lit, Lit>> latches;
  vector<vector<Lit>> gates;
  Lit output;
  // We use variable 0 to represent false
  Lit const_false;
  void parse(string file);
  void parse_first_line(const string &line);
  void print();
  void circuit_cnf(vec<vec<Lit>>& result, int step);
  void initial_circuit_tseitin(vec<vec<Lit>>& result, Var *next_free);
  void initial_cnf(vec<vec<Lit>>& result);
  void bad_cnf(vec<vec<Lit>>& result, int from, int to);
  void transition_cnf(vec<vec<Lit>>& result, int step);
  void initial_tseitin(vec<vec<Lit>>& result, Var *next_free);
};
