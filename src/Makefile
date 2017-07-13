CXX = mpicxx
CFLAGS = -g -fPIC -std=c++11
INCLUDE = 
LDFLAGS =
LIBS = -lunwind 

OBJ_DIR = /g/g17/chapp1/csmpi/src/obj/
BUILD_DIR = ../build/lib/

all:
	$(CXX) $(CFLAGS) $(INCLUDE) -c csmpi.cpp -o $(OBJ_DIR)csmpi.o 
	$(CXX) -shared -o $(BUILD_DIR)/libcsmpi.so $(OBJ_DIR)*.o $(LDFLAGS) $(LIBS)

clean:
	rm $(OBJ_DIR)*
	rm $(BUILD_DIR)*
