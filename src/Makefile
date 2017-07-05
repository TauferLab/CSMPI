CC = mpicc
CFLAGS = -g -std=c++11 -fPIC -shared
INCLUDE = 
LDFLAGS =
LIBS = -lstdc++ -lunwind

OBJ_DIR = /g/g17/chapp1/callstack_mpi/src/obj/
BUILD_DIR = ../build/

all:
	$(CC) $(CFLAGS) $(INCLUDE) callstack_mpi.cpp $(LDFLAGS) $(LIBS) -o $(OBJ_DIR)callstack_mpi.o
	$(CC) -shared -fPIC -o $(BUILD_DIR)/callstack_mpi.so $(OBJ_DIR)*.o

clean:
	rm $(OBJ_DIR)*
	rm $(BUILD_DIR)*
