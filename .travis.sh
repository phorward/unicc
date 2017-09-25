#!/bin/sh
set -ex
./configure
make
sudo make install
unicc -V
