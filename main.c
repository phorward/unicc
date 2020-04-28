/* -MODULE----------------------------------------------------------------------
UniCC² Parser Generator
Copyright (C) 2006-2020 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	main.c
Usage:	Program entry.
----------------------------------------------------------------------------- */

#include "unicc.h"

/* Main */

static void version( char** argv, char* descr )
{
	printf( "UniCC %s\n", UNICC_VERSION );
	printf( "Universal LR/LALR/GLR Parser Generator.\n\n" );
	printf( "Copyright (C) 2006-2020 by Phorward Software Technologies, "
			"Jan Max Meyer\n"
			"All rights reserved. See LICENSE for more information.\n" );
}

static void help( char** argv )
{
	printf( "Usage: %s OPTIONS grammar [input [input ...]]\n\n"

	"   grammar                   Grammar to create a parser from.\n"
	"   input                     Input to be processed by the parser.\n\n"

	"   -G                        Dump constructed grammar\n"
	"                             (as JSON when '-R json' provided)\n"
	"   -h  --help                Show this help, and exit.\n"
	"   -P                        Dump constructed parser\n"
	"                             (as JSON when '-R json' provided)\n"
	"   -r  --run --repl          Run generated parser either from input\n"
	"                             or in REPL mode\n"
	"   -R  --render  RENDERER    Use AST renderer RENDERER:\n"
	"                             full, json, short (default), tree2svg, yaml\n"
	"   -s  --scannerless         Transform grammar into scannerless form\n"
	"   -t  --trim                Trim stdin input before parsing\n"
	"   -v  --verbose             Print processing information.\n"
	"   -V  --version             Show version info and exit.\n"

	"\n", *argv );

}

