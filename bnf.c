/* -MODULE----------------------------------------------------------------------
UniCC² Parser Generator
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	bnf.c
Usage:	Parsing grammars from BNF/EBNF definitions
----------------------------------------------------------------------------- */

#include "unicc.h"

#define NAMELEN			72
#define DERIVCHAR		'\''

static pboolean traverse_production( Grammar* g, Symbol* lhs, AST_node* node );

/* Derive name from basename */
static char* derive_name( Grammar* g, char* base )
{
	int             i;
	static
	char    deriv   [ ( NAMELEN * 2 ) + 1 ];

	sprintf( deriv, "%s%c", base, DERIVCHAR );

	for( i = 0; strlen( deriv ) < ( NAMELEN * 2 ); i++ )
	{
		if( !sym_get_by_name( g, deriv ) )
			return deriv;

		sprintf( deriv + strlen( deriv ), "%c", DERIVCHAR );
	}

	return (char*)NULL;
}

#define NODE_IS( n, s ) 	( !strcmp( (n)->emit, s ) )

static Symbol* traverse_symbol( Grammar* g, Symbol* lhs, AST_node* node )
{
	Symbol*		sym			= (Symbol*)NULL;
	AST_node*		child;
	char		name		[ NAMELEN * 2 + 1 ];

	/* fprintf( stderr, "sym >%s<\n", node->emit ); */

	if( NODE_IS( node, "inline") )
	{
		sym = sym_create( g, derive_name( g, lhs->name ),
				FLAG_FREENAME | FLAG_DEFINED | FLAG_GENERATED );

		for( child = node->child; child; child = child->next )
			if( !traverse_production( g, sym, child->child ) )
				return (Symbol*)NULL;
	}
	else if( NODE_IS( node, "Terminal") || NODE_IS( node, "Nonterminal") )
	{
		sprintf( name, "%.*s", (int)node->len, node->start );

		if( !( sym = sym_get_by_name( g, name ) ) )
			sym = sym_create( g, name, FLAG_FREENAME );
	}
	else
		MISSINGCASE;

	if( sym )
		sym->flags |= FLAG_CALLED;

	return sym;
}

static pboolean traverse_production( Grammar* g, Symbol* lhs, AST_node* node )
{
	Symbol*		sym;
	Production*		prod;

	prod = prod_create( g, lhs, (Symbol*)NULL );

	for( ; node; node = node->next )
	{
		/* fprintf( stderr, "prod >%s<\n", node->emit ); */

		if( NODE_IS( node, "symbol" ) )
		{
			if( !( sym = traverse_symbol( g, lhs, node->child ) ) )
				return FALSE;

			prod_append( prod, sym );
		}
		else
		{
			sym = traverse_symbol( g, lhs, node->child->child );

			if( NODE_IS( node, "pos" ) )
				sym = sym_mod_positive( sym );

			else if( NODE_IS( node, "opt" ) )
				sym = sym_mod_optional( sym );

			else if( NODE_IS( node, "star" ) )
				sym = sym_mod_kleene( sym );

			prod_append( prod, sym );
		}
	}

	return TRUE;
}

static pboolean ast_to_gram( Grammar* g, AST_node* ast )
{
	Symbol*		sym			= (Symbol*)NULL;
	AST_node*		child;
	char		name		[ NAMELEN * 2 + 1 ];

	for( ; ast; ast = ast->next )
	{
		if( NODE_IS( ast, "nonterm" ) )
		{
			/* Get nonterminal's name */
			child = ast->child;
			sprintf( name, "%.*s", (int)child->len, child->start );

			/* Create the non-terminal symbol */
			if( !( sym = sym_get_by_name( g, name ) ) )
				sym = sym_create( g, name,
						FLAG_FREENAME | FLAG_DEFINED );

			for( child = child->next; child; child = child->next )
			{
				if( NODE_IS( child, "production" ) )
				{
					if( !traverse_production( g, sym, child->child ) )
						return FALSE;
				}
				else
					MISSINGCASE;
			}
		}
		else
			MISSINGCASE;
	}

	/* If there is no goal, then the last defined nonterminal
		becomes the goal symbol */
	if( !g->goal )
		g->goal = sym;

	/* Look for unique goal sequence; if this is not the case, wrap it with
		another, generated nonterminal. */
	if( sym_getprod( g->goal, 1 ) )
	{
		sym = sym_create( g, derive_name( g, g->goal->name ),
				FLAG_FREENAME | FLAG_DEFINED
					| FLAG_CALLED | FLAG_GENERATED );

		prod_create( g, sym, g->goal, (Symbol*)NULL );
		g->goal = sym;
	}

	return TRUE;
}

