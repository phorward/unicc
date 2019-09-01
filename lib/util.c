/* -MODULE----------------------------------------------------------------------
Phorward C/C++ Library
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
https://phorward.info ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	util.c
Usage:	Utility library for the command-line tools.
----------------------------------------------------------------------------- */

#include "phorward.h"

void version( char** argv, char* descr )
{
	printf( "%s v%s\n", *argv, LIBPHORWARD_VERSION );

	if( descr && *descr )
		printf( "%s. Part of the Phorward C/C++ Library.\n\n", descr );

	printf( "Copyright (C) 2006-2019 by Phorward Software Technologies, "
				"Jan Max Meyer\n"
			"All rights reserved. See LICENSE for more information.\n" );
}
