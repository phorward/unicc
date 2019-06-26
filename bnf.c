/* -MODULE----------------------------------------------------------------------
UniCC² Parser Generator
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	pbnf.c
Usage:	Parsing full parser definitions from Phorward BNF (PBNF).
----------------------------------------------------------------------------- */

#include "unicc.h"

/* Derive name from basename */
static char* derive_name( Grammar* gram, char* base )
{
	int             i;
	static char		deriv   [ ( NAMELEN * 2 ) + 1 ];

	sprintf( deriv, "%s%c", base, DERIVCHAR );

	for( i = 0; strlen( deriv ) < ( NAMELEN * 2 ); i++ )
	{
		if( !sym_get_by_name( gram, deriv ) )
			return pstrdup( deriv );

		sprintf( deriv + strlen( deriv ), "%c", DERIVCHAR );
	}

	return (char*)NULL;
}

/* Traverse a grammar from a parsed AST */
static void traverse( Grammar* g, AST_node* ast )
{
	AST_node*	node;
	char		buf		[ BUFSIZ + 1 ];
	static int	prec	= 0;

	PROC( "traverse" );

	while( ( node = ast ) )
	{
		if( ast->child )
			traverse( g, ast->child );

		VARS( "emit", "%s", ast->emit );

		if( node->len )
		{
			sprintf( buf, "%.*s", (int)node->len, node->start );
			VARS( "buf", "%s", buf );
		}
		else
			*buf = '\0';

		if( !strcmp( node->emit, "Identifier" ) )
			node->ptr = strdup( buf );
		else if( !strcmp( node->emit, "emits" ) )
			node->ptr = node->child->ptr;
		else if( !strcmp( node->emit, "variable" ) )
		{
			if( !( node->ptr = (void*)sym_get_by_name(
						g, node->child->ptr ) ) )
			{
				Symbol* sym;

				sym = sym_create( g, node->child->ptr );
				sym->flags.freename = TRUE;

				node->ptr = (void*)sym;
			}
			else
				pfree( node->child->ptr );
		}
		else if( !strcmp( node->emit, "CCL" )
					|| !strcmp( node->emit, "String" )
						|| !strcmp( node->emit, "Token" )
							|| !strcmp( node->emit, "Regex" ) )
		{
			if( !( node->ptr = (void*)sym_get_by_name( g, buf ) ) )
			{
				Symbol* sym;

				sym = sym_create( g, pstrdup( buf ) );
				sym->flags.freeemit = TRUE;
				sym->flags.nameless = TRUE;

				if( !strcmp( node->emit, "CCL" )
					|| !strcmp( node->emit, "String" )
						|| !strcmp( node->emit, "Token" )
							|| !strcmp( node->emit, "Regex" ) )
				{
					memmove( buf, buf + 1, node->len - 1 );
					buf[ node->len - 2 ] = '\0';

					if( !strcmp( node->emit, "CCL" ) )
						sym->ccl = pccl_create( -1, -1, buf );
					else if( !strcmp( node->emit, "String" )
								|| !strcmp( node->emit, "Token" ) )
						sym->str = strdup( buf );
					else if( !strcmp( node->emit, "Regex" ) )
						sym->ptn = pregex_ptn_create(
										buf, PREGEX_COMP_NOERRORS );

					if( !strcmp( node->emit, "Token" ) )
						sym->emit = sym->str;
				}

				node->ptr = (void*)sym;
			}
		}
		else if( !strcmp( node->emit, "kle" ) )
			node->ptr = (void*)sym_mod_kleene( node->child->ptr );
		else if( !strcmp( node->emit, "pos" ) )
			node->ptr = (void*)sym_mod_positive( node->child->ptr );
		else if( !strcmp( node->emit, "opt" ) )
			node->ptr = (void*)sym_mod_optional( node->child->ptr );
		else if( !strcmp( node->emit, "definition" )
					|| !strcmp( node->emit, "inline" ) )
		{
			Symbol*		sym;
			Production*	prod;
			AST_node*	pnode;

			if( !strcmp( node->emit, "inline" ) )
			{
				for( pnode = node; pnode; pnode = pnode->parent )
				{
					VARS( "pnode->emit", "%s", pnode->emit );

					if( !strcmp( pnode->emit, "definition" ) )
					{
						sym = (Symbol*)pnode->child->ptr;
						break;
					}
				}

				VARS( "sym->name", "%s", sym->name );

				node->ptr = sym = sym_create( g,
									derive_name( g, sym->name ) );

				VARS( "sym->name", "%s", sym->name );

				sym->flags.freename = TRUE;
				sym->flags.nameless = TRUE;
				sym->flags.defined = TRUE;
				sym->flags.generated = TRUE;

				node = node->child;
			}
			else
			{
				sym = (Symbol*)node->child->ptr;
				VARS( "sym->name", "%s", sym->name );

				node = node->child->next;

				if( node && !strcmp( node->emit, "goal" ) )
				{
					g->goal = sym;
					node = node->next;
				}

				if( node && !strcmp( node->emit, "emitsdef" ) )
				{
					sym->emit = sym->name;
					node = node->next;
				}
			}

			while( node )
			{
				prod = prod_create( g, sym, NULL );

				for( pnode = node->child; pnode; pnode = pnode->next )
				{
					if( !strcmp( pnode->emit, "emits" ) )
					{
						prod->emit = pnode->ptr;
						prod->flags.freeemit = TRUE;
						break;
					}

					prod_append( prod, pnode->ptr );
				}

				node = node->next;
			}
		}
		else if( !strcmp( node->emit, "ignore" )
					|| !strcmp( node->emit, "assoc_left" )
						|| !strcmp( node->emit, "assoc_right" )
							|| !strcmp( node->emit, "assoc_none" ) )
		{
			Symbol*		sym;
			AST_node*	snode;

			if( !strncmp( node->emit, "assoc_", 6 ) )
				prec++;

			for( snode = node->child; snode; snode = snode->next )
			{
				sym = (Symbol*)snode->ptr;

				if( !strcmp( node->emit, "assoc_left" ) )
				{
					sym->assoc = ASSOC_LEFT;
					sym->prec = prec;
				}
				else if( !strcmp( node->emit, "assoc_right" ) )
				{
					sym->assoc = ASSOC_RIGHT;
					sym->prec = prec;
				}
				else if( !strcmp( node->emit, "assoc_none" ) )
				{
					sym->assoc = ASSOC_NONE;
					sym->prec = prec;
				}
				else if( !strcmp( node->emit, "ignore" ) )
					sym->flags.whitespace = TRUE;
			}
		}

		ast = ast->next;
	}

	VOIDRET;
}


