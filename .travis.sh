#!/bin/sh
set -ex

LIBPHORWARD_VERSION="0.24.0"

# Phorward C/C++ Library
wget https://github.com/phorward/phorward/archive/v${LIBPHORWARD_VERSION}.tar.gz
tar xvfz v${LIBPHORWARD_VERSION}.tar.gz
cd phorward-${LIBPHORWARD_VERSION}
./configure --prefix=/usr
make
sudo make install
cd ..

# UniCC
touch aclocal.m4 configure Makefile.am Makefile.in
./configure
make
sudo make install
unicc -V
make -f Makefile.gnu test