int main( int argc, char** argv )
{
	pboolean	verbose	= FALSE;
	pboolean	lm		= FALSE;
	pboolean	dg		= FALSE;
	pboolean	dp		= FALSE;
	pboolean	sg		= FALSE;
	pboolean	run		= FALSE;
	pboolean	trim	= FALSE;
	int			r		= 0;
	AST_node*	a		= (AST_node*)NULL;
	Grammar*	g;
	Parser*		p;
	char*		gfile	= "grammar";
	char*		gstr	= (char*)NULL;
	char*		ifile;
	char*		istr	= (char*)NULL;
	char*		s		= (char*)NULL;
	int			i;
	int			rc;
	int			next;
	int			ret		= 0;
	char		opt		[ 20 + 1 ];
	char		line	[ 1024 + 1 ];
	char*		param;

	PROC( "unicc" );

	for( i = 0;
			( rc = pgetopt( opt, &param, &next, argc, argv,
				"GhPrR:stvV",
				"help repl run renderer: scannerless "
					"trim verbose version", i ) )
						== 0; i++ )
	{
		if( !strcmp(opt, "G" ) )
			dg = TRUE;
		else if( !strcmp(opt, "P" ) )
			dp = TRUE;
		else if( !strcmp( opt, "help" ) || !strcmp( opt, "h" ) )
		{
			help( argv );
			RETURN( 0 );
		}
		else if( !strcmp(opt, "r" )
					|| !strcmp( opt, "run" )
						|| !strcmp( opt, "repl") )
			run = TRUE;
		else if( !strcmp( opt, "renderer" ) || !strcmp( opt, "R" ) )
		{
			if( pstrcasecmp( param, "full" ) == 0 )
				r = 1;
			else if( pstrcasecmp( param, "json" ) == 0 )
				r = 2;
			else if( pstrcasecmp( param, "tree2svg" ) == 0 )
				r = 3;
			else if( pstrcasecmp( param, "yaml" ) == 0 )
				r = 4;
		}
		else if( !strcmp(opt, "s" ) || !strcmp( opt, "scannerless" ) )
			sg = TRUE;
		else if( !strcmp(opt, "t" ) || !strcmp( opt, "trim" ) )
			trim = TRUE;
		else if( !strcmp( opt, "verbose" ) || !strcmp( opt, "v" ) )
			verbose = TRUE;
		else if( !strcmp( opt, "version" ) || !strcmp( opt, "V" ) )
		{
			version( argv, "Parser construction command-line utility" );
			RETURN( 0 );
		}
	}

	if( rc == 1 && param )
	{
		if( pfiletostr( &gstr, param ) )
			gfile = param;
		else
			gstr = param;

		next++;
	}

	if( !gstr )
	{
		help( argv );
		RETURN( 1 );
	}

	MSG( "Command-line parameters read" );

	if( verbose )
		printf( "Parsing grammar from '%s'\n", gfile );

	g = gram_create();
	/* g->flags.debug = TRUE; */

	if( !gram_from_bnf( g, gstr ) )
	{
		fprintf( stderr, "%s: Parse error in >%s<\n", gfile, gstr );
		RETURN( 1 );
	}

	MSG( "Grammar parsed" );

	if( !gram_prepare( g ) )
	{
		fprintf( stderr, "%s: Unable to prepare grammar\n", gfile );
		RETURN( 1 );
	}

	MSG( "Grammar prepared" );

	/*
	{
		plistel*	e;
		Symbol*		s;

		plist_for(g->symbols, e)
		{
			s = plist_access(e);
			printf(">%s<\n", s->name);
		}
	}
	*/

	if( sg )
		gram_transform_to_scannerless( g );

	if( dg )
	{
		if( r == 2 ) /* json */
			gram_dump_json( stdout, g, "" );
		else
		{
			GRAMMAR_DUMP( g );
			printf( "%s\n", gram_to_bnf( g ) );
		}
	}

	p = par_create( g );

	MSG( "Parser created" );

	if( dp )
	{
		if( r == 2 )  /* json */
			par_dump_json( stdout, p );
		else
		{
			fprintf( stderr, "Parser can only be dumped in JSON for now\n" );
			par_dump_json( stderr, p );
		}
	}

	lm = argc == next;
	i = 0;

	if( next < argc )
		run = TRUE;

	while( run )
	{
		ifile = (char*)NULL;

		if( lm )
		{
			fgets( line, sizeof( line ) - 1, stdin );
			if( trim )
				s = pstrtrim( line );
			else
				s = line;

			if( !*s )
				break;
		}
		else
		{
			if( next == argc )
				break;

			if( pfiletostr( &istr, argv[ next ] ) )
			{
				ifile = argv[ next ];
				s = istr;
			}
			else
				s = argv[ next ];

			next++;
		}

		if( !ifile )
		{
			sprintf( opt, "input.%d", i );
			ifile = opt;
		}

		if( s )
		{
			MSG( "Parsing" );

			if( par_parse( &a, p, s ) )
			{
				switch( r )
				{
					case 0:
						ast_dump_short( stdout, a );
						break;

					case 1:
						ast_dump( stdout, a );
						break;

					case 2:
						ast_dump_json( stdout, a );
						printf( "\n" );
						break;

					case 3:
						ast_dump_tree2svg( stdout, a );
						printf( "\n" );
						break;

					case 4:
						ast_dump_yaml( stdout, a, 0 );
						break;

					/*
					case 5:
						{
							pvm*		vm;
							pvmprog*	prg;

							vm = pvm_create();
							prg = pvm_prog_create( vm, (char*)NULL );
							ast_dump_pvm( prg, a );

							pvm_prog_dump( prg );
							pvm_prog_run( (pany**)NULL, prg );

							pvm_prog_free( prg );
							pvm_free( vm );
						}
						break;
					*/

					default:
						MISSINGCASE;
						break;
				}
			}
			else
			{
				fprintf( stderr, "%s: Parse error\n", ifile );
				ret = 1;
			}

			a = ast_free( a );
		}

		istr = pfree( istr );
		i++;
	}

	RETURN( ret );
}

