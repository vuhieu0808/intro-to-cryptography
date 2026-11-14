ifeq ($(ROOT),)
ROOT:=.
endif

CPP:=g++
CPP_FLAGS:=-std=c++14 -O3 -Wall -Wextra -Wpedantic -Wvla -Wredundant-decls

SHA3_SRC:=$(ROOT)/lib/fips202.c
SHA3_INCLUDE:=-I $(ROOT)/lib

SRC:=$(ROOT)
INCLUDE:=-I $(ROOT) -pthread
LIB:=$(SHA3_INCLUDE)

MAIN_PRNG:=$(ROOT)/main_prng.cpp
PRNG_SRC:=$(ROOT)/lib/prng.cpp

BIN:=bin
BUILD:=bin/build

.PHONY: all folders prng run-prng test-bigint test-modular test-all clean

all: prng

folders:
	@echo -e "\n### Creating folders\n"
	mkdir -p $(BUILD) $(BIN)

prng: folders
	@echo -e "\n### Compiling prng\n"
	$(CPP) $(CPP_FLAGS) $(MAIN_PRNG) $(PRNG_SRC) $(SHA3_SRC) $(LIB) -o $(BIN)/prng

run-prng: prng
	./$(BIN)/prng

test-bigint: folders
	@echo -e "\n### Compiling and running bigintTest\n"
	$(CPP) $(CPP_FLAGS) $(ROOT)/test/bigintTest.cpp -o $(BIN)/bigintTest
	./$(BIN)/bigintTest

test-modular: folders
	@echo -e "\n### Compiling and running modularTest\n"
	$(CPP) $(CPP_FLAGS) $(ROOT)/test/modularTest.cpp -o $(BIN)/modularTest
	./$(BIN)/modularTest

test-dh: folders
	@echo -e "\n### Compiling and running dhTest\n"
	$(CPP) $(CPP_FLAGS) $(ROOT)/test/dhTest.cpp $(ROOT)/dh.cpp $(PRNG_SRC) $(SHA3_SRC) $(INCLUDE) $(LIB) -o $(BIN)/dhTest
	./$(BIN)/dhTest

test-all: test-bigint test-modular

clean:
	rm -rf ./bin
