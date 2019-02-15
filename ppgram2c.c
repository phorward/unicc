/* -MODULE----------------------------------------------------------------------
UniCC² Parser Generator
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	Grammar2c.c
Usage:	Grammar to C-code generator
----------------------------------------------------------------------------- */

#include "unicc.h"

char*	emptystr	= "";
char*	gname		= "g";
char*	symname		= "sym";
char*	prodname	= "prod";
char*	indent;

void help( char** argv )
{
	printf( "Usage: %s OPTIONS grammar\n\n"

	"   -d  --debug               Print parsed grammar.\n"
	"   -h  --help                Show this help, and exit.\n"
	"   -i  --indent NUM          Indent generated code by NUM tabs\n"
	"   -o  --output FILE         Output to FILE; Default is stdout.\n"
	"   -V  --version             Show version info and exit.\n",

	*argv );
}

char* cident( char* ident )
{
	return ident;
}

void gen_decl( FILE* f, Grammar* g )
{
	fprintf( f, "%sSymbol*	%s[ %d ];\n",
		indent, symname, plist_count( g->symbols ) );
	fprintf( f, "%sProduction*	%s[ %d ];\n",
		indent, prodname, plist_count( g->prods ) );
}

void gen_sym( FILE* f, Grammar* g )
{
	int		i;
	Symbol*	sym;
	char*	def;
	char*	name;
	char*	symtypes[]	= { "PPSYMTYPE_NONTERM",
							"PPSYMTYPE_CCL",
							"PPSYMTYPE_STRING",
							"PPSYMTYPE_REGEX",
							"PPSYMTYPE_SPECIAL" };

	fprintf( f, "%s/* Symbols */\n\n", indent );

	for( i = 0; sym = pp_sym_get( g, i ); i++ )
	{
		switch( sym->type )
		{
			case PPSYMTYPE_SPECIAL:
				/* Ignore special symbols */
				continue;

			case PPSYMTYPE_CCL:
				def = pccl_to_str( sym->ccl, TRUE );
				break;

			case PPSYMTYPE_STRING:
				def = sym->str;
				break;

			case PPSYMTYPE_REGEX:
				def = pregex_ptn_to_regex( sym->re->ptn );
				break;

			default:
				def = (char*)NULL;
				break;
		}

		if( ( name = sym->name ) )
			name = pregex_qreplace( "([\\\\\"])", sym->name, "\\$1", 0 );

		if( def )
			def = pregex_qreplace( "([\\\\\"])", def, "\\$1", 0 );

		fprintf( f,
			"%s%s[ %d ] = pp_sym_create( %s, %s, %s%s%s, %s%s%s );\n",
			indent,
			symname, sym->id,
			gname, symtypes[ sym->type ],

			name && !( sym->flags & FLAG_NAMELESS ) ? "\"" : "",
			name && !( sym->flags & FLAG_NAMELESS ) ? name : "(char*)NULL",
			name && !( sym->flags & FLAG_NAMELESS ) ? "\"" : "",

			def ? "\"" : "",
			def ? def : "(char*)NULL",
			def ? "\"" : "" );

		pfree( name );
		pfree( def );

		/* Has emit info? */
		if( sym->emit )
			if( sym->emit == sym->name )
				fprintf( f, "%s%s[ %d ]->emit = %s[ %d ]->name;\n",
								indent, symname, sym->id, symname, sym->id );
			else
				fprintf( f, "%s%s[ %d ]->emit = pstrdup( \"%s\" );\n",
								indent, symname, sym->id, sym->emit );

		/* Set flags? */
		if( sym->flags & FLAG_LEXEM )
			fprintf( f, "%s%s[ %d ]->flags |= FLAG_LEXEM;\n",
				indent, symname, sym->id );
		else if( sym->flags & FLAG_WHITESPACE )
			fprintf( f, "%s%s[ %d ]->flags |= FLAG_WHITESPACE;\n",
				indent, symname, sym->id );

		/* Is this the goal? */
		if( g->goal == sym )
			fprintf( f, "%s%s->goal = %s[ %d ];\n",
				indent, gname, symname, sym->id );

		fprintf( f, "\n" );
	}

	fprintf( f, "\n" );
}

