#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "util.h"
#include "transition_system.h"
#include "MiniSat-p_v1.14/SolverTypes.h"

using namespace std;

void TransitionSystem::print() {
  // This omits the constant false
  cout << "aag " << max_index << " " << nr_inputs << " " << nr_latches << " " << nr_outputs << " " << nr_gates << endl;
  for(int i = 1; i <= nr_inputs; i++) {
    cout << 2 * i << endl;
  }
  for(int i = 0; i < nr_latches; i++) {
    cout << index(latches[i].first) << " " << index(latches[i].second) << endl;
  }
  cout << index(output) << endl;
  for(int i = 0; i < nr_gates; i++) {
    cout << index(gates[i][0]) << " " <<  index(gates[i][1]) << " " << index(gates[i][2]) << endl;
  }
}

// Expecting an input file in aiger ASCII format containing a single output
void TransitionSystem::parse(string file_name) {
  ifstream file;
  string line;
  int tmp;
  try {
    file.open(file_name);
    if(file.is_open()) {
      // parse first line
      getline(file,line);
      parse_first_line(line);

      // Constant false
      const_false = Lit(0);
      
      // parse inputs
      // There seems to be no need to parse inputs (Even if negated)
      for(int i = 0; i < nr_inputs; i++) {
	getline(file, line);
	if(stoi(line) % 2) {
	  cerr << "Input is negated!!" << endl;
	}
      }

      // parse latches
      latches = vector<pair<Lit,Lit>>(nr_latches);
      for(int i = 0; i < nr_latches; i++) {
	getline(file, line);
	
	// Remove trailing and leading whitespace
	line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
	line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);
	latches[i] = pair<Lit,Lit>(toLit(stoi(line.substr(0, line.find(" ")))),toLit(stoi(line.substr(line.find(" ")+1, string::npos))));
      }
      
      // parse output
      getline(file, line);
      output = toLit(stoi(line));
      
      // parse gates
      gates = vector<vector<Lit>>(nr_gates);
      for(int i = 0; i < nr_gates; i++) {
	getline(file, line);
	// Remove trailing and leading whitespace
	line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
	line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);
	
	vector<Lit> gate(3);
	tmp = stoi(line.substr(0, line.find(" ")));
	if(tmp % 2) {
	  cerr << "Output of gate is negated";
	}
	gate[0] = toLit(tmp);
	gate[1] = toLit(stoi(line.substr(line.find(" ")+1,line.find(" ", line.find(" ")))));
	gate[2] = toLit(stoi(line.substr(line.find_last_of(" "), string::npos)));
	gates[i] = gate;
      }

      // Handle constant false (Assuming there is at least one variable)
      gates.push_back({const_false, Lit(1), ~Lit(1)});
      file.close();
    }
  }
  catch(...) {
    cout << "Exception occured while parsing" << endl;
    file.close();
  }
}

void TransitionSystem::parse_first_line(const string &line) {
  string tmp;
  stringstream ss(line);
  
  getline(ss, tmp, ' ');
  getline(ss, tmp, ' ');
  max_index = stoi(tmp);

  getline(ss, tmp, ' ');
  nr_inputs = stoi(tmp);

  getline(ss, tmp, ' ');
  nr_latches = stoi(tmp);

  getline(ss, tmp, ' ');
  nr_outputs = stoi(tmp);

  getline(ss, tmp, ' ');
  nr_gates = stoi(tmp);
}

// Adds clauses representing the circuit to result
// Shifts all literals to correspond to the step + 1 state
void TransitionSystem::circuit_cnf(vec<vec<Lit>>& result, int step) {
  Lit x1, x2, x3;
  vec<Lit> lits;
  int offset = step * (max_index + 1);

  for(vector<Lit> gate : gates) {
    // Account for offset of variables
    tseitin_and(shift_literal(gate[0], offset), shift_literal(gate[1], offset),
		shift_literal(gate[2], offset), result);
  }
}

// Encodes the initial state as cnf and adds clauses to result
void TransitionSystem::initial_cnf(vec<vec<Lit>>& result) {
  vec<Lit> lits;
  for(pair<Lit, Lit> latch : latches) {
    lits.push(~latch.first);
    result.push();
    lits.copyTo(result.last());
    lits.clear();
  }
  circuit_cnf(result, 0);
}

// Add a disjunction of all bad properties for states "from"-"to" to result
void TransitionSystem::bad_cnf(vec<vec<Lit>>& result, int from, int to) {
  vec<Lit> lits;
  for(int i = from; i <= to; i++) {
    lits.push(shift_literal(output, i * (max_index + 1)));
  }
  result.push(); lits.copyTo(result.last()); lits.clear();
}

// Adds latch-dependencies and circuit clauses of state "step + 1"
void TransitionSystem::transition_cnf(vec<vec<Lit>>& result, int step) {
  vec<Lit> lits;
  int from_offset = (step) * (max_index + 1);
  int to_offset = from_offset + (max_index + 1);
  Lit x1, x2;

  for(pair<Lit, Lit> latch : latches) {
    x1 = latch.first;
    x2 = latch.second;
    // x_1 <=> x_2: (~x_1 or x_2) and (x_1 or ~x_2)
    // C1
    lits.push(~shift_literal(x1, to_offset));
    lits.push(shift_literal(x2, from_offset));
    result.push(); lits.copyTo(result.last()); lits.clear();

    // C2
    lits.push(shift_literal(x1, to_offset));
    lits.push(~shift_literal(x2, from_offset));
    result.push(); lits.copyTo(result.last()); lits.clear();

  }

  circuit_cnf(result, step+1);
}

// Adds tseitinized circuit for initial state to result
// Next free will be used and incremented for labels
void TransitionSystem::initial_circuit_tseitin(vec<vec<Lit>>& result, Var *next_free) {
  // The idea is to have a label for every gate
  // Then compute top level label l <-> (l1 /\ l2 ...)
  vec<Lit> labels;
  for(vector<Lit> gate : gates) {
    // Handle conjunction
    tseitin_and(Lit(*next_free), gate[1], gate[2], result);
    tseitin_iff(Lit(*next_free +1), gate[0],Lit(*next_free), result);
    labels.push(Lit(*next_free+1));
    *next_free += 2;
  }
  
  Lit left = labels[0];
  for(int i = 1; i < labels.size(); i++) {
    tseitin_and(Lit(*next_free), left, labels[i], result);
    left = Lit(*next_free);
    *next_free += 1;
  }
}

// Adds tseitinized initial state to result
// next_free is used for labels and will be incremented
void TransitionSystem::initial_tseitin(vec<vec<Lit>>& result, Var *next_free) {
  if(latches.size() > 0) {
    Lit left = ~latches[0].first;
    for(int i = 1; i < latches.size(); i++) {
      tseitin_and(Lit(*next_free), left, ~latches[i].first, result);
      left = Lit(*next_free);
      *next_free += 1;
    }
    if(latches.size() > 1)
      left = Lit(*next_free -1);
    initial_circuit_tseitin(result, next_free);
    tseitin_and(Lit(*next_free), left, Lit(*next_free - 1), result);
    *next_free += 1;
  } else {
    initial_circuit_tseitin(result, next_free);
  }
}


