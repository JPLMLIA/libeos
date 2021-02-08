EOS Simulation Framework
========================

This directory contains code to simulate the execution of EOS functionality
within the context of a larger FSW system. There are simulation environments for
running code both within Linux and VxWorks, which can be compiled using `make
sim` or `make vxworks` within the repository root directory.

## Linux Framework

The compiled binary for Linux is placed at `bin/run_sim` from the root of the
repository. This framework is intended to allow file-based configuration and
enables fully executing the functionality of the library. See command-line help
for more instruction for running.

## VxWorks Framework

The VxWorks framework is compiled into a single `.o` file that can be loaded
into VxWorks from the shell on one of the SBCs attached to `aisbc`. The
framework is currently placed under `/scratch/europa/bin` on `aisbc` (the only
system currently supported for running this framework). The functionality within
the VxWorks files under the `vxworks` directory must also be called directly
from the VxWorks shell. Auto-generated VxWorks shell scripts for calling the
provided with appropriate arguments is recommended for testing, benchmarking,
and other purposes.

## Code Organization

The code is currently organized as follows:
 - **The Linux CLI** is implemented in `run_sim.c`
 - **Instrument-specific Linux simulation code** is implemented within the
   instrument specific files (e.g., `sim_ethemis.c` and `sim_mise.c`).
 - **Shared simulation functionality** for both Linux and VxWorks for logging,
   file I/O, and other utilities are in `sim_io.c`, `sim_log.c`, and
   `sim_util.c`.
 - **VxWorks Simulations** are located under the `vxworks` directory, and
   provide a subset of the full framework's functionality as required for
   benchmarking on the PPC750 boards

## Development

The simulation framework has some basic requirements for setting up and tearing
down that should be followed in newly developed instrument-specific simulations.
First, the logging framework should be initialized using `sim_log_init` found in
`sim_log.c`. Then, the EOS library itself should be initialized using
`eos_init`, passing in the `log_function` from `sim_log.c`. After executing the
simulation, tear down using `eos_teardown` and `sim_log_teardown`.
