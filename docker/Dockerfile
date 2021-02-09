FROM ubuntu:18.04

# Copy files from the build directory.
WORKDIR /home/
COPY valgrind_replacements valgrind_replacements
COPY test_data test_data

# Install wget, git and other installation dependencies.
RUN  apt-get update \
  && apt-get install -y wget \
  && apt-get install -y git \
  && apt-get install -y autotools-dev \
  && apt-get install -y automake \
  && apt-get install -y build-essential \
  && apt-get install -y python \ 
  && rm -rf /var/lib/apt/lists/*

# Install BITFLIPS.
RUN wget https://sourceware.org/pub/valgrind/valgrind-3.16.1.tar.bz2
RUN tar jxvf valgrind-3.16.1.tar.bz2 
RUN cd valgrind-3.16.1 \
 && git clone https://github.com/JPLMLIA/bitflips.git \
 && cp -r /home/valgrind_replacements/* . \
 && ./autogen.sh \
 && ./configure \
 && make \
 && make install

# Install libconfig.
RUN mkdir -p /home/.local
RUN wget http://hyperrealm.github.io/libconfig/dist/libconfig-1.7.2.tar.gz
RUN tar xvzf libconfig-1.7.2.tar.gz
RUN cd libconfig-1.7.2 \
 && ./configure --prefix=/home/.local \
 && make \
 && make install

ENV LIBRARY_PATH=/home/.local/lib:$LIBRARY_PATH
ENV C_INCLUDE_PATH=/home/.local/include:$C_INCLUDE_PATH
ENV LD_LIBRARY_PATH=/home/.local/lib:$LD_LIBRARY_PATH

# Download libeos simulator source code.
RUN git clone https://github.com/JPLMLIA/libeos.git

# Start a Bash shell.
CMD /bin/sh
