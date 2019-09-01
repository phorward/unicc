#!/bin/sh
set -ex

touch aclocal.m4 configure Makefile.am Makefile.in
./configure
make
sudo make install

unicc -V

make -f Makefile.gnu test