void gen_prods( FILE* f, Grammar* g )
{
	int		i;
	int		j;
	Production*	prod;
	Symbol*	sym;
	char*	name;
	char*	def;
	char*	s;
	char	pat		[ 10 + 1 ];

	fprintf( f, "%s/* Productions */\n\n", indent );

	for( i = 0; prod = pp_prod_get( g, i ); i++ )
	{
		fprintf( f, "%s%s[ %d ] = pp_prod_create( %s, %s[ %d ] /* %s */,",
							indent, prodname, i, gname, symname, prod->lhs->id,
								prod->lhs->name );

		for( j = 0; sym = pp_prod_getfromrhs( prod, j ); j++ )
		{
			switch( sym->type )
			{
				case PPSYMTYPE_CCL:
					def = pccl_to_str( sym->ccl, TRUE );
					s = "\'";
					break;

				case PPSYMTYPE_STRING:
					def = sym->str;
					s = "\"";
					break;

				case PPSYMTYPE_REGEX:
					def = pregex_ptn_to_regex( sym->re->ptn );
					s = "/";
					break;

				default:
					name = sym->name;
					def = (char*)NULL;
					s = "";
					break;
			}

			if( *s && def )
			{
				sprintf( pat, "([\\\\%s])", s );
				def = pregex_qreplace( pat, def, "\\$1", 0 );

				name = pmalloc( ( strlen( def ) + 2 * strlen( s ) + 1 )
									* sizeof( char ) );
				sprintf( name, "%s%s%s", s, def, s );

				pfree( def );
				def = name;
			}

			if( strstr( name, "*/" ) )
			{
				name = pstrreplace( name, "*/", "*\\/" );
				pfree( def );
				def = name;
			}

			fprintf( f, "\n%s\t%s[ %d ], /* %s */",
				indent, symname, sym->id, name );

			pfree( def );
		}

		fprintf( f, "\n%s\t", indent );
		fprintf( f, "(Symbol*)NULL );\n" );

		/* Has emit info */
		if( prod->emit && prod->emit != prod->lhs->emit )
			if( prod->emit == prod->lhs->name )
				fprintf( f, "%s%s[ %d ]->emit = %s[ %d ]->name;\n",
								indent, prodname, i, symname, prod->lhs->id );
			else
				fprintf( f, "%s%s[ %d ]->emit = \"%s\";\n",
								indent, prodname, i, prod->emit );

		fprintf( f, "\n" );
	}

	fprintf( f, "\n" );
}

int main( int argc, char** argv )
{
	char*		e;
	char*		s;
	Grammar*	g;
	int			i;
	int			j;
	int			rc;
	int			next;
	char		opt		[ 20 + 1 ];
	char*		param;
	char*		in		= (char*)NULL;
	char*		gram;
	char*		out		= (char*)NULL;
	FILE*		f		= stdout;
	pboolean	debug	= FALSE;

	for( i = 0; ( rc = pgetopt( opt, &param, &next, argc, argv,
						"dhi:o:V",
						"debug help indent: output: version:", i ) )
							== 0; i++ )
	{
		if( !strcmp( opt, "debug" ) || !strcmp( opt, "d" ) )
			debug = TRUE;
		else if( !strcmp( opt, "help" ) || !strcmp( opt, "h" ) )
		{
			help( argv );
			return 0;
		}
		else if( !strcmp( opt, "output" ) || !strcmp( opt, "o" ) )
			out = param;
		else if( !strcmp( opt, "indent" ) || !strcmp( opt, "i" ) )
		{
			pfree( indent );

			if( ( j = atoi( param ) ) > 0 )
			{
				indent = pmalloc( ( j + 1 ) * sizeof( char ) );

				while( j-- )
					indent[ j ] = '\t';
			}
		}
		else if( !strcmp( opt, "version" ) || !strcmp( opt, "V" ) )
		{
			version( argv, "Converts phorward grammars into C code" );
			return 0;
		}
	}

	if( !indent )
		indent = emptystr;

	if( rc == 1 )
		in = param;

	if( !in )
	{
		help( argv );
		return 1;
	}

	/* Read grammar from input */
	if( !pfiletostr( &gram, in ) )
	{
		fprintf( stderr, "Can't read from '%s'\n", in );
		return 1;
	}

	/* Generate & parse grammar */
	if( !( ( g = pp_gram_create() )
				&& pp_gram_from_bnf( g, gram ) ) )
	{
		fprintf( stderr, "Parse error\n" );
		return 1;
	}

	if( debug )
		pp_gram_dump( stderr, g );

	pp_gram_prepare( g );

	pfree( gram );

	/* Open output file */
	if( out )
	{
		if( !( f = fopen( out, "rb" ) ) )
		{
			fprintf( stderr, "Can't open file '%s' for write\n", out );
			return 1;
		}
	}

	/* Generate C code */
	gen_decl( f, g );
	gen_sym( f, g );
	gen_prods( f, g );

	/* Clear mem */
	pp_gram_free( g );

	if( indent != emptystr )
		pfree( indent );

	return 0;
}
