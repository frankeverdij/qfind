# specfile/readspec Makefile

# gcc compiler
CC = gcc

# g++ compiler
CXX = g++

# gcc compiler flags
CFLAGS = -g -O3 -fopenmp -march=native -Wall -I.

# g++ compiler flags
CXXFLAGS = -O3 -fopenmp -march=native -Wall -I.

# NOCACHE directive
 CFLAGS += -DNOCACHE
 CXXFLAGS += -DNOCACHE

# g++ linker
CXX_LD = $(CXX)

#gcc linker
LD = $(CC)

# linker flags
LDFLAGS = -fopenmp

# name of executable
EXE = qfind

# list of all sources
SOURCES = qfind.c \
          src/gcd.c src/insort.c src/loadfile.c src/parserule.c src/putnum.c \
          src/evolve.c src/echoparams.c src/makephases.c src/finalreport.c \
          src/cache.c

# list of all objects
OBJECTS = $(SOURCES:.c=.o)

# Makefile
all: qfind
qfind: $(OBJECTS)
	$(LD) -o $(EXE) $(OBJECTS) $(LDFLAGS)
%.o: %.c common.h
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.cpp common.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
# Cleaning everything
.PHONY : clean
clean:
	rm -f $(EXE) $(OBJECTS)
# End
