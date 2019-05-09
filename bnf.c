/* -MODULE----------------------------------------------------------------------
UniCC² Parser Generator
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	pbnf.c
Usage:	Parsing full parser definitions from Phorward BNF (PBNF).
----------------------------------------------------------------------------- */

#include "unicc.h"

static pboolean traverse_production( Grammar* gram, Symbol* lhs, AST_node* node );

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

#define NODE_IS( n, s ) 	( !strcmp( (n)->emit, s ) )

static Symbol* traverse_terminal( Grammar* gram, AST_node* node )
{
	Symbol*			sym;
	char*			name;

	PROC( "traverse_terminal" );
	VARS( "node->emit", "%s", node->emit );

	name = pstrndup( node->start, node->len );
	VARS( "name", "%s", name );

	if( !( sym = sym_get_by_name( gram, name ) ) )
	{
		if( !NODE_IS( node, "Terminal" ) )
		{
			memmove( name, name + 1, strlen( name ) - 1 );
			name[ strlen( name ) - 2 ] = '\0';

			sym = sym_create( gram, name );
			sym->flags.nameless = TRUE;

			if( NODE_IS( node, "CCL" ) )
			{
				sym->ptn = pregex_ptn_create( name, PREGEX_COMP_NOERRORS );
				sym->ccl = sym->ptn->ccl;
			}
			else
			{
				if( NODE_IS( node, "Token") || NODE_IS( node, "String" ) )
				{
					sym->ptn = pregex_ptn_create( name, PREGEX_COMP_STATIC );
					sym->str = name;

					if( NODE_IS( node, "Token") )
						sym->emit = name;
				}
				else if( NODE_IS( node, "Regex" ) )
					sym->ptn = pregex_ptn_create( name, PREGEX_COMP_NOERRORS );
				else
					MISSINGCASE;
			}
		}
		else
			sym = sym_create( gram, name );

		sym->flags.freename = TRUE;
	}
	else
		pfree( name );

	RETURN( sym );
}

static Symbol* traverse_symbol( Grammar* gram, Symbol* lhs, AST_node* node )
{
	Symbol*			sym;
	char			name		[ NAMELEN * 2 + 1 ];
	AST_node*		child;

	PROC( "traverse_symbol" );
	PARMS( "node->emit", "%s", node->emit );

	if( NODE_IS( node, "inline" ) )
	{
		sym = sym_create( gram, derive_name( gram, lhs->name ) );
		sym->flags.freename = TRUE;
		sym->flags.nameless = TRUE;
		sym->flags.defined = TRUE;
		sym->flags.generated = TRUE;

		for( child = node->child; child; child = child->next )
			if( !traverse_production( gram, sym, child->child ) )
				RETURN( (Symbol*)NULL );
	}
	else if( NODE_IS( node, "Nonterminal" ) )
	{
		sprintf( name, "%.*s", (int)node->len, node->start );

		if( !( sym = sym_get_by_name( gram, name ) ) )
		{
			sym = sym_create( gram, pstrdup( name ) );
			sym->flags.freename = TRUE;
		}
	}
	else
		sym = traverse_terminal( gram, node );

	if( sym )
		sym->flags.called = TRUE;

	RETURN( sym );
}


