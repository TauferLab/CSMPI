CC = mpicc
CFLAGS = -g -std=c++11 -fPIC 
INCLUDE = 
LDFLAGS =
LIBS = -lstdc++ -lunwind

OBJ_DIR = /g/g17/chapp1/csmpi/src/obj/
BUILD_DIR = ../build/lib/

all:
	$(CC) $(CFLAGS) $(INCLUDE) -c csmpi.cpp -o $(OBJ_DIR)csmpi.o 
	$(CC) -shared -o $(BUILD_DIR)/libcsmpi.so $(OBJ_DIR)*.o $(LDFLAGS) $(LIBS)

clean:
	rm $(OBJ_DIR)*
	rm $(BUILD_DIR)*
