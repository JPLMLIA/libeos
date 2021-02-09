# Make test
EOSTESTDATA:=test/data
export EOSTESTDATA

# Suppress malloc error messages
MallocLogFile:=/dev/null
MallocDebugReport:=none
export MallocLogFile
export MallocDebugReport

VXWORKS_BIN:=/scratch/europa/bin # SET PATH HERE

# Check if CodeQL repository is unassigned.
ifndef CODEQL_QUERIES_PATH
	CODEQL_QUERIES_PATH=/dev/null # SET PATH HERE
endif

# Outputs of static analysis tools.
CODESONAR_OUTPUT:=libeos-codesonar
CODEQL_OUTPUT:=libeos-codeql

.PHONY: all docs test sim vxworks

all: test

docs:
	make -C docs html

clean: clean_eos clean_test

cleaner: cleaner_eos

libeos:
	make -C eos

libeoscov:
	make -C eos libeoscov.a

vxworks:
	make -C eos libeosvxw.a
	make -C sim vxworks
	cp sim/vxworks/lib_run_sim.o $(VXWORKS_BIN)/

test: libeoscov
	make -C test

sim: libeos
	make -C sim

bf_test_sim: libeos
	make -C sim bf_test

clean_eos:
	make -C eos clean

cleaner_eos:
	make -C eos cleaner

clean_test:
	make -C test clean

run_tests: test
	./bin/run_tests

mem_tests: test
	valgrind --leak-check=full ./bin/run_tests

coverage: run_tests
	mkdir -p test/coverage
	lcov --capture --directory eos --output-file test/coverage/coverage.info
	genhtml test/coverage/coverage.info --output-dir test/coverage

codesonar: clean
	codesonar analyze $(CODESONAR_OUTPUT) make libeos

codeql-create: clean
	rm -r $(CODEQL_OUTPUT)
	codeql database create $(CODEQL_OUTPUT) --language=cpp --command="make libeos"

codeql:
	codeql database analyze $(CODEQL_OUTPUT) $(CODEQL_QUERIES_PATH) --threads=0 --format=csv --output=$(CODEQL_OUTPUT).csv
