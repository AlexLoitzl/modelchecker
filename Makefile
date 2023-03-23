CC = g++

CFLAGS = -std=c++14 -I/Minisat-p_v1.14

TARGET = modelchecker

OBJS = transition_system.o traverser.o util.o

MINISAT = MiniSat-p_v1.14/Proof.o MiniSat-p_v1.14/Solver.o MiniSat-p_v1.14/File.o

M_DIR = MiniSat-p_v1.14

all: $(TARGET)

$(TARGET): $(TARGET).cpp $(OBJS) minisat $(MINISAT)
	$(CC) $(CFLAGS) $(MINISAT) $(OBJS) $(TARGET).cpp -o $(TARGET)

clean:
	$(RM) $(TARGET) $(OBJS)
	cd $(M_DIR); $(MAKE) clean

minisat:
	$(MAKE) -C $(M_DIR)

transition_system.o: transition_system.cpp util.o
	$(CC) $(CFLAGS) -c transition_system.cpp

traverser.o: traverser.cpp util.o
	$(CC) $(CFLAGS) -c traverser.cpp

util.o: util.cpp
	$(CC) $(CFLAGS) -c util.cpp

