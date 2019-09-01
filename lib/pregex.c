/* -MODULE----------------------------------------------------------------------
Phorward C/C++ Library
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
https://phorward.info ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	pregex.c
Usage:	A pregex object demonstration suite.
----------------------------------------------------------------------------- */

#include "local.h"

void help( char** argv )
{
	printf( "Usage: %s OPTIONS {expression} input\n\n"

	"   -a  --action    ACTION    Perform regular expression action:\n"
	"                             match (default), find, split, replace\n"
	"   -d  --delimiter STRING    Use STRING as result delimiter\n"
	"                             (newline is default)\n"
	"   -D                        Dump constructed DFA\n"
	"   -e  --exec      EXPR      Use string EXPR as expression.\n"
	"   -f  --file      FILENAME  Read expression from FILENAME\n"
	"   -h  --help                Show this help, and exit.\n"
	"   -i  --input     INPUT     Use string INPUT as input.\n"
	"   -r  --replace   STRING    Replacement string for 'replace' action.\n"
	"   -V  --version             Show version info and exit.\n"

	"\n", *argv );
}

int main( int argc, char** argv )
{
	char*		actions[]	= { "match", "find", "split", "replace" };
	char*		action		= actions[0];
	char*		fexpr		= (char*)NULL;
	char*		expr		= (char*)NULL;
	char*		finput		= (char*)NULL;
	char*		input		= (char*)NULL;
	char*		replace		= "";
	char*		delimiter	= "\n";
	char*		start;
	char*		end;
	char*		nstart;
	pboolean	dump		= FALSE;
	pregex*		re;

	int			i;
	int			j;
	int			rc;
	int			next;
	char		opt		[ 10 + 1 ];
	char*		param;

	PROC( "pregex" );

	/* Analyze command-line parameters */
	for( i = 0; ( rc = pgetopt( opt, &param, &next, argc, argv,
						"a:d:De:hf:i:r:V",
						"action: delimiter: exec: file: help "
							"input: replace: version", i ) )
								== 0; i++ )
	{
		VARS( "opt", "%s", opt );
		VARS( "param", "%s", param );

		if( !strcmp( opt, "action" ) || !strcmp( opt, "a" ) )
		{
			action = (char*)NULL;

			for( j = 0; j < sizeof(actions) / sizeof(*actions); j++ )
				if( pstrcasecmp( param, actions[j] ) == 0 )
				{
					action = actions[j];
					break;
				}

			if( !action )
			{
				fprintf( stderr, "Invalid action '%s'\n", param );
				help( argv );
				RETURN( 1 );
			}

		}
		else if( !strcmp( opt, "delimiter" ) || !strcmp( opt, "d" ) )
			delimiter = pstrunescape( param );
		else if( !strcmp( opt, "exec" ) || !strcmp( opt, "e" ) )
			expr = param;
		else if( !strcmp( opt, "file" ) || !strcmp( opt, "f" ) )
		{
			fexpr = pfree( fexpr );

			if( !pfiletostr( &fexpr, param ) )
			{
				fprintf( stderr, "Unable to read expression file '%s'\n",
					param );
				RETURN( 1 );
			}

			expr = fexpr;
		}
		else if( !strcmp( opt, "help" ) || !strcmp( opt, "h" )  )
		{
			help( argv );
			RETURN( 0 );
		}
		else if( !strcmp( opt, "input" ) || !strcmp( opt, "i" ) )
			input = param;
		else if( !strcmp( opt, "replace" ) || !strcmp( opt, "r" ) )
			replace = param;
		else if( !strcmp( opt, "version" ) || !strcmp( opt, "V" ) )
		{
			version( argv, "Regular expression command-line utility" );
			RETURN( 0 );
		}
	}

	VARS( "rc", "%d", rc );

	if( rc == 1 )
	{
		if( !expr && argc > next )
			expr = argv[next++];

		if( !input && argc > next )
		{
			if( !pfiletostr( &finput, argv[next] ) )
				input = argv[next];
			else
				input = finput;

			next++; /* <- obsolete, but complete ;-) */
		}
	}

	VARS( "expr", "%s", expr );
	VARS( "input", "%s", input );

	if( !( expr && input ) || ( rc < 0 && param ) )
	{
		if( rc < 0 && param )
			fprintf( stderr, "Unknown option '%s'\n", param );
		else if( argc > 1 )
			fprintf( stderr, "Too less parameters given.\n" );

		help( argv );
		RETURN( 1 );
	}

	/* Process */

	start = input;
	re = pregex_create( expr, PREGEX_FLAG_NONE );

	VARS( "action", "%s", action );

	if( strcmp( action, "match" ) == 0 )
	{
		if( pregex_match( re, start, &end ) )
			printf( "%.*s%s", end - start, start, delimiter );
	}
	else if( strcmp( action, "find" ) == 0 )
	{
		while( ( start = pregex_find( re, start, &end ) ) )
		{
			printf( "%.*s%s", end - start, start, delimiter );
			start = end;
		}
	}
	else if( strcmp( action, "split" ) == 0 )
	{
		while( start )
		{
			if( pregex_split( re, start, &end, &nstart ) )
				printf( "%.*s%s", end - start, start, delimiter );

			start = nstart;
		}
	}
	else if( strcmp( action, "replace" ) == 0 )
	{
		VARS( "replace", "%s", replace );

		start = pregex_replace( re, start, replace );
		printf( "%s", start );

		pfree( start );
	}

	fflush( stdout );

	pfree( finput );
	pfree( fexpr );

	RETURN( 0 );
}

