#include "unicc.h"

int main( int argc, char** argv )
{
	Grammar*	g;
	Parser*		p;
	AST_node*	a;

	PROC( "main" );

	g = gram_create();

	#include "bnftestgen.c"

	GRAMMAR_DUMP( g );
	/* printf( "%s\n", gram_to_bnf( g ) ); */

	gram_prepare( g );

	p = par_create( g );

	if( argc > 1 && par_parse( &a, p, argv[1] ) )
		ast_dump_short( stdout, a );

	RETURN( 0 );
}
