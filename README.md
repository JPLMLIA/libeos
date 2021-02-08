LIBEOS
======

# Dependencies

## Argument Parsing

It might be necessary on macOS to set the `LIBRARY_PATH` and `C_INCLUDE_PATH`
environment variables to the location of the `argp` library for argument
parsing. On Linux it should be part of the GNU standard library.

## Configuration Parsing

The `libconfig` library is required for configuration parsing. The
MacPorts port is called `libconfig-hr`, and in homebrew it is
`libconfig`. On MLIA Linux machines, it is probably the
`libconfig-devel` package that needs to be installed. However, it's
also possible to install the library locally via the following:

```Console
$ mkdir -p $HOME/.local
$ wget http://hyperrealm.github.io/libconfig/dist/libconfig-1.7.2.tar.gz
$ tar xvzf libconfig-1.7.2.tar.gz
$ cd libconfig-1.7.2
$ ./configure --prefix=$HOME/.local
$ make
$ make install
```

then add the following to your `.bashrc` file (create if it doesn't exist):

```bash
export LIBRARY_PATH=$HOME/.local/lib:$LIBRARY_PATH
export C_INCLUDE_PATH=$HOME/.local/include:$C_INCLUDE_PATH
export LD_LIBRARY_PATH=$HOME/.local/lib:$LD_LIBRARY_PATH
```

or, if you're using tcsh, then add to your `.tcshrc` file (create if it doesn't exist):

```tcsh
if ( ! $?LIBRARY_PATH ) then
        setenv LIBRARY_PATH ""
else
        setenv LIBRARY_PATH ":${LIBRARY_PATH}"
endif
setenv LIBRARY_PATH $HOME/.local/lib${LIBRARY_PATH}

if ( ! $?C_INCLUDE_PATH ) then
        setenv C_INCLUDE_PATH ""
else
        setenv C_INCLUDE_PATH ":${C_INCLUDE_PATH}"
endif
setenv C_INCLUDE_PATH $HOME/.local/include${C_INCLUDE_PATH}

if ( ! $?LD_LIBRARY_PATH ) then
        setenv LD_LIBRARY_PATH ""
else
        setenv LD_LIBRARY_PATH ":${LD_LIBRARY_PATH}"
endif
setenv LD_LIBRARY_PATH $HOME/.local/lib${LD_LIBRARY_PATH}
```

## BITFLIPS

See the [BITFLIPS](https://github.com/JPLMLIA/bitflips) repository for
installation instructions. If you would like to install locally (recommended),
use the following steps in place of the corresponding instructions:

```Console
$ ./autogen.sh
$ ./configure --prefix=$HOME/.local
$ make
$ make install
```

If you haven't already, fill in the `.bashrc`/`.tcshrc` file as specified for other dependencies
above.

Finally, run, for bash:
```bash
source ~/.bashrc
```
or, for tcsh:
```tcsh
source ~/.tcshrc
```

# Documentation

Library documentation is built using [Sphinx](http://www.sphinx-doc.org) with
the [Hawkmoth](https://hawkmoth.readthedocs.io/) plugin for C
auto-documentation. The built environment requires Python 3.7, and Python
bindings for Clang. It is recommended that the py37-clang package be installed
via a system package manager (e.g., MacPorts), then any venv created for Python
3.7 reference the `--system-site-packages` to import the `clang` library that is
properly linked to the system libraries. In addition to `sphinx` and `hawkmoth`,
the `sphinx_rtd_theme` package is also required.

After installing dependencies, documentation can be compiled using `make docs`.

## Simulation Framework

The `sim` directory contains code to simulate the execution of EOS functionality
within the context of a larger FSW system. More details are included in the
README under the `sim` directory. Two Makefile targets, `make sim` and `make
vxworks` can be used within the root of this repo to build the Linux and VxWorks
versions of the simulation code, respectively. Some modification of the
Makefile is necessary depending on where VxWorks is installed on the target
machine.

## Docker

See the contents of the `docker` directory for running the simulator within a
Docker container.

# Copyright

Copyright 2021, by the California Institute of Technology. ALL RIGHTS RESERVED.
United States Government Sponsorship acknowledged. Any commercial use must be
negotiated with the Office of Technology Transfer at the California Institute
of Technology.
