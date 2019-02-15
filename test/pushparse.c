/* cc -o pushparse -I ../src pushparse.c  ../src/libphorward.a */

#include "phorward.h"

int main()
{
	ppgram*		grm;
	pppar*		par;
	ppsym*		sym;

	plex*		lex;
	parray*		tokens;
	prange*		token;
	char*		s = "1 + 23 * 456 - 789";

	/* Create a lexer */
	lex = plex_create(0);
	plex_define( lex, "\\d+", 1, 0 );
	plex_define( lex, "\\+", 2, 0 );
	plex_define( lex, "-", 3, 0 );
	plex_define( lex, "\\*", 4, 0 );
	plex_define( lex, "/", 5, 0 );
	plex_prepare( lex );

	/* Create a grammar */
	grm = pp_gram_create();
	pp_gram_from_pbnf( grm,
		"Int     : /[0-9]+/ = int;"

		"<<      '+' '-';"
		"<<      '*' '/';"

		"expr$   : expr '*' expr = mul"
				"| expr '/' expr = div"
				"| expr '+' expr = add"
				"| expr '-' expr = sub"
				"| '(' expr ')'"
				"| Int"
				";" );

	par = pp_par_create( grm );

	/*
	Int = pp_sym_get_by_name( grm, "Int" );
	plus = pp_sym_get_by_name( grm, "'+'" );
	minus = pp_sym_get_by_name( grm, "'-'" );
	star = pp_sym_get_by_name( grm, "'*'" );
	slash = pp_sym_get_by_name( grm, "'/'" );
	*/

	/* Tokenize the input */
	plex_tokenize( lex, s, &tokens );

	parray_for( tokens, token )
	{
		sym = pp_sym_get( grm, token->id );

		printf( "%d >%.*s< %s\n",
			token->id, token->end - token->start, token->start,
				sym->name );

		if( pp_par_pushparse( par, sym, token->start, token->end )
				!= PPPAR_STATE_NEXT )
				break;
	}

	if( pp_par_pushparse( par, pp_sym_get( grm, 0 ), (char*)NULL, (char*)NULL )
			== PPPAR_STATE_DONE )
	{
		printf( "Success!\n" );
		pp_ast_dump_short( stdout, par->root );
	}

	return 0;
}