static pboolean traverse_production( Grammar* gram, Symbol* lhs, AST_node* node )
{
	Symbol*			sym;
	Symbol*			csym;
	Production*		prod;
	Production*		popt;
	int				i;

	PROC( "traverse_production" );

	prod = prod_create( gram, lhs, (Symbol*)NULL );

	for( ; node; node = node->next )
	{
		VARS( "node->emit", "%s", node->emit );

		if( NODE_IS( node, "symbol" ) )
		{
			if( !( sym = traverse_symbol( gram, lhs, node->child ) ) )
				RETURN( FALSE );

			prod_append( prod, sym );
		}
		else if( NODE_IS( node, "emits" ) )
		{
			if( NODE_IS( node->child, "Code" ) )
				prod->emit = pstrndup( node->child->start + 2,
										node->child->len - 4 );
			else
				prod->emit = pstrndup( node->child->start, node->child->len );

			prod->flags.freeemit = TRUE;
		}
		else
		{
			sym = traverse_symbol( gram, lhs, node->child->child );

			if( NODE_IS( node, "pos" ) )
				sym = sym_mod_positive( sym );

			else if( NODE_IS( node, "opt" ) )
				sym = sym_mod_optional( sym );

			else if( NODE_IS( node, "kle" ) )
				sym = sym_mod_kleene( sym );

			prod_append( prod, sym );
		}
	}

	/*
		Optimize productions with one generated symbol
		that was introduced via 'inline'.
	*/
	if( ( sym = prod_getfromrhs( prod, 0 ) )
			&& !prod_getfromrhs( prod, 1 ) )
	{
		if( !SYM_IS_TERMINAL( sym )
				&& sym->flags.generated
					&& sym->emit && !sym_getprod( sym, 1 ) )
		{
			if( sym->flags.freeemit )
			{
				prod->emit = pstrdup( sym->emit );
				prod->flags.freeemit = TRUE;
			}
			else
				prod->emit = sym->emit;

			prod_remove( prod, sym );

			popt = sym_getprod( sym, 0 );

			for( i = 0; ( csym = prod_getfromrhs( popt, i ) ); i++ )
				prod_append( prod, csym );

			sym_drop( sym );
		}
	}

	RETURN( TRUE );
}

static pboolean ast_to_gram( Grammar* gram, AST_node* ast )
{
	Symbol*			sym;
	Symbol*			nonterm		= (Symbol*)NULL;
	AST_node* 		node;
	AST_node*		child;
	char			name		[ NAMELEN * 2 + 1 ];
	char			def			[ NAMELEN * 2 + 1 ];
	char*			emit;
	pboolean		flag_ignore;
	Assoc			assoc;
	unsigned int	prec		= 0;

	/* ast_dump_short( stderr, ast ); */

	for( node = ast; node; node = node->next )
	{
		emit = (char*)NULL;
		flag_ignore = FALSE;

		/* fprintf( stderr, "gram >%s<\n", node->emit ); */

		if( NODE_IS( node, "nontermdef" ) )
		{
			child = node->child;

			sprintf( name, "%.*s", (int)child->len, child->start );

			/* Create the terminal symbol */
			if( !( nonterm = sym = sym_get_by_name( gram, name ) ) )
			{
				nonterm = sym = sym_create( gram, pstrdup( name ) );
				sym->flags.freename = TRUE;
			}

			sym->flags.defined = TRUE;

			for( child = node->child->next; child; child = child->next )
			{
				if( NODE_IS( child, "flag_goal" ) )
				{
					if( !gram->goal ) /* fixme */
					{
						gram->goal = sym;
						sym->flags.called = TRUE;
					}
				}
				else if( NODE_IS( child, "flag_lexem" ) )
					sym->flags.lexem = TRUE;
				else if( NODE_IS( child, "emitsdef" ) )
					sym->emit = sym->name;
				else if( NODE_IS( child, "production" ) )
				{
					if( !traverse_production( gram, sym, child->child ) )
						return FALSE;
				}
			}
		}
		else if( NODE_IS( node, "termdef" ) )
		{
			child = node->child;
			*name = '\0';

			if( NODE_IS( child, "flag_ignore" ) )
			{
				flag_ignore = TRUE;
				child = child->next;
			}
			else if( NODE_IS( child, "Terminal" ) )
			{
				sprintf( name, "%.*s", (int)child->len, child->start );
				child = child->next;
			}

			if( NODE_IS( child, "emitsdef" ) )
			{
				emit = name;
				child = child->next;
			}

			sym = sym_create( gram, *name ? pstrdup( name ) : (char*)NULL );
			sym->flags.freename = TRUE;
			sym->flags.defined = TRUE;

			if( NODE_IS( child, "CCL" ) )
			{
				sprintf( def, "%.*s", (int)child->len, child->start );

				sym->ptn = pregex_ptn_create( def, PREGEX_COMP_NOERRORS );
				sym->ccl = sym->ptn->ccl;
			}
			else
			{
				sprintf( def, "%.*s", (int)child->len - 2, child->start + 1 );

				if( NODE_IS( child, "Token") || NODE_IS( child, "String" ) )
				{
					sym->str = pstrdup( def );
					sym->ptn = pregex_ptn_create( def, PREGEX_COMP_STATIC );
				}
				else if( NODE_IS( child, "Regex" ) )
					sym->ptn = pregex_ptn_create( def, PREGEX_COMP_NOERRORS );
				else
					MISSINGCASE;
			}

			child = child->next;

			if( child && NODE_IS( child, "emits" ) )
			{
				if( NODE_IS( child->child, "Code" ) )
					emit = pstrndup( child->child->start + 2,
											child->child->len - 4 );
				else
					emit = pstrndup( child->child->start, child->child->len );
			}

			/* Set emitting & flags */
			if( emit == name )
				sym->emit = sym->name;
			else if( ( sym->emit = emit ) )
				sym->flags.freeemit = TRUE;

			if( flag_ignore )
				sym->flags.whitespace = TRUE;
		}
		else if( NODE_IS( node, "assoc_left" )
					|| NODE_IS( node, "assoc_right" )
						|| NODE_IS( node, "assoc_not" ) )
		{
			if( NODE_IS( node, "assoc_left" ) )
				assoc = ASSOC_LEFT;
			else if( NODE_IS( node, "assoc_right" ) )
				assoc = ASSOC_RIGHT;
			else
				assoc = ASSOC_NOT;

			prec++;

			for( child = node->child; child; child = child->next )
			{
				sym = traverse_terminal( gram, child );
				sym->assoc = assoc;
				sym->prec = prec;
			}
		}
	}

	/* If there is no goal, then the last defined nonterm becomes goal symbol */
	if( !gram->goal && !( gram->goal = nonterm ) )
	{
		fprintf( stderr, "There is not any nonterminal symbol defined!\n" );
		return FALSE;
	}

	/* Look for unique goal sequence */
	if( sym_getprod( gram->goal, 1 ) )
	{
		nonterm = sym_create( gram, derive_name( gram, gram->goal->name ) );
		nonterm->flags.freename = TRUE;

		prod_create( gram, nonterm, gram->goal, (Symbol*)NULL );
		gram->goal = nonterm;
	}

	/* gram_dump( stderr, gram ); */

	return TRUE;
}


