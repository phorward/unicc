/* -MODULE----------------------------------------------------------------------
UniCC² Parser Generator
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	gram2c.c
Usage:	Grammar to C-code generator
----------------------------------------------------------------------------- */

/*
	This program reads a UniCC grammar definition using the UniCC grammar
	abstraction layer into a Grammar object and generates the C code that
	is necessary to generate exactly the same, parsed grammar as it was
	written by hand using the grammar abstraction layer functions.
*/

#include "unicc.h"

char*	gname		= "g";
char	indent		[ BUFSIZ ];

plist	ident2sym;

void help( char** argv )
{
	printf( "Usage: %s OPTIONS grammar\n\n"

	"   -a  --all                 Generate all parts.\n"
	"   -d  --debug               Print parsed grammar.\n"
	"   -D                        Generate symbol variable declarations\n"
	"   -h  --help                Show this help, and exit.\n"
	"   -i  --indent NUM          Indent generated code by NUM tabs\n"
	"   -o  --output FILE         Output to FILE; Default is stdout.\n"
	"   -P                        Generate productions\n"
	"   -S                        Generate symbols\n"
	"   -T                        Generate an AST traversal function stub\n"
	"   -V  --version             Show version info and exit.\n",

	*argv );
}

void gen_decl( FILE* f, Grammar* g )
{
	int			i;
	int			cnt;
	Symbol*		sym;
	char		var		[ BUFSIZ ];
	char*		ptr;
	plistel*	e;
	plistel*	x;

	plist_init( &ident2sym, 0, PLIST_DFT_HASHSIZE, PLIST_MOD_KEEPKEYS );

	for( i = 0; ( sym = sym_get( g, i ) ); i++ )
	{
		if( sym->flags.special )
			continue;

		if( SYM_IS_TERMINAL( sym ) )
			strcpy( var, "t_" );
		else
			strcpy( var, "n_" );

		for( ptr = sym->name; ptr && *ptr; ptr++ )
			if( isalpha( *ptr ) || *ptr == '_' )
				sprintf( var + strlen( var ), "%c", *ptr );

		if( strlen( var ) == 2 )
		{
			if( SYM_IS_TERMINAL( sym ) )
				strcpy( var, "_t_noname" );
			else
				strcpy( var, "_n_noname" );
		}

		e = plist_insert( &ident2sym, NULL, var, sym );
	}

	if( !f )
		return;

	for( i = 0; ( x = e = plist_getkey( &ident2sym, i ) ); i++ )
	{
		cnt = 1;

		while( ( x = plist_hashnext( x ) ) )
		{
			if( strcmp( x->hashprev->key, x->key ) )
				break;

			cnt++;
		}

		if( cnt > 1 )
			fprintf( f, "%sSymbol*	%s[ %d ];\n", indent, plist_key( e ), cnt );
		else
			fprintf( f, "%sSymbol*	%s;\n", indent, plist_key( e ) );
	}

	fprintf( f, "\n" );
}

char* get_symbol_var( Symbol* sym )
{
	int				cnt		= -1;
	plistel*		e;
	plistel*		x;
	static char		var		[ BUFSIZ ];

	for( e = plist_first( &ident2sym ); ( x = e ); e = plist_next( e ) )
		if( plist_access( e ) == sym )
		{
			if( ( plist_hashprev( e )
					&& !strcmp( plist_hashprev( e )->key, e->key ) )
				|| ( plist_hashnext( e )
						&& !strcmp( plist_hashnext( e )->key, e->key ) ) )
			{
				for( cnt = 0; ( x = plist_hashprev( x ) ); cnt++ )
				{
					if( strcmp( x->hashnext->key, x->key ) )
					{
						cnt = -1;
						break;
					}
				}
			}

			if( cnt < 0 )
				strcpy( var, plist_key( e ) );
			else
				sprintf( var, "%s[ %d ]", plist_key( e ), cnt );

			return var;
		}

	return (char*)NULL;
}

