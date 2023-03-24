# modelchecker
This tool includes both a Bounded Model Checker as well as an Interpolation-based Model Checker. 
It expects a transition system specified as an And-inverter graph in [AIGER ASCII format](http://fmv.jku.at/aiger) with a single output interpreted as the bad property.

esigned to find , based on thematerial presented in class in the course [Computer Aided Verification](https://tiss.tuwien.ac.at/course/courseDetails.xhtml?courseNr=181145) by Georg Weissenbacher at TU Wien and the description in [[VWM2015]](http://dx.doi.org/10.1109/JPROC.2015.2455034).
## Usage
### Optional Parameters
 - -v &ensp;&ensp; Prints some interesting info while running the interpolation-based checker
 - -V &ensp;&ensp; Prints more interesting info while running the interpolation-based checker
 - -a n &ensp;&ensp; Limits inner loop iterations in the interpolation-based checker, i.e. the number of interpolants added to initial states
 - -b n &ensp;&ensp; Limits outer loop iterations in the interpolation-based checker
### Bounded Model Checker
To run the bounded model checking procedure simply pass a bound k **before** specifying the input file:
```
./modelchecker 42 input_file.aag
```
### Interpolation-based Model Checker
To run the interpolation-based checker simply specify an input file along with some optional parameters
```
./modelchecker input_file.aag
```
## Build
There is a Makefile attached. Adapt accordingly

## Notes
For large instances one might run into a stack overflow, just increase the stack size if that is the case. At that point the solver is usually already struggling to come up with an answer.

On the [sequential modelchecking benchmarks](http://fmv.jku.at/aiger/tip-aig-20061215.zip) from 2006 provided on the [AIGER FORMAT](http://fmv.jku.at/aiger) website, my tool performs comparable to [nuXmv](https://nuxmv.fbk.eu).
