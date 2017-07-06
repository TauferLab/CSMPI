CC = mpicc
CFLAGS = -g -Wall -std=c++11
INCLUDE = 
LDFLAGS = -L/g/g17/chapp1/callstack_mpi/build/lib
LIBS = -lcallstack_mpi -lstdc++ -lunwind

PMPI_MODULE = #/g/g17/chapp1/callstack_mpi/build/callstack_mpi.so

SRCS = nested_isend_templated.cpp
OBJS = $(SRCS:.c=.o)
EXEC = $(OBJS:.o=.exe)

all:
	$(CC) $(CFLAGS) $(INCLUDE) -c nested_isend_templated.cpp -o nested_isend_templated.o
	$(CC) nested_isend_templated.o $(PMPI_MODULE) $(LDFLAGS) $(LIBS) -o nested_isend_templated.exe 

clean:
	rm -f nested_isend_templated.o
	rm -f nested_isend_templated.exe
