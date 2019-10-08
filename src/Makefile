MPICXX = mpicxx
RM = rm -f

CPPFLAGS = -fPIC -Wall -Werror -pedantic #-g
CFLAGS = 
CXXFLAGS = -std=c++17
INCLUDES= -I. -I.. -I../include
INCLUDES+= -I../include/external/
INCLUDES+= -I../include/csmpi/
INCLUDES+= -I/usr/tce/packages/boost/boost-1.69.0-mvapich2-2.3-gcc-4.9.3/include

LDFLAGS = 
LD_LIBRARY_FLAGS = -L/usr/tce/packages/boost/boost-1.69.0-mvapich2-2.3-gcc-4.9.3/lib
LIBS = -lboost_mpi -lboost_serialization


TEST_SOURCES = 
TEST_SOURCES += test_broadcast_config.cpp
#TEST_SOURCES += test_parse_config.cpp

TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)
TEST_TARGETS = $(TEST_SOURCES:.cpp=.exe)

SOURCES = configuration.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = libcsmpi.so

all: $(TEST_TARGETS)

$(TEST_TARGETS): $(TEST_OBJECTS) $(OBJECTS)
	$(MPICXX) $(LDFLAGS) -o $@ $(TEST_OBJECTS) $(LD_LIBRARY_FLAGS) $(OBJECTS) $(LIBS)  

%.o: %.cpp
	$(MPICXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<


clean:
	$(RM) *.o
	$(RM) *.so
	$(RM) *.exe
