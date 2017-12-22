#!/bin/sh
if [ ! -f ../../unicc.pdf ]
then
	wget -o ../../unicc.pdf https://www.phorward-software.com/download/unicc/unicc.pdf
fi

if [ ! -x ../../unicc.exe ]
then
	echo "unicc.exe does not exist..."

	cd ../..
	LDFLAGS="-L /lib" CFLAGS="-I /include" CPPFLAGS="-I /include" ./configure
	make
	cd -
fi