/** Compiles a Phorward Backus-Naur form definition into a parser.

//g// is the grammar that receives the result of the parse.
This grammar is extended to new definitions when it already contains symbols.

In difference to gram_from_bnf() and gram_from_ebnf(),
par_from_pbnf() allows for a full-fledged parser definition with
lexical analyzer-specific definitions, grammar and AST construction features.


**Grammar:**
```
// Whitespace ------------------------------------------------------------------

%skip			/[ \t\r\n]+/
%skip			/\/\/[^\n]*\n/
%skip			/\/\*([^*]|\*[^\/])*\*\/|\/\/[^\n]*\n|#[^\n]*\n/

// Terminals -------------------------------------------------------------------

@Terminal		:= /[A-Z][A-Za-z0-9_]*\
@Nonterminal	:= /[a-z_][A-Za-z0-9_]*\

@CCL 			:= /\[(\\.|[^\\\]])*\]/
@String 		:= /'[^']*'/
@Token			:= /"[^"]*"/
@Regex 			:= /\/(\\.|[^\\\/])*\//

@Int			:= /[0-9]+/
@Function		:= /[A-Za-z_][A-Za-z0-9_]*\(\)/

@Flag_goal		:= '$'
@Flag_ignore	:= /%(ignore|skip)/

// Nonterminals ----------------------------------------------------------------

@colon          : ':=' = emitsdef
                | ':'

@emits          := '=' Terminal
                | '=' Nonterminal

@terminal		: CCL | String | Token | Regex | Function

@inline			:= '(' alternation ')'

@symbol 		:= Terminal
                | Nonterminal
                | terminal
                | inline

@modifier		: symbol '*' = kle
                | symbol '+' = pos
                | symbol '?' = opt
                | symbol

@sequence		: sequence modifier | modifier

@production	 	:= sequence? emits?

@alternation	: alternation '|' production | production

@nontermdef		:= '@' Nonterminal Flag_goal? colon alternation

@termdef		:= '@' Terminal colon terminal emits?
                | Flag_ignore terminal emits?

@assocdef		: '<' terminal+	= assoc_left
                | '>' terminal+	= assoc_right
                | '^' terminal+	= assoc_none

@definition		: nontermdef
                | termdef
                | assocdef

@grammar	$	: definition+
```
*/
pboolean gram_from_bnf( Grammar* g, char* src )
{
	Parser*			ppar;
	Grammar*		pbnf;
	AST_node*		ast;

	Symbol*			whitespace;
	Symbol*			comment;

	Symbol*			def;
	Symbol*			terminal;
	Symbol*			nonterminal;
	Symbol*			colon;
	Symbol*			colonequal;
	Symbol*			pipe;
	Symbol*			brop;
	Symbol*			brcl;
	Symbol*			star;
	Symbol*			quest;
	Symbol*			plus;
	Symbol*			equal;

	Symbol*			code;
	Symbol*			t_ccl;
	Symbol*			t_string;
	Symbol*			t_token;
	Symbol*			t_regex;
	Symbol*			flag_goal;
	Symbol*			flag_lexem;
	Symbol*			flag_ignore;
	Symbol*			assoc_left;
	Symbol*			assoc_right;
	Symbol*			assoc_not;

	Symbol*			n_emit;
	Symbol*			n_opt_emit;
	Symbol*			n_terminal;
	Symbol*			n_pos_terminal;
	Symbol*			n_symbol;
	Symbol*			n_inline;
	Symbol*			n_mod;
	Symbol*			n_seq;
	Symbol*			n_opt_seq;
	Symbol*			n_prod;
	Symbol*			n_alt;

	Symbol*			n_nonterm_flags;
	Symbol*			n_nonterm;
	Symbol*			n_term;
	Symbol*			n_assoc;

	Symbol*			n_def;
	Symbol*			n_defs;
	Symbol*			n_grammar;

	Production*		p;

	PROC( "gram_from_pbnf" );

	if( !( g && src ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	PARMS( "g", "%p", g );
	PARMS( "src", "%s", src );

	/* Define a grammar for pbnf */

	pbnf = gram_create();

	/* Terminals */

	MSG( "Defining terminals" );

	whitespace = sym_create( pbnf, (char*)NULL );
	whitespace->flags.whitespace = TRUE;

	comment = sym_create( pbnf, (char*)NULL );
	comment->flags.whitespace = TRUE;

	terminal = sym_create( pbnf, "Terminal" );
	terminal->emit = "Terminal";

	nonterminal = sym_create( pbnf, "Nonterminal" );
	nonterminal->emit = "Nonterminal";

	code = sym_create( pbnf, "Code" );
	code->emit = "Code";

	colonequal = sym_create( pbnf, ":=" );
	colonequal->emit = "emitsdef";

	def = sym_create( pbnf, "@" );
	colon = sym_create( pbnf, ":" );
	pipe = sym_create( pbnf, "|" );
	brop = sym_create( pbnf, "(" );
	brcl = sym_create( pbnf, ")" );
	star = sym_create( pbnf, "*" );
	quest = sym_create( pbnf, "?" );
	plus = sym_create( pbnf, "+" );

	equal = sym_create( pbnf, "=" );

	flag_goal = sym_create( pbnf, "$" );
	flag_goal->emit = "flag_goal";

	flag_lexem = sym_create( pbnf, "!" );
	flag_lexem->emit = "flag_lexem";

	flag_ignore = sym_create( pbnf, "%ignore" );
	flag_ignore->emit = "flag_ignore";

	t_ccl = sym_create( pbnf, "CCL" );
	t_ccl->emit = "CCL";
	t_string = sym_create( pbnf, "String" );
	t_string->emit = "String";
	t_token = sym_create( pbnf, "Token" );
	t_token->emit = "Token";
	t_regex = sym_create( pbnf, "Regex" );
	t_regex->emit = "Regex";

	assoc_left = sym_create( pbnf, "<" );
	assoc_right = sym_create( pbnf, ">" );
	assoc_not = sym_create( pbnf, "^" );

	/* Nonterminals */
	MSG( "Nonterminals" );

	n_emit = sym_create( pbnf, "emit" );
	n_emit->emit = "emits";

	n_opt_emit = sym_mod_optional( n_emit );

	n_terminal = sym_create( pbnf, "terminal" );
	n_pos_terminal = sym_create( pbnf, "pos_terminal" );

	n_inline = sym_create( pbnf, "inline" );
	n_inline->emit = "inline";

	n_symbol = sym_create( pbnf, "symbol" );
	n_symbol->emit = "symbol";

	n_mod = sym_create( pbnf, "modifier" );
	n_seq = sym_create( pbnf, "sequence" );
	n_opt_seq = sym_create( pbnf, "opt_sequence" );

	n_prod = sym_create( pbnf, "production" );
	n_prod->emit = "production";

	n_alt = sym_create( pbnf, "alternation" );

	n_nonterm_flags = sym_create( pbnf, "nontermdef_flags" );

	n_nonterm = sym_create( pbnf, "nontermdef" );
	n_nonterm->emit = "nontermdef";

	n_term = sym_create( pbnf, "termdef" );
	n_term->emit = "termdef";

	n_assoc = sym_create( pbnf, "assocdef" );
	n_assoc->emit = "assocdef";

	n_def = sym_create( pbnf, "def" );

	n_defs = sym_create( pbnf, "defs" );

	n_grammar = sym_create( pbnf, "grammar" );
	pbnf->goal = n_grammar;

	/* Productions */
	MSG( "Productions" );

		/* emit */
	prod_create( pbnf, n_emit, equal, nonterminal, (Symbol*)NULL );
	prod_create( pbnf, n_emit, equal, terminal, (Symbol*)NULL );
	prod_create( pbnf, n_emit, equal, code, (Symbol*)NULL );

		/* terminal */

	prod_create( pbnf, n_terminal, t_ccl, (Symbol*)NULL );
	prod_create( pbnf, n_terminal, t_string, (Symbol*)NULL );
	prod_create( pbnf, n_terminal, t_token, (Symbol*)NULL );
	prod_create( pbnf, n_terminal, t_regex, (Symbol*)NULL );

		/* pos_terminal */

	prod_create( pbnf, n_pos_terminal, n_pos_terminal, n_terminal,
						(Symbol*)NULL );
	prod_create( pbnf, n_pos_terminal, n_terminal, (Symbol*)NULL );

		/* inline */

	prod_create( pbnf, n_inline, brop, n_alt, brcl, (Symbol*)NULL );

		/* symbol */

	prod_create( pbnf, n_symbol, terminal, (Symbol*)NULL );
	prod_create( pbnf, n_symbol, nonterminal, (Symbol*)NULL );
	prod_create( pbnf, n_symbol, n_terminal, (Symbol*)NULL );
	prod_create( pbnf, n_symbol, n_inline, (Symbol*)NULL );

		/* mod */
	prod_create( pbnf, n_mod, n_symbol, (Symbol*)NULL );

	p = prod_create( pbnf, n_mod, n_symbol, star, (Symbol*)NULL );
	p->emit = "kle";

	p = prod_create( pbnf, n_mod, n_symbol, plus, (Symbol*)NULL );
	p->emit = "pos";

	p = prod_create( pbnf, n_mod, n_symbol, quest, (Symbol*)NULL );
	p->emit = "opt";

		/* seq */
	prod_create( pbnf, n_seq, n_seq, n_mod, (Symbol*)NULL );
	prod_create( pbnf, n_seq, n_mod, (Symbol*)NULL );

		/* n_opt_seq */
	prod_create( pbnf, n_opt_seq, n_seq, (Symbol*)NULL );
	prod_create( pbnf, n_opt_seq, (Symbol*)NULL );

		/* prod */
	prod_create( pbnf, n_prod, n_opt_seq, n_opt_emit, (Symbol*)NULL );

		/* alt */
	prod_create( pbnf, n_alt, n_alt, pipe, n_prod, (Symbol*)NULL );
	prod_create( pbnf, n_alt, n_prod, (Symbol*)NULL );

		/* nonterm def */

	prod_create( pbnf, n_nonterm_flags,
						n_nonterm_flags, flag_goal, (Symbol*)NULL );
	prod_create( pbnf, n_nonterm_flags,
						n_nonterm_flags, flag_lexem, (Symbol*)NULL );
	prod_create( pbnf, n_nonterm_flags, (Symbol*)NULL );

	prod_create( pbnf, n_nonterm, def, nonterminal,
						n_nonterm_flags, colon, n_alt, (Symbol*)NULL );
	prod_create( pbnf, n_nonterm, def, nonterminal,
						n_nonterm_flags, colonequal, n_alt, (Symbol*)NULL );

		/* term def */

	prod_create( pbnf, n_term, def, terminal, colon, n_terminal,
						n_opt_emit, (Symbol*)NULL );
	prod_create( pbnf, n_term, def, terminal, colonequal, n_terminal,
						(Symbol*)NULL );
	prod_create( pbnf, n_term, flag_ignore, n_terminal,
						(Symbol*)NULL );

		/* assoc */

	p = prod_create( pbnf, n_assoc, assoc_left, n_pos_terminal,
							(Symbol*)NULL );
	p->emit = "assoc_left";

	p = prod_create( pbnf, n_assoc, assoc_right, n_pos_terminal,
							(Symbol*)NULL );
	p->emit = "assoc_right";

	p = prod_create( pbnf, n_assoc, assoc_not, n_pos_terminal,
							(Symbol*)NULL );
	p->emit = "assoc_not";

		/* def */

	prod_create( pbnf, n_def, n_term, (Symbol*)NULL );
	prod_create( pbnf, n_def, n_nonterm, (Symbol*)NULL );
	prod_create( pbnf, n_def, n_assoc, (Symbol*)NULL );

	prod_create( pbnf, n_defs, n_defs, n_def, (Symbol*)NULL );
	prod_create( pbnf, n_defs, n_def, (Symbol*)NULL );

	prod_create( pbnf, n_grammar, n_defs, (Symbol*)NULL );

	/* Setup a parser */
	ppar = par_create( pbnf );
	GRAMMAR_DUMP( pbnf );

	/* Lexer */
	whitespace->ptn = pregex_ptn_create( "[ \t\r\n]+", 0 );
	comment->ptn = pregex_ptn_create( "/\\*([^*]|\\*[^/])*\\*/"
										"|//[^\n]*\n"
										"|#[^\n]*\n", 0 );

	terminal->ptn = pregex_ptn_create( "[A-Z&]\\w*", 0 );
	nonterminal->ptn = pregex_ptn_create( "[a-z_]\\w*", 0 );
	code->ptn = pregex_ptn_create( "{{.*}}", 0 );

	t_ccl->ptn = pregex_ptn_create( "\\[(\\.|[^\\\\\\]])*\\]", 0 );
	t_string->ptn = pregex_ptn_create( "'[^']*'", 0 );
	t_token->ptn = pregex_ptn_create( "\"[^\"]*\"", 0 );
	t_regex->ptn = pregex_ptn_create( "/(\\\\.|[^\\\\/])*/", 0 );

	flag_ignore->ptn = pregex_ptn_create( "%(ignore|skip)", 0 );

	/* Parse */
	if( !par_parse( &ast, ppar, src ) )
	{
		par_free( ppar );
		gram_free( pbnf );
		return FALSE;
	}

	AST_DUMP( ast );

	if( !ast_to_gram( g, ast ) )
		return FALSE;

	GRAMMAR_DUMP( g );

	par_free( ppar );
	gram_free( pbnf );
	ast_free( ast );

	return TRUE;
}


/* cc -o pbnf -I .. pbnf.c ../libphorward.a */
#if 0
int main( int argc, char** argv )
{
	Grammar*	g;
	char*	s;

	if( argc < 2 )
	{
		printf( "%s FILE-OR-GRAMMAR\n", *argv );
		return 0;
	}

	g = gram_create();

	s = argv[1];
	pfiletostr( &s, s );

	gram_from_pbnf( g, s );

	gram_prepare( g );
	gram_dump( stderr, g );

	if( s != argv[1] )
		pfree( s );

	return 0;
}
#endif
