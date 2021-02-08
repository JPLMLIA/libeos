#CC = clang or gcc, if needed
# -g : allows use of GNU Debugger
# -Wall : show all warnings
CFLAGS = -g -Wall -Wextra -O3 -std=iso9899:1999 -coverage -D_XOPEN_SOURCE=700
OS := $(shell uname)
ifeq ($(OS),Darwin)
	LIBS = -I../eos -L../eos -leoscov -lm
else
	LIBS = -I../eos -L../eos -leoscov -lgcov -lm
endif
BIN = ../bin
CUTEST = $(BIN)/run_tests
CUTEST_SRC = run_tests.c CuTest.c util.c \
	memory_test.c log_test.c param_test.c util_test.c \
	ethemis_test.c data_test.c eos_test.c \
	mise_test.c heap_test.c

all: $(CUTEST)

$(CUTEST): $(CUTEST_SRC) clean_coverage
	$(CC) $(CFLAGS) $(CUTEST_SRC) -o $(CUTEST) $(LIBS) 

clean_coverage:
	@# Using the '@' sign suppresses echoing
	@# the line while the command is run
	rm -rf *.gcno *.gcda

clean: clean_coverage
	@# Using the '@' sign suppresses echoing
	@# the line while the command is run
	rm -rf *.dSYM
