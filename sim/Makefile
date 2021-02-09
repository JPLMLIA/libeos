#CC = clang or gcc, if needed
# -g : allows use of GNU Debugger
# -Wall : show all warnings
CCPPC = ccppc
CCPPCFLAGS = -mhard-float -mstrict-align -mregnames -O3 \
	-fstrength-reduce -fno-builtin \
	-DCPU=PPC32 -DTOOL_FAMILY=gnu -DTOOL=gnu -DEOS_UNSAFE_LOG -DEOS_NO_LIBCONFIG \
	-std=iso9899:1999 \
	-I$(WIND_BASE)/vxworks-6.9/target/h
CCPPCLIBS = -I../eos -L../eos -leos -lm
CFLAGS = -g -Wall -Wextra -O3 -std=iso9899:1999 -D_XOPEN_SOURCE=700
OS := $(shell uname)
ifeq ($(OS),Darwin)
# -lprofiler : enable use of gperftools
# -Wl,-no_pie : disable ASLR so function names are legible
	LIBS = -I../eos -L../eos -leos -largp -lconfig -lprofiler -Wl,-no_pie -lm
else
# -pg : enable use of gprof
	LIBS = -I../eos -L../eos -leos -pg -lconfig -lm
endif

ifdef CONFIG_MODE
ifeq ($(CONFIG_MODE),EOS_NO_LIBCONFIG)
	CFLAGS += -DEOS_NO_LIBCONFIG
else
    $(error Unrecognized value $(CONFIG_MODE) for CONFIG_MODE)
endif
endif

ifdef PIMS_COUNT_T
ifeq ($(PIMS_COUNT_T),U16)
	CCPPCFLAGS += -DEOS_PIMS_U16_DATA
	CFLAGS += -DEOS_PIMS_U16_DATA
else
    $(error Unrecognized value $(PIMS_COUNT_T) for PIMS_COUNT_T)
endif
endif

# Location of destination for compiled binaries
BIN = ../bin

# Destination of the Linux simulation binary
SIMULATION = $(BIN)/run_sim

# Source code files required for Linux simulation
SIMULATION_SRC = run_sim.c sim_ethemis.c sim_mise.c sim_pims.c \
	sim_log.c sim_io.c sim_util.c

# Shared code also used by VxWorks simulation
VXWORKS_SIM_SRC = sim_log.c sim_io.c sim_util.c

.PHONY: bf_test vxworks

all: $(SIMULATION)

vxworks: simvxworks
	make -C vxworks

simvxworks: $(VXWORKS_SIM_SRC)
	$(CCPPC) $(CCPPCFLAGS) $(CCPPCLIBS) -c $(VXWORKS_SIM_SRC)
	for f in *.o; do mv $$f $$f"v"; done

bf_test: $(SIMULATION_SRC) clean
	$(CC) $(CFLAGS) $(SIMULATION_SRC) -o $(SIMULATION) -D EOS_BF_TEST $(LIBS) 

$(SIMULATION): $(SIMULATION_SRC) clean
	$(CC) $(CFLAGS) $(SIMULATION_SRC) -o $(SIMULATION) $(LIBS) 

clean:
	rm -rf *.dSYM *.ov
	make -C vxworks clean
