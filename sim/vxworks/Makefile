CCPPC = ccppc
CCPPCFLAGS = -mhard-float -mstrict-align -mregnames -O3 \
	-fstrength-reduce -fno-builtin \
	-DCPU=PPC32 -DTOOL_FAMILY=gnu -DTOOL=gnu -DEOS_UNSAFE_LOG \
	-I$(WIND_BASE)/vxworks-6.9/target/h
LIBS = -I../../eos -I../ -L../../eos -leos

# Output binary object file for compiled VxWorks sim code
SIMULATION = lib_run_sim.o

# List of C files to include in the VxWorks sim
SIMULATION_SRC = ethemis.c

.PHONY: sim_objects

all: $(SIMULATION)

$(SIMULATION): sim_objects
	# Includes the VxWorks objects from this directory, the parent
	# directory, and the main libeos library
	ldppc -o $(SIMULATION) -r *.ov ../*.ov ../../eos/libeosvxw.a

sim_objects: $(SIMULATION_SRC) clean
	$(CCPPC) $(CCPPCFLAGS) $(LIBS) -c $(SIMULATION_SRC)
	for f in *.o; do mv $$f $$f"v"; done

clean:
	rm -rf *.ov *.o
