#CC = clang or gcc, if needed
# -g : allows use of GNU Debugger
# -Wall : show all warnings
CFLAGS = -fPIC -g -Wall -Wextra -O3 -pedantic -std=iso9899:1999 \
      -Wshadow \
      -Wpointer-arith \
      -Wcast-qual
CCPPC = ccppc
CCPPCFLAGS = -mhard-float -mstrict-align -mregnames -O3 \
	-fstrength-reduce -fno-builtin \
	-DCPU=PPC32 -DTOOL_FAMILY=gnu -DTOOL=gnu -DEOS_UNSAFE_LOG \
	-I$(WIND_BASE)/vxworks-6.9/target/h
OS := $(shell uname)
ifeq ($(OS),Darwin)
	LIBS =
else
	CC=gcc
	LIBS = -lm -lgcov
endif
EOS_SRC = eos.c eos_memory.c eos_log.c eos_util.c eos_params.c \
	eos_ethemis.c eos_mise.c eos_data.c eos_heap.c
EOS_O = eos.oa
EOS_OCOV = eos.oc
EOS_OVXW = eos.ov
LIBEOS = libeos.a
LIBEOSCOV = libeoscov.a
LIBEOSVXW = libeosvxw.a

all: $(LIBEOS)

$(LIBEOS): $(EOS_O)
	ar ruv $(LIBEOS) *.oa
	ranlib $(LIBEOS)

$(LIBEOSVXW): $(EOS_OVXW)
	ldppc -o $(LIBEOSVXW) -r *.ov

$(EOS_O): $(EOS_SRC)
	$(CC) $(CFLAGS) -c $(EOS_SRC) $(LIBS) 
	for f in *.o; do mv $$f $$f"a"; done

$(EOS_OVXW): $(EOS_SRC) clean_coverage
	$(CCPPC) $(CCPPCFLAGS) -c $(EOS_SRC)
	for f in *.o; do mv $$f $$f"v"; done

$(LIBEOSCOV): $(EOS_OCOV)
	ar ruv $(LIBEOSCOV) *.oc
	ranlib $(LIBEOSCOV)

$(EOS_OCOV): $(EOS_SRC) clean_coverage
	$(CC) $(CFLAGS) -coverage -c $(EOS_SRC) $(LIBS) 
	for f in *.o; do mv $$f $$f"c"; done

clean_coverage:
	@# Using the '@' sign suppresses echoing
	@# the line while the command is run
	rm -rf *.gcno *.gcda

clean: clean_coverage
	@# Using the '@' sign suppresses echoing
	@# the line while the command is run
	rm -rf *.o *.oa *.oc *.ov *.gcno *.gcda

cleaner: clean
	rm -rf *.a
