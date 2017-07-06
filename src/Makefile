CC = mpicc
CFLAGS = -g -std=c++11 -fPIC 
INCLUDE = 
LDFLAGS =
LIBS = -lstdc++ -lunwind

OBJ_DIR = /g/g17/chapp1/callstack_mpi/src/obj/
BUILD_DIR = ../build/lib/

all:
	$(CC) $(CFLAGS) $(INCLUDE) -c callstack_mpi.cpp -o $(OBJ_DIR)callstack_mpi.o 
	$(CC) -shared -o $(BUILD_DIR)/libcallstack_mpi.so $(OBJ_DIR)*.o $(LDFLAGS) $(LIBS)

clean:
	rm $(OBJ_DIR)*
	rm $(BUILD_DIR)*
