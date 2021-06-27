# specfile/readspec Makefile

# gcc compiler
CC = gcc

# g++ compiler
CXX = g++

# gcc compiler flags
CFLAGS = -O3 -fopenmp -march=native -Wall -I.

# g++ compiler flags
CXXFLAGS = -O3 -fopenmp -march=native -Wall -I.

# g++ linker
CXX_LD = $(CXX)

#gcc linker
LD = $(CC)

# linker flags
LDFLAGS = -fopenmp

# name of executable
EXE = qfind

# list of all sources
SOURCES = qfind.c

# list of all objects
OBJECTS = $(SOURCES:.c=.o)

# Makefile
all: qfind
qfind: $(OBJECTS)
	$(LD) -o $(EXE) $(OBJECTS) $(LDFLAGS)
%.o: %.c common.hpp
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
# Cleaning everything
.PHONY : clean
clean:
	rm -f $(EXE) $(OBJECTS)
# End
