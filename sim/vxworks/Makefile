CCPPC = ccppc
CCPPCFLAGS = -mhard-float -mstrict-align -mregnames -O3 \
	-fstrength-reduce -fno-builtin \
	-DCPU=PPC32 -DTOOL_FAMILY=gnu -DTOOL=gnu -DEOS_UNSAFE_LOG -DEOS_NO_LIBCONFIG \
	-std=iso9899:1999 \
	-I$(WIND_BASE)/vxworks-6.9/target/h
CCPPCLIBS = -L $(WIND_BASE)/vxworks-6.9/target/lib/ppc/PPC32/common/ -lgcc
LIBS = -I../../eos -I../ -L../../eos -leos

ifdef CONFIG_MODE
ifneq ($(CONFIG_MODE),EOS_NO_LIBCONFIG)
    $(error Unrecognized value $(CONFIG_MODE) for CONFIG_MODE)
endif
endif

ifdef PIMS_COUNT_T
ifeq ($(PIMS_COUNT_T),U16)
	CCPPCFLAGS += -DEOS_PIMS_U16_DATA
else
    $(error Unrecognized value $(PIMS_COUNT_T) for PIMS_COUNT_T)
endif
endif

# Output binary object file for compiled VxWorks sim code
SIMULATION = lib_run_sim.o

# List of C files to include in the VxWorks sim
SIMULATION_SRC = ethemis.c mise.c ../sim_pims.c

.PHONY: sim_objects

all: $(SIMULATION)

$(SIMULATION): sim_objects
	# Includes the VxWorks objects from this directory, the parent
	# directory, and the main libeos library
	ldppc -o $(SIMULATION) -r *.ov ../*.ov ../../eos/libeosvxw.a $(CCPPCLIBS)

sim_objects: $(SIMULATION_SRC) clean
	$(CCPPC) $(CCPPCFLAGS) $(LIBS) -c $(SIMULATION_SRC)
	for f in *.o; do mv $$f $$f"v"; done

clean:
	rm -rf *.ov *.o
