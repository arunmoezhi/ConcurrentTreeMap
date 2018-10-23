TBB_INC=${TBBROOT}/include
TBB_LIB=${TBBROOT}/lib/intel64
TBBFLAGS=-I$(TBB_INC) -Wl,-rpath,$(TBB_LIB) -L$(TBB_LIB) -ltbb -ltbbmalloc_proxy -ltbbmalloc
GSLFLAGS=-I$(TACC_GSL_INC) -Wl,-rpath,$(TACC_GSL_LIB) -L$(TACC_GSL_LIB) -lgsl -lgslcblas
CC=icpc
CFLAGS= -O3 -lrt -lpthread -std=c++11 -march=native 
SRC1= ./src/Test.cpp
OBJ1= ./bin/ConcurrentTreeMap.o
all: 
	$(CC) $(CFLAGS) $(TBBFLAGS) $(GSLFLAGS) -o $(OBJ1) $(SRC1)
clean:
	rm -rf ./bin/*.*
