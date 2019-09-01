/* -MODULE----------------------------------------------------------------------
Phorward C/C++ Library
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
https://phorward.info ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	plex.c
Usage:	A plex object demonstration suite.
----------------------------------------------------------------------------- */

#include "local.h"

void help( char** argv )
{
	printf( "Usage: %s OPTIONS patterns...\n\n"

	"   -b  --begin     STRING    Use STRING as result begin separator\n"
	"                             (' >' is default)\n"
	"   -d  --dot                 Dump scanner as graph for visualizations in\n"
	"                             Graphviz dot-format\n"
	"   -e  --end       STRING    Use STRING as result end separator\n"
	"                             ('<\\n' is default)\n"
	"   -f  --file      FILENAME  Read input from FILENAME\n"
	"   -h  --help                Show this help, and exit.\n"
	"   -i  --input     INPUT     Use string INPUT as input.\n"
	"   -V  --version             Show version info and exit.\n"


	"\n", *argv );
}

int main( int argc, char** argv )
{
	char*		finput		= (char*)NULL;
	char*		begin_sep	= " >";
	char*		end_sep		= "<\n";
	char*		start		= (char*)NULL;
	char*		end;
	plex*		lex;
	int			id;
	pboolean	dot			= FALSE;

	int			i;
	int			rc;
	int			next;
	char		opt			[ 10 + 1 ];
	char*		param;
	size_t		size		= 0;

	/* Analyze command-line parameters */
	for( i = 0; ( rc = pgetopt( opt, &param, &next, argc, argv,
						"b:de:Df:hi:V",
						"begin: dot end: file: "
							"help input: version", i ) ) == 0; i++ )
	{
		if( !strcmp( opt, "begin" ) || !strcmp( opt, "b" ) )
			begin_sep = pstrunescape( param );
		else if( !strcmp( opt, "dot" ) || !strcmp( opt, "d" ) )
			dot = TRUE;
		else if( !strcmp( opt, "end" ) || !strcmp( opt, "e" ) )
			end_sep = pstrunescape( param );
		else if( !strcmp( opt, "file" ) || !strcmp( opt, "f" ) )
		{
			finput = pfree( finput );

			if( !pfiletostr( &finput, param ) )
			{
				fprintf( stderr, "Unable to read input file '%s'\n", param );
				return 1;
			}

			start = finput;
		}
		else if( !strcmp( opt, "help" ) || !strcmp( opt, "h" )  )
		{
			help( argv );
			return 0;
		}
		else if( !strcmp( opt, "input" ) || !strcmp( opt, "i" ) )
			start = param;
		else if( !strcmp( opt, "version" ) || !strcmp( opt, "V" ) )
		{
			version( argv, "Lexical analysis command-line utility" );
			return 0;
		}
	}

	if( rc < 0 && param )
	{
		fprintf( stderr, "Unknown option '%s'\n", param );
		help( argv );
		return 1;
	}

	lex = plex_create( 0 );

	for( i = 0; next < argc; next++, i++ )
		plex_define( lex, argv[next], i + 1, 0 );

	if( !i )
	{
		plex_free( lex );

		if( argc > 1 )
			fprintf( stderr, "Too less parameters given.\n" );

		help( argv );
		return 1;
	}

	/* Dump to dot */
	if( dot )
	{
		plex_dump_dot( stdout, lex );
		plex_free( lex );

		return 0;
	}

	/* Read from stdin */
	if( !start )
	{
		pgetline( &finput, &size, stdin );
		start = finput;
	}

	/* Process */
	while( start && *start )
	{
		if( !( start = plex_next( lex, start, &id, &end ) ) )
			break;

		/* Print match ID only when more than one token was defined */
		if( i > 1 )
			printf( "%d", id );

		printf( "%s%.*s%s", begin_sep, (int)( end - start ), start, end_sep );
		start = end;
	}

	fflush( stdout );

	pfree( finput );
	plex_free( lex );

	return 0;
}

