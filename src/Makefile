MPICC = mpicc
MPICXX = mpicxx
RM = rm -f

CPPFLAGS = -fPIC -Wall -Werror -pedantic -g
CFLAGS = 
CXXFLAGS = -std=c++11  -fno-omit-frame-pointer -O0
INCLUDES= -I. -I.. -I../include
INCLUDES+= -I../include/external/
INCLUDES+= -I../include/csmpi/
INCLUDES+= -I/usr/tce/packages/boost/boost-1.69.0-mvapich2-2.3-gcc-4.9.3/include

LDFLAGS = 
LD_LIBRARY_FLAGS = -L/usr/tce/packages/boost/boost-1.69.0-mvapich2-2.3-gcc-4.9.3/lib
LIBS = -lboost_mpi -lboost_serialization -lunwind


TEST_APP_SOURCES = test_app.c
TEST_APP_OBJECTS = $(TEST_APP_SOURCES:.c=.o)
TEST_APP_TARGET = test_app.exe

TEST_SOURCES =
#TEST_SOURCES += test_csmpi_runtime.cpp
TEST_SOURCES += test_backtrace_perf.cpp
#TEST_SOURCES += test_backtrace.cpp
#TEST_SOURCES += test_broadcast_config.cpp
#TEST_SOURCES += test_parse_config.cpp

TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)
TEST_TARGETS = $(TEST_SOURCES:.cpp=.exe)

LIB_SOURCES = configuration.cpp runtime.cpp callstack.cpp csmpi.cpp
LIB_OBJECTS = $(LIB_SOURCES:.cpp=.o)
LIB_TARGET = libcsmpi.so

all: $(TEST_TARGETS) $(LIB_TARGET) $(TEST_APP_TARGET)

lib: $(LIB_TARGET)

tests: $(TEST_TARGETS)

testapp: $(TEST_APP_TARGET)

$(TEST_APP_TARGET): $(TEST_APP_OBJECTS) 
	$(MPICC) $(LDFLAGS) -o $@ $(TEST_APP_OBJECTS) $(LD_LIBRARY_FLAGS) $(LIBS)

$(LIB_TARGET): $(LIB_OBJECTS)
	$(MPICXX) $(LDFLAGS) -shared -o $@ $(LIB_OBJECTS) $(LD_LIBRARY_FLAGS) $(LIBS)

$(TEST_TARGETS): $(TEST_OBJECTS) $(LIB_OBJECTS)
	$(MPICXX) $(LDFLAGS) -o $@ $(TEST_OBJECTS) $(LD_LIBRARY_FLAGS) $(LIB_OBJECTS) $(LIBS)  

%.o: %.cpp
	$(MPICXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.c
	$(MPICC) $(CPPFLAGS) $(CFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	$(RM) *.o
	$(RM) *.so
	$(RM) *.exe