/** Compiles a UniCC Backus-Naur form definition into a parser.

//g// is the grammar that receives the resulting grammar parsed from the
definition. This grammar may already contain symbols and productions, so it
will be extended during the parse.

**Grammar:**
```
// Whitespace ------------------------------------------------------------------

%skip			/[ \t\r\n]+/
%skip			/\/\/[^\n]*\n/
%skip			/\/\*([^*]|\*[^\/])*\*\/|\/\/[^\n]*\n|#[^\n]*\n/

// Terminals -------------------------------------------------------------------

@Identifier		:= /[A-Za-z_][A-Za-z0-9_]*\

@CCL 			:= /\[(\\.|[^\\\]])*\]/
@String 		:= /'[^']*'/
@Token			:= /"[^"]*"/
@Regex 			:= /\/(\\.|[^\\\/])*\//

#@Code			:= /{{([^}]|}[^}])*}}/

#@Int			:= /[0-9]+/
#@Function		:= /[A-Za-z_][A-Za-z0-9_]*\(\)/

@goal			:= '$'

// Nonterminals ----------------------------------------------------------------

@colon          : ':=' = emitsdef
                | ':'

@emits          := '=' Identifier

@terminal		: CCL | String | Token | Regex

@inline			:= '(' alternation ')'

@variable		:= Identifier

@symbol 		: variable
				| terminal
				| inline

@modifier		: symbol '*' = kle
				| symbol '+' = pos
				| symbol '?' = opt
				| symbol

@sequence		: sequence modifier
				| modifier

@rule		 	:= sequence? emits?

@alternation	: rule ('|' rule)+
				| rule

@definition		: '@' variable goal? colon alternation	= definition
				| /%(ignore|skip)/ ( terminal | variable )+	= ignore

@assocdef		: '<' terminal+	= assoc_left
				| '>' terminal+	= assoc_right
				| '^' terminal+	= assoc_none

@grammar	$	: (definition | assocdef)+
```
*/
pboolean gram_from_bnf( Grammar* g, char* src )
{
	Grammar*	pbnf;
	AST_node*	root;
	Parser*		p;

/*GENERATE ./gram2c -Di 1 grammars/pbnf.bnf */
	Symbol*	n_opt_sequence;
	Symbol*	n_inline;
	Symbol*	n_sequence;
	Symbol*	n_assocdef;
	Symbol*	t_Regex;
	Symbol*	n_alternation[ 2 ];
	Symbol*	t_nn;
	Symbol*	n_variable;
	Symbol*	n_definition[ 2 ];
	Symbol*	t_Identifier;
	Symbol*	n_rule;
	Symbol*	n_modifier;
	Symbol*	n_pos_terminal;
	Symbol*	n_grammar[ 2 ];
	Symbol*	n_pos_grammar;
	Symbol*	n_colon;
	Symbol*	t_CCL;
	Symbol*	t_ignoreskip;
	Symbol*	t_trn;
	Symbol*	t_goal;
	Symbol*	n_opt_emits;
	Symbol*	_t_noname[ 13 ];
	Symbol*	n_terminal;
	Symbol*	n_symbol;
	Symbol*	t_Token;
	Symbol*	n_emits;
	Symbol*	n_pos_alternation;
	Symbol*	n_pos_definition;
	Symbol*	n_opt_goal;
	Symbol*	t_String;
	Symbol*	t_nnnn;

/*ETARENEG*/

	PROC( "gram_from_bnf" );
	PARMS( "g", "%p", g );
	PARMS( "src", "%s", src );

	pbnf = gram_create();

/*GENERATE ./gram2c -g pbnf -SPi 1 grammars/pbnf.bnf */
	/* Symbols */

	t_goal = sym_create( pbnf, "goal" );
	t_goal->str = "$";
	t_goal->emit = t_goal->name;

	_t_noname[ 0 ] = sym_create( pbnf, "':='" );
	_t_noname[ 0 ]->str = ":=";

	_t_noname[ 1 ] = sym_create( pbnf, "':'" );
	_t_noname[ 1 ]->str = ":";

	_t_noname[ 2 ] = sym_create( pbnf, "'='" );
	_t_noname[ 2 ]->str = "=";

	_t_noname[ 3 ] = sym_create( pbnf, "'('" );
	_t_noname[ 3 ]->str = "(";

	_t_noname[ 4 ] = sym_create( pbnf, "')'" );
	_t_noname[ 4 ]->str = ")";

	_t_noname[ 5 ] = sym_create( pbnf, "'*'" );
	_t_noname[ 5 ]->str = "*";

	_t_noname[ 6 ] = sym_create( pbnf, "'+'" );
	_t_noname[ 6 ]->str = "+";

	_t_noname[ 7 ] = sym_create( pbnf, "'?'" );
	_t_noname[ 7 ]->str = "?";

	_t_noname[ 8 ] = sym_create( pbnf, "'|'" );
	_t_noname[ 8 ]->str = "|";

	_t_noname[ 9 ] = sym_create( pbnf, "'@'" );
	_t_noname[ 9 ]->str = "@";

	_t_noname[ 10 ] = sym_create( pbnf, "'<'" );
	_t_noname[ 10 ]->str = "<";

	_t_noname[ 11 ] = sym_create( pbnf, "'>'" );
	_t_noname[ 11 ]->str = ">";

	_t_noname[ 12 ] = sym_create( pbnf, "'^'" );
	_t_noname[ 12 ]->str = "^";

	t_trn = sym_create( pbnf, "/[ \\t\\r\\n]+/" );
	t_trn->ptn = pregex_ptn_create( "[\\t\\n\\r ]+", 0 );
	t_trn->flags.whitespace = TRUE;

	t_nn = sym_create( pbnf, "/\\/\\/[^\\n]*\\n/" );
	t_nn->ptn = pregex_ptn_create( "//[^\\n]*\\n", 0 );
	t_nn->flags.whitespace = TRUE;

	t_nnnn = sym_create( pbnf, "/\\/\\*([^*]|\\*[^\\/])*\\*\\/|\\/\\/[^\\n]*\\n|#[^\\n]*\\n/" );
	t_nnnn->ptn = pregex_ptn_create( "/\\*([^*]|\\*[^/])*\\*/|//[^\\n]*\\n|#[^\\n]*\\n", 0 );
	t_nnnn->flags.whitespace = TRUE;

	t_Identifier = sym_create( pbnf, "Identifier" );
	t_Identifier->ptn = pregex_ptn_create( "[A-Z_a-z][0-9A-Z_a-z]*", 0 );
	t_Identifier->emit = t_Identifier->name;

	t_CCL = sym_create( pbnf, "CCL" );
	t_CCL->ptn = pregex_ptn_create( "\\[(\\\\.|[^\\\\\\]])*\\]", 0 );
	t_CCL->emit = t_CCL->name;

	t_String = sym_create( pbnf, "String" );
	t_String->ptn = pregex_ptn_create( "'[^']*'", 0 );
	t_String->emit = t_String->name;

	t_Token = sym_create( pbnf, "Token" );
	t_Token->ptn = pregex_ptn_create( "\"[^\"]*\"", 0 );
	t_Token->emit = t_Token->name;

	t_Regex = sym_create( pbnf, "Regex" );
	t_Regex->ptn = pregex_ptn_create( "/(\\\\.|[^/\\\\])*/", 0 );
	t_Regex->emit = t_Regex->name;

	t_ignoreskip = sym_create( pbnf, "/%(ignore|skip)/" );
	t_ignoreskip->ptn = pregex_ptn_create( "%(ignore|skip)", 0 );

	n_colon = sym_create( pbnf, "colon" );

	n_emits = sym_create( pbnf, "emits" );
	n_emits->emit = n_emits->name;

	n_terminal = sym_create( pbnf, "terminal" );

	n_inline = sym_create( pbnf, "inline" );
	n_inline->emit = n_inline->name;

	n_alternation[ 0 ] = sym_create( pbnf, "alternation" );

	n_variable = sym_create( pbnf, "variable" );
	n_variable->emit = n_variable->name;

	n_symbol = sym_create( pbnf, "symbol" );

	n_modifier = sym_create( pbnf, "modifier" );

	n_sequence = sym_create( pbnf, "sequence" );

	n_rule = sym_create( pbnf, "rule" );
	n_rule->emit = n_rule->name;

	n_opt_sequence = sym_create( pbnf, "opt_sequence" );

	n_opt_emits = sym_create( pbnf, "opt_emits" );

	n_alternation[ 1 ] = sym_create( pbnf, "alternation'" );

	n_pos_alternation = sym_create( pbnf, "pos_alternation'" );

	n_definition[ 0 ] = sym_create( pbnf, "definition" );

	n_opt_goal = sym_create( pbnf, "opt_goal" );

	n_definition[ 1 ] = sym_create( pbnf, "definition'" );

	n_pos_definition = sym_create( pbnf, "pos_definition'" );

	n_assocdef = sym_create( pbnf, "assocdef" );

	n_pos_terminal = sym_create( pbnf, "pos_terminal" );

	n_grammar[ 0 ] = sym_create( pbnf, "grammar" );
	pbnf->goal = n_grammar[ 0 ];

	n_grammar[ 1 ] = sym_create( pbnf, "grammar'" );

	n_pos_grammar = sym_create( pbnf, "pos_grammar'" );

	/* Productions */

	prod_create( pbnf, n_colon /* colon */,
		_t_noname[ 0 ], /* ":=" */
		(Symbol*)NULL
	)->emit = "emitsdef";

	prod_create( pbnf, n_colon /* colon */,
		_t_noname[ 1 ], /* ":" */
		(Symbol*)NULL
	);

	prod_create( pbnf, n_emits /* emits */,
		_t_noname[ 2 ], /* "=" */
		t_Identifier, /* /[A-Z_a-z][0-9A-Z_a-z]*\/ */
		(Symbol*)NULL
	);

	prod_create( pbnf, n_terminal /* terminal */,
		t_CCL, /* /\\[(\\\\.|[^\\\\\\]])*\\]/ */
		(Symbol*)NULL
	);
	prod_create( pbnf, n_terminal /* terminal */,
		t_String, /* /'[^']*'/ */
		(Symbol*)NULL
	);
	prod_create( pbnf, n_terminal /* terminal */,
		t_Token, /* /"[^"]*"/ */
		(Symbol*)NULL
	);
	prod_create( pbnf, n_terminal /* terminal */,
		t_Regex, /* /\/(\\\\.|[^\/\\\\])*\// */
		(Symbol*)NULL
	);

	prod_create( pbnf, n_inline /* inline */,
		_t_noname[ 3 ], /* "(" */
		n_alternation[ 0 ], /* alternation */
		_t_noname[ 4 ], /* ")" */
		(Symbol*)NULL
	);

	prod_create( pbnf, n_variable /* variable */,
		t_Identifier, /* /[A-Z_a-z][0-9A-Z_a-z]*\/ */
		(Symbol*)NULL
	);

	prod_create( pbnf, n_symbol /* symbol */,
		n_variable, /* variable */
		(Symbol*)NULL
	);
	prod_create( pbnf, n_symbol /* symbol */,
		n_terminal, /* terminal */
		(Symbol*)NULL
	);
	prod_create( pbnf, n_symbol /* symbol */,
		n_inline, /* inline */
		(Symbol*)NULL
	);

	prod_create( pbnf, n_modifier /* modifier */,
		n_symbol, /* symbol */
		_t_noname[ 5 ], /* "*" */
		(Symbol*)NULL
	)->emit = "kle";

	prod_create( pbnf, n_modifier /* modifier */,
		n_symbol, /* symbol */
		_t_noname[ 6 ], /* "+" */
		(Symbol*)NULL
	)->emit = "pos";

	prod_create( pbnf, n_modifier /* modifier */,
		n_symbol, /* symbol */
		_t_noname[ 7 ], /* "?" */
		(Symbol*)NULL
	)->emit = "opt";

	prod_create( pbnf, n_modifier /* modifier */,
		n_symbol, /* symbol */
		(Symbol*)NULL
	);

	prod_create( pbnf, n_sequence /* sequence */,
		n_sequence, /* sequence */
		n_modifier, /* modifier */
		(Symbol*)NULL
	);
	prod_create( pbnf, n_sequence /* sequence */,
		n_modifier, /* modifier */
		(Symbol*)NULL
	);

	prod_create( pbnf, n_opt_sequence /* opt_sequence */,
		n_sequence, /* sequence */
		(Symbol*)NULL
	);
	prod_create( pbnf, n_opt_sequence /* opt_sequence */,
		(Symbol*)NULL
	);

	prod_create( pbnf, n_opt_emits /* opt_emits */,
		n_emits, /* emits */
		(Symbol*)NULL
	);
	prod_create( pbnf, n_opt_emits /* opt_emits */,
		(Symbol*)NULL
	);

	prod_create( pbnf, n_rule /* rule */,
		n_opt_sequence, /* opt_sequence */
		n_opt_emits, /* opt_emits */
		(Symbol*)NULL
	);

	prod_create( pbnf, n_alternation[ 1 ] /* alternation' */,
		_t_noname[ 8 ], /* "|" */
		n_rule, /* rule */
		(Symbol*)NULL
	);

	prod_create( pbnf, n_pos_alternation /* pos_alternation' */,
		n_pos_alternation, /* pos_alternation' */
		n_alternation[ 1 ], /* alternation' */
		(Symbol*)NULL
	);
	prod_create( pbnf, n_pos_alternation /* pos_alternation' */,
		n_alternation[ 1 ], /* alternation' */
		(Symbol*)NULL
	);

	prod_create( pbnf, n_alternation[ 0 ] /* alternation */,
		n_rule, /* rule */
		n_pos_alternation, /* pos_alternation' */
		(Symbol*)NULL
	);
	prod_create( pbnf, n_alternation[ 0 ] /* alternation */,
		n_rule, /* rule */
		(Symbol*)NULL
	);

	prod_create( pbnf, n_opt_goal /* opt_goal */,
		t_goal, /* "$" */
		(Symbol*)NULL
	);
	prod_create( pbnf, n_opt_goal /* opt_goal */,
		(Symbol*)NULL
	);

	prod_create( pbnf, n_definition[ 1 ] /* definition' */,
		n_terminal, /* terminal */
		(Symbol*)NULL
	);
	prod_create( pbnf, n_definition[ 1 ] /* definition' */,
		n_variable, /* variable */
		(Symbol*)NULL
	);

	prod_create( pbnf, n_pos_definition /* pos_definition' */,
		n_pos_definition, /* pos_definition' */
		n_definition[ 1 ], /* definition' */
		(Symbol*)NULL
	);
	prod_create( pbnf, n_pos_definition /* pos_definition' */,
		n_definition[ 1 ], /* definition' */
		(Symbol*)NULL
	);

	prod_create( pbnf, n_definition[ 0 ] /* definition */,
		_t_noname[ 9 ], /* "@" */
		n_variable, /* variable */
		n_opt_goal, /* opt_goal */
		n_colon, /* colon */
		n_alternation[ 0 ], /* alternation */
		(Symbol*)NULL
	)->emit = "definition";

	prod_create( pbnf, n_definition[ 0 ] /* definition */,
		t_ignoreskip, /* /%(ignore|skip)/ */
		n_pos_definition, /* pos_definition' */
		(Symbol*)NULL
	)->emit = "ignore";


	prod_create( pbnf, n_pos_terminal /* pos_terminal */,
		n_pos_terminal, /* pos_terminal */
		n_terminal, /* terminal */
		(Symbol*)NULL
	);
	prod_create( pbnf, n_pos_terminal /* pos_terminal */,
		n_terminal, /* terminal */
		(Symbol*)NULL
	);

	prod_create( pbnf, n_assocdef /* assocdef */,
		_t_noname[ 10 ], /* "<" */
		n_pos_terminal, /* pos_terminal */
		(Symbol*)NULL
	)->emit = "assoc_left";

	prod_create( pbnf, n_assocdef /* assocdef */,
		_t_noname[ 11 ], /* ">" */
		n_pos_terminal, /* pos_terminal */
		(Symbol*)NULL
	)->emit = "assoc_right";

	prod_create( pbnf, n_assocdef /* assocdef */,
		_t_noname[ 12 ], /* "^" */
		n_pos_terminal, /* pos_terminal */
		(Symbol*)NULL
	)->emit = "assoc_none";


	prod_create( pbnf, n_grammar[ 1 ] /* grammar' */,
		n_definition[ 0 ], /* definition */
		(Symbol*)NULL
	);
	prod_create( pbnf, n_grammar[ 1 ] /* grammar' */,
		n_assocdef, /* assocdef */
		(Symbol*)NULL
	);

	prod_create( pbnf, n_pos_grammar /* pos_grammar' */,
		n_pos_grammar, /* pos_grammar' */
		n_grammar[ 1 ], /* grammar' */
		(Symbol*)NULL
	);
	prod_create( pbnf, n_pos_grammar /* pos_grammar' */,
		n_grammar[ 1 ], /* grammar' */
		(Symbol*)NULL
	);

	prod_create( pbnf, n_grammar[ 0 ] /* grammar */,
		n_pos_grammar, /* pos_grammar' */
		(Symbol*)NULL
	);

/*ETARENEG*/

	gram_prepare( pbnf );
	p = par_create( pbnf );

	if( !par_parse( &root, p, src ) )
		RETURN( FALSE );

	/* ast_dump_short( stdout, root ); */

	/* Traverse AST, retrieve parsed grammar */
	traverse( g, root );

	/* Set symbols as terminals */
	{
		int			i;
		Symbol*		sym;
		Symbol*		term;

		do
		{
			for( i = 0; ( sym = sym_get( g, i ) ); i++ )
			{
				if( !SYM_IS_TERMINAL( sym )
					&& !sym_getprod( sym, 1 )
					&& SYM_IS_TERMINAL( ( term = prod_getfromrhs(
											sym_getprod( sym, 0 ), 0 ) ) )
					&& !prod_getfromrhs( sym_getprod( sym, 0 ), 1 )
					&& term->usages == 1 )
				{
					/*
					fprintf( stderr, "%s = >%s<\n", sym->name, term->name );
					*/

					prod_free( sym_getprod( sym, 0 ) );

					sym->ptn = term->ptn;
					sym->ccl = term->ccl;

					if( ( sym->str = term->str ) )
						term->name = NULL;

					term->ptn = NULL;
					term->ccl = NULL;
					term->str = NULL;

					sym_free( term );
					break;
				}
			}
		}
		while( sym );
	}

	GRAMMAR_DUMP( g );


	/* If there is no goal, then the last defined nonterminal
		becomes the goal symbol */
	if( !g->goal )
	{
		plistel*	e	= plist_last( g->symbols );

		while( e )
		{
			if( !SYM_IS_TERMINAL( ( g->goal = (Symbol*)plist_access( e ) ) ) )
				break;

			e = plist_prev( e );
		}

		if( !e )
			g->goal = NULL;
	}

	/* Look for unique goal sequence */
	if( sym_getprod( g->goal, 1 ) )
	{
		Symbol*		s;

		s = sym_create( g, derive_name( g, g->goal->name ) );
		s->flags.freename = TRUE;

		prod_create( g, s, g->goal, (Symbol*)NULL );
		g->goal = s;
	}

	/* gram_dump( stderr, gram ); */

	RETURN( TRUE );
}