void gen_sym( FILE* f, Grammar* g )
{
	Symbol*		sym;
	char*		def;
	char*		name;
	plistel*	e;
	plistel*	x;
	char*		var;

	fprintf( f, "%s/* Symbols */\n\n", indent );

	for( e = plist_first( &ident2sym ); ( x = e ); x = e = plist_next( e ) )
	{
		sym = (Symbol*)plist_access( e );
		var = get_symbol_var( sym );

		if( ( name = sym->name ) )
			name = pregex_qreplace( "([\\\\\"])", sym->name, "\\$1", 0 );

		fprintf( f,
			"%s%s = sym_create( %s, %s%s%s );\n",
			indent,
			var,
			gname,

			name ? "\"" : "",
			name ? name : "(char*)NULL",
			name ? "\"" : "" );

		if( sym->ccl )
			def = pccl_to_str( sym->ccl, TRUE );
		else if( sym->str )
			def = sym->str;
		else if( sym->ptn )
			def = pregex_ptn_to_regex( sym->ptn );
		else
			def = (char*)NULL;

		if( def )
		{
			def = pregex_qreplace( "([\\\\\"])", def, "\\$1", 0 );

			if( sym->ccl )
				fprintf( f,
					"%s%s->ccl = pccl_create( -1, -1, \"%s\" );\n",
						indent, var, def );
			else if( sym->str )
				fprintf( f,
					"%s%s->str = \"%s\";\n",
						indent, var, def );
			else if( sym->ptn )
				fprintf( f,
					"%s%s->ptn = pregex_ptn_create( \"%s\", 0 );\n",
						indent, var, def );

			pfree( def );
		}

		pfree( name );

		/* Has emit info? */
		if( sym->emit )
		{
			if( sym->emit == sym->name )
				fprintf( f, "%s%s->emit = %s->name;\n",
								indent, var, var );
			else
				fprintf( f, "%s%s->emit = pstrdup( \"%s\" );\n",
								indent, var, sym->emit );
		}

		/* Set flags? */
		if( sym->flags.lexem )
			fprintf( f, "%s%s->flags.lexem = TRUE;\n",
				indent, var );
		else if( sym->flags.whitespace )
			fprintf( f, "%s%s->flags.whitespace = TRUE;\n",
				indent, var );

		/* Is this the goal? */
		if( g->goal == sym )
			fprintf( f, "%s%s->goal = %s;\n",
				indent, gname, var );

		fprintf( f, "\n" );
	}
}

