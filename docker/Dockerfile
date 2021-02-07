FROM ubuntu:18.04

# Install BITFLIPS.
RUN wget https://sourceware.org/pub/valgrind/valgrind-3.16.1.tar.bz2
RUN tar jxvf valgrind-3.16.1.tar.bz2
RUN cd valgrind-3.16.1
RUN git clone https://github.com/JPLMLIA/BITFLIPS.git
RUN cp valgrind_replacements/* .
RUN ./autogen.sh
RUN ./configure
RUN make
RUN make install

# Install libconfig.
RUN mkdir -p $HOME/.local
RUN wget http://hyperrealm.github.io/libconfig/dist/libconfig-1.7.2.tar.gz
RUN tar xvzf libconfig-1.7.2.tar.gz
RUN cd libconfig-1.7.2
RUN ./configure --prefix=$HOME/.local
RUN make
RUN make install
ENV LIBRARY_PATH=$HOME/.local/lib:$LIBRARY_PATH
ENV C_INCLUDE_PATH=$HOME/.local/include:$C_INCLUDE_PATH
ENV LD_LIBRARY_PATH=$HOME/.local/lib:$LD_LIBRARY_PATH

# Download libeos simulator source code.
RUN git clone https://github.com/JPLMLIA/libeos.git

CMD /bin/sh