/** Compiles a Backus-Naur form definition into a grammar.

//g// is the grammar that receives the result of the parse.
This grammar gets extended with new definitions if it already contains symbols.

//src// is the BNF definition string that defines the grammar.

The function returns TRUE in case the grammar could be compiled,
FALSE otherwise.

**Grammar:**
```
symbol : Terminal | Nonterminal ;
sequence : sequence symbol | symbol ;
production : sequence | ;
alternation : alternation '|' production | production ;

nonterm : Nonterminal ':' alternation ';';
defs : defs nonterm | nonterm ;

grammar$ : defs ;
```
*/
pboolean gram_from_bnf( Grammar* g, char* src )
{
	Parser*		par;
	Grammar*		bnf;
	AST_node*		ast;

	Symbol*		terminal;
	Symbol*		nonterminal;
	Symbol*		colon;
	Symbol*		semi;
	Symbol*		pipe;

	Symbol*		n_symbol;
	Symbol*		n_seq;
	Symbol*		n_prod;
	Symbol*		n_alt;
	Symbol*		n_nonterm;
	Symbol*		n_defs;
	Symbol*		n_grammar;

	PROC( "gram_from_bnf" );

	if( !( g && src ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	PARMS( "g", "%p", g );
	PARMS( "src", "%s", src );

	/* Define a grammar for BNF */
	bnf = gram_create();

	/* Terminals */
	terminal = sym_create( bnf, "Terminal", FLAG_NONE );
	terminal->emit = "Terminal";

	nonterminal = sym_create( bnf, "Nonterminal", FLAG_NONE );
	nonterminal->emit = "Nonterminal";

	colon = sym_create( bnf, ":", FLAG_NONE );
	semi = sym_create( bnf, ";", FLAG_NONE );
	pipe = sym_create( bnf, "|", FLAG_NONE );

	/* Nonterminals */
	n_symbol = sym_create( bnf, "symbol", FLAG_NONE );
	n_symbol->emit = "symbol";

	n_seq = sym_create( bnf, "sequence", FLAG_NONE );

	n_prod = sym_create( bnf, "production", FLAG_NONE );
	n_prod->emit = "production";

	n_alt = sym_create( bnf, "alternation", FLAG_NONE );

	n_nonterm = sym_create( bnf, "nonterm", FLAG_NONE );
	n_nonterm->emit = "nonterm";

	n_defs = sym_create( bnf, "defs", FLAG_NONE );

	n_grammar = sym_create( bnf, "grammar", FLAG_NONE );
	bnf->goal = n_grammar;

	/* Productions */
	prod_create( bnf, n_symbol, terminal, (Symbol*)NULL );
	prod_create( bnf, n_symbol, nonterminal, (Symbol*)NULL );

	prod_create( bnf, n_seq, n_seq, n_symbol, (Symbol*)NULL );
	prod_create( bnf, n_seq, n_symbol, (Symbol*)NULL );

	prod_create( bnf, n_prod, n_seq, (Symbol*)NULL );
	prod_create( bnf, n_prod, (Symbol*)NULL );

	prod_create( bnf, n_alt, n_alt, pipe, n_prod, (Symbol*)NULL );
	prod_create( bnf, n_alt, n_prod, (Symbol*)NULL );

	prod_create( bnf, n_nonterm, nonterminal, colon,
							n_alt, semi, (Symbol*)NULL );

	prod_create( bnf, n_defs, n_defs, n_nonterm, (Symbol*)NULL );
	prod_create( bnf, n_defs, n_nonterm, (Symbol*)NULL );

	prod_create( bnf, n_grammar, n_defs, (Symbol*)NULL );

	/* Setup a parser */
	par = par_create( bnf );
	GRAMMAR_DUMP( bnf );

	/* Lexer */
	terminal->ptn = pregex_ptn_create( "[^a-z_:;| \t\r\n][^:;| \t\r\n]*", 0 );
	nonterminal->ptn = pregex_ptn_create( "[a-z_][^:;| \t\r\n]*", 0 );

	/* Parse */
	if( !par_parse( &ast, par, src ) )
	{
		par_free( par );
		gram_free( bnf );
		RETURN( FALSE );
	}

	AST_DUMP( ast );

	if( !ast_to_gram( g, ast ) )
		RETURN( FALSE );

	GRAMMAR_DUMP( g );

	par_free( par );
	gram_free( bnf );
	ast_free( ast );

	RETURN( TRUE );
}

/** Compiles an Extended Backus-Naur form definition into a grammar.

//g// is the grammar that receives the result of the parse.
This grammar is extended to new definitions when it already contains symbols.

//src// is the EBNF definition string that defines the grammar.

The function returns TRUE in case the grammar could be compiled,
FALSE otherwise.

**Grammar:**
```
symbol : '(' alternation ')' | Terminal | Nonterminal ;
modifier : symbol | symbol '*' | symbol '+' | symbol '?' ;
sequence : sequence modifier | modifier ;
production : sequence | ;
alternation : alternation '|' production | production ;

nonterm : Nonterminal ':' alternation ';';
defs : defs nonterm | nonterm ;

grammar$ : defs ;
```
*/
pboolean gram_from_ebnf( Grammar* g, char* src )
{
	Parser*		par;
	Grammar*		ebnf;
	AST_node*		ast;

	Symbol*		terminal;
	Symbol*		nonterminal;
	Symbol*		colon;
	Symbol*		semi;
	Symbol*		pipe;
	Symbol*		brop;
	Symbol*		brcl;
	Symbol*		star;
	Symbol*		quest;
	Symbol*		plus;

	Symbol*		n_symbol;
	Symbol*		n_mod;
	Symbol*		n_seq;
	Symbol*		n_prod;
	Symbol*		n_alt;
	Symbol*		n_nonterm;
	Symbol*		n_defs;
	Symbol*		n_grammar;

	Production*		p;

	PROC( "gram_from_ebnf" );

	if( !( g && src ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	PARMS( "g", "%p", g );
	PARMS( "src", "%s", src );

	/* Define a grammar for EBNF */

	ebnf = gram_create();

	/* Terminals */
	terminal = sym_create( ebnf, "Terminal", FLAG_NONE );
	terminal->emit = "Terminal";

	nonterminal = sym_create( ebnf, "Nonterminal", FLAG_NONE );
	nonterminal->emit = "Nonterminal";

	colon = sym_create( ebnf, ":", FLAG_NONE );
	semi = sym_create( ebnf, ";", FLAG_NONE );
	pipe = sym_create( ebnf, "|", FLAG_NONE );
	brop = sym_create( ebnf, "(", FLAG_NONE );
	brcl = sym_create( ebnf, ")", FLAG_NONE );
	star = sym_create( ebnf, "*", FLAG_NONE );
	quest = sym_create( ebnf, "?", FLAG_NONE );
	plus = sym_create( ebnf, "+", FLAG_NONE );

	/* Nonterminals */
	n_symbol = sym_create( ebnf, "symbol", FLAG_NONE );
	n_symbol->emit = "symbol";

	n_mod = sym_create( ebnf, "modifier", FLAG_NONE );
	n_seq = sym_create( ebnf, "sequence", FLAG_NONE );

	n_prod = sym_create( ebnf, "production", FLAG_NONE );
	n_prod->emit = "production";

	n_alt = sym_create( ebnf, "alternation", FLAG_NONE );

	n_nonterm = sym_create( ebnf, "nonterm", FLAG_NONE );
	n_nonterm->emit = "nonterm";

	n_defs = sym_create( ebnf, "defs", FLAG_NONE );

	n_grammar = sym_create( ebnf, "grammar", FLAG_NONE );
	ebnf->goal = n_grammar;

	/* Productions */
	p = prod_create( ebnf, n_symbol, brop, n_alt, brcl, (Symbol*)NULL );
	p->emit = "inline";
	p = prod_create( ebnf, n_symbol, terminal, (Symbol*)NULL );
	p = prod_create( ebnf, n_symbol, nonterminal, (Symbol*)NULL );

	p = prod_create( ebnf, n_mod, n_symbol, (Symbol*)NULL );
	p = prod_create( ebnf, n_mod, n_symbol, star, (Symbol*)NULL );
	p->emit = "star";
	p = prod_create( ebnf, n_mod, n_symbol, plus, (Symbol*)NULL );
	p->emit = "pos";
	p = prod_create( ebnf, n_mod, n_symbol, quest, (Symbol*)NULL );
	p->emit = "opt";

	prod_create( ebnf, n_seq, n_seq, n_mod, (Symbol*)NULL );
	prod_create( ebnf, n_seq, n_mod, (Symbol*)NULL );

	prod_create( ebnf, n_prod, n_seq, (Symbol*)NULL );
	prod_create( ebnf, n_prod, (Symbol*)NULL );

	prod_create( ebnf, n_alt, n_alt, pipe, n_prod, (Symbol*)NULL );
	prod_create( ebnf, n_alt, n_prod, (Symbol*)NULL );

	prod_create( ebnf, n_nonterm, nonterminal, colon,
							n_alt, semi, (Symbol*)NULL );

	prod_create( ebnf, n_defs, n_defs, n_nonterm, (Symbol*)NULL );
	prod_create( ebnf, n_defs, n_nonterm, (Symbol*)NULL );

	prod_create( ebnf, n_grammar, n_defs, (Symbol*)NULL );

	/* Setup a parser */
	par = par_create( ebnf );
	GRAMMAR_DUMP( ebnf );

	/* Lexer */
	terminal->ptn = pregex_ptn_create(
		"[^a-z_:;|()*?+ \t\r\n][^:;|()*?+ \t\r\n]*" 	/* Ident */
		"|/(\\.|[^\\/])*/(@\\w*)?"						/* /regular
																expression/ */
		"|\"[^\"]*\"(@\\w*)?"							/* "double-quoted
																string" */
		"|'[^']*'(@\\w*)?",								/* 'single-quoted
																string' */
		0 );

	nonterminal->ptn = pregex_ptn_create(
		"[a-z_][^:;|()*?+ \t\r\n]*",					/* ident */
		0 );

	/* Parse */
	if( !par_parse( &ast, par, src ) )
	{
		par_free( par );
		gram_free( ebnf );
		return FALSE;
	}

	AST_DUMP( ast );

	if( !ast_to_gram( g, ast ) )
		return FALSE;

	GRAMMAR_DUMP( g );

	par_free( par );
	gram_free( ebnf );
	ast_free( ast );

	return TRUE;
}
