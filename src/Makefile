CXX = mpicxx
CFLAGS = -g -fPIC -std=c++11
INCLUDE = 
LDFLAGS =
LD_LIBRARY_FLAGS = 
LIBS = -lunwind -lz 

OBJ_DIR = $(HOME)/repos/csmpi/src/obj/
BUILD_DIR = ../build/
LIB_DIR = $(BUILD_DIR)/lib

all:
	mkdir $(OBJ_DIR)
	mkdir $(BUILD_DIR)
	mkdir $(LIB_DIR)
	$(CXX) $(CFLAGS) $(INCLUDE) -c csmpi.cpp -o $(OBJ_DIR)csmpi.o 
	$(CXX) -shared -o $(BUILD_DIR)/libcsmpi.so $(OBJ_DIR)*.o $(LDFLAGS) $(LD_LIBRARY_FLAGS) $(LIBS)

clean:
	rm -rf $(OBJ_DIR)*
	rm -rf $(BUILD_DIR)*
