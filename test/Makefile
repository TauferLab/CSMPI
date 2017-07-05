CC = mpicc
CFLAGS = -g -Wall -std=c++11
INCLUDE = 
LDFLAGS = 
LIBS = -lstdc++ -lunwind

PMPI_MODULE = /g/g17/chapp1/callstack_mpi/build/callstack_mpi.o

SRCS = nested_isend.cpp
OBJS = $(SRCS:.c=.o)
EXEC = $(OBJS:.o=.exe)

all:
	$(CC) $(CFLAGS) $(INCLUDE) -c nested_isend.cpp -o nested_isend.o
	$(CC) nested_isend.o $(PMPI_MODULE) $(LDFLAGS) $(LIBS) -o nested_isend 

clean:
	rm -f nested_isend.o
	rm -f nested_isend.exe