void gen_prods( FILE* f, Grammar* g )
{
	int			i;
	int			j;
	Production*	prod;
	Symbol*		sym;
	Symbol*		lhs		= (Symbol*)NULL;
	char*		name;
	char*		def;
	char*		s;
	char		pat		[ 10 + 1 ];

	fprintf( f, "%s/* Productions */\n\n", indent );

	for( i = 0; ( prod = prod_get( g, i ) ); i++ )
	{
		if( lhs && lhs != prod->lhs )
			fprintf( f, "\n" );

		lhs = prod->lhs;
		fprintf( f, "%sprod_create( %s, %s /* %s */,",
						indent, gname, get_symbol_var( prod->lhs ),
							prod->lhs->name );

		for( j = 0; ( sym = prod_getfromrhs( prod, j ) ); j++ )
		{
			if( sym->ccl )
			{
				def = pccl_to_str( sym->ccl, TRUE );
				s = "\'";
			}
			else if( sym->str )
			{
				def = sym->str;
				s = "\"";
			}
			else if( sym->ptn )
			{
				def = pregex_ptn_to_regex( sym->ptn );
				s = "/";
			}
			else
			{
				name = sym->name;
				def = (char*)NULL;
				s = "";
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

			fprintf( f, "\n%s\t%s, /* %s */",
				indent, get_symbol_var( sym ), name );

			pfree( def );
		}

		fprintf( f, "\n%s\t", indent );
		fprintf( f, "(Symbol*)NULL\n%s)", indent );

		/* Has emit info */
		if( prod->emit && prod->emit != prod->lhs->emit )
			if( prod->emit == prod->lhs->name )
				fprintf( f, "->emit = %s->name;\n\n",
							get_symbol_var( prod->lhs ) );
			else
				fprintf( f, "->emit = \"%s\";\n\n", prod->emit );
		else
			fprintf( f, ";\n" );
	}

	fprintf( f, "\n" );
}

void gen_traversal( FILE* f, Grammar* g )
{
	plist		emits;
	int			i;
	pboolean	first	= TRUE;
	Symbol*		sym;
	Production*	prod;

	plist_init( &emits, 0, PLIST_DFT_HASHSIZE, PLIST_MOD_UNIQUE );

	fprintf( f, "%sstatic void traverse( AST_node* node )\n", indent );
	fprintf( f, "%s{\n", indent );

	fprintf( f, "%s\twhile( node )\n", indent );
	fprintf( f, "%s\t{\n", indent );
	fprintf( f, "%s\t\tif( node->child )\n", indent );
	fprintf( f, "%s\t\t\ttraverse( node->child );\n\n", indent );

	for( i = 0; ( sym = sym_get( g, i ) ); i++ )
	{
		if( !sym->emit || !plist_insert( &emits, NULL, sym->emit, NULL ) )
			continue;

		fprintf( f, "%s\t\t%sif( !strcmp( node->emit, \"%s\" ) )\n",
			indent, !first ? "else " : "", sym->emit );

		fprintf( f, "%s\t\t{\n", indent );

		if( SYM_IS_TERMINAL( sym ) )
			fprintf( f,
				"%s\t\t\tprintf( \"%s >%%.*s<\\n\", "
					"(int)node->len, node->start );\n",
					indent, sym->emit );
		else
			fprintf( f,
				"%s\t\t\tprintf( \"%s\\n\" );\n",
					indent, sym->emit );

		fprintf( f, "%s\t\t}\n", indent );

		first = FALSE;
	}

	for( i = 0; ( prod = prod_get( g, i ) ); i++ )
	{
		if( !prod->emit || !plist_insert( &emits, NULL, prod->emit, NULL ) )
			continue;

		fprintf( f, "%s\t\t%sif( !strcmp( node->emit, \"%s\" ) )\n",
			indent, !first ? "else " : "", prod->emit );

		fprintf( f, "%s\t\t{\n", indent );

		fprintf( f,
			"%s\t\t\tprintf( \"%s\\n\" );\n",
				indent, prod->emit );

		fprintf( f, "%s\t\t}\n", indent );

		first = FALSE;
	}

	fprintf( f, "\n%s\t\tnode = node->next;\n", indent );
	fprintf( f, "%s\t}\n", indent );
	fprintf( f, "%s}\n", indent );

	plist_erase( &emits );
}


int main( int argc, char** argv )
{
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
	pboolean	gdecl	= FALSE;
	pboolean	gsym	= FALSE;
	pboolean	gprod	= FALSE;
	pboolean	gast	= FALSE;

	PROC( "gram2c" );

	for( i = 0; ( rc = pgetopt( opt, &param, &next, argc, argv,
						"aDdhi:o:PSTV",
						"all debug help indent: output: version:", i ) )
							== 0; i++ )
	{
		if( !strcmp( opt, "all" ) || !strcmp( opt, "a" ) )
			gdecl = gsym = gprod = gast = TRUE;
		else if( !strcmp( opt, "debug" ) || !strcmp( opt, "d" ) )
			debug = TRUE;
		else if( !strcmp( opt, "D" ) )
			gdecl = TRUE;
		else if( !strcmp( opt, "help" ) || !strcmp( opt, "h" ) )
		{
			help( argv );
			return 0;
		}
		else if( !strcmp( opt, "output" ) || !strcmp( opt, "o" ) )
			out = param;
		else if( !strcmp( opt, "P" ) )
			gprod = TRUE;
		else if( !strcmp( opt, "S" ) )
			gsym = TRUE;
		else if( !strcmp( opt, "T" ) )
			gast = TRUE;
		else if( !strcmp( opt, "indent" ) || !strcmp( opt, "i" ) )
		{
			if( ( j = atoi( param ) ) > 0 )
			{
				if( j >= BUFSIZ )
					j = BUFSIZ - 1;

				while( j-- )
					indent[ j ] = '\t';
			}
		}
		else if( !strcmp( opt, "version" ) || !strcmp( opt, "V" ) )
		{
			/* version( argv, "Converts phorward grammars into C code" ); */
			RETURN( 0 );
		}
	}

	if( rc == 1 )
		in = param;

	if( !in )
	{
		help( argv );
		RETURN( 1 );
	}

	/* Read grammar from input */
	if( !pfiletostr( &gram, in ) )
	{
		fprintf( stderr, "Can't read from '%s'\n", in );
		RETURN( 1 );
	}

	/* Generate & parse grammar */
	if( !( ( g = gram_create() ) && gram_from_bnf( g, gram ) ) )
	{
		fprintf( stderr, "Parse error\n" );
		return 1;
	}

	if( debug )
		GRAMMAR_DUMP( g );

	gram_prepare( g );

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

	/* If nothing shall be printed, then enforce all! */
	if( !gdecl && !gsym && !gprod && !gast )
		gdecl = gsym = gprod = gast = TRUE;

	/* Generate C code */
	if( !gdecl )
		gen_decl( NULL, g ); /* Must be called to build the variable names! */
	else
		gen_decl( f, g );

	/*
	get_symbol_var( sym_get_by_name( g, "sequence" ) );
	return 0;
	*/

	if( gsym )
		gen_sym( f, g );

	if( gprod )
		gen_prods( f, g );

	if( gast )
		gen_traversal( f, g );

	/* Clear mem */
	gram_free( g );
	plist_erase( &ident2sym );

	RETURN( 0 );
}
