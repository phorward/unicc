/* -MODULE----------------------------------------------------------------------
UniCC² Parser Generator
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	grammar.c
Usage:	Abstraction of grammars into particular objects.
----------------------------------------------------------------------------- */

#include "unicc.h"

/* -----------------------------------------------------------------------------
	Symbols
----------------------------------------------------------------------------- */

/** Check for a symbol type, whether it is configured to be a terminal or a
nonterminal symbol.

Returns TRUE in case //sym// is a terminal symbol, and FALSE otherwise.
*/
/*MACRO:SYM_IS_TERMINAL( Symbol* sym )*/

/** Creates a new symbol in the grammar //g//.

	//name// is the unique name for the symbol. It can be left empty,
	configuring the symbol as an unnamed symbol.
*/
Symbol* sym_create( Grammar* g, char* name )
{
	Symbol*	sym;

	if( !( g ) )
	{
		WRONGPARAM;
		return (Symbol*)NULL;
	}

	if( g->flags.frozen )
	{
		fprintf( stderr, "Grammar is frozen, can't create symbols\n" );
		return (Symbol*)NULL;
	}

	if( name && sym_get_by_name( g, name ) )
	{
		fprintf( stderr, "Symbol '%s' already exists in this grammar\n", name );
		return (Symbol*)NULL;
	}

	/* Insert into symbol table */
	if( !( sym = (Symbol*)plist_access(
			plist_insert(
					g->symbols, (plistel*)NULL,
					name, (void*)NULL ) ) ) )
		return (Symbol*)NULL;

	sym->grm = g;
	sym->idx = plist_count( g->symbols ) - 1;

	if( !( sym->name = name ) )
		sym->flags.nameless = TRUE;

	sym->first = plist_create( 0, PLIST_MOD_PTR );

	g->flags.finalized = FALSE;
	return sym;
}

/** Frees a symbol. */
Symbol* sym_free( Symbol* sym )
{
	if( !sym )
		return (Symbol*)NULL;

	sym->grm->flags.finalized = FALSE;

	if( sym->flags.freeemit )
		pfree( sym->emit );

	/* Remove symbol from pool */
	plist_remove( sym->grm->symbols,
				  plist_get_by_ptr( sym->grm->symbols, sym ) );

	if( sym->flags.freename )
		pfree( sym->name );

	if( sym->ptn )
		pregex_ptn_free( sym->ptn );

	plist_free( sym->first );

	return (Symbol*)NULL;
}

/** Removes a symbol from its grammar. */
Symbol* sym_drop( Symbol* sym )
{
	plistel* 		e;
	Production*		p;

	if( !sym )
		return (Symbol*)NULL;

	if( sym->grm->flags.frozen )
	{
		fprintf( stderr, "Grammar is frozen, can't delete symbol\n" );
		return sym;
	}

	/* Remove all references from other productions */
	for( e = plist_first( sym->grm->prods ); e; )
	{
		p = (Production*)plist_access( e );
		e = plist_next( e );

		if( p->lhs == sym )
		{
			prod_free( p );
			continue;
		}

		prod_remove( p, sym );
	}

	/* Clear yourself */
	return sym_free( sym );
}

/** Get the //n//th symbol from grammar //g//.
Returns (Symbol*)NULL if no symbol was found. */
Symbol* sym_get( Grammar* g, unsigned int n )
{
	if( !( g ) )
	{
		WRONGPARAM;
		return (Symbol*)NULL;
	}

	return (Symbol*)plist_access( plist_get( g->symbols, n ) );
}

/** Get a symbol from grammar //g// by its //name//. */
Symbol* sym_get_by_name( Grammar* g, char* name )
{
	if( !( g && name && *name ) )
	{
		WRONGPARAM;
		return (Symbol*)NULL;
	}

	return (Symbol*)plist_access( plist_get_by_key( g->symbols, name ) );
}

/** Find a nameless terminal symbol by its pattern. */
Symbol* sym_get_nameless_term_by_def( Grammar* g, char* name )
{
	int		i;
	Symbol*	sym;

	if( !( g && name && *name ) )
	{
		WRONGPARAM;
		return (Symbol*)NULL;
	}

	for( i = 0; ( sym = sym_get( g, i ) ); i++ )
	{
		if( !SYM_IS_TERMINAL( sym ) || !sym->flags.nameless )
			continue;

		if( sym->name && strcmp( sym->name, name ) == 0 )
			return sym;
	}

	return (Symbol*)NULL;
}

/** Get the //n//th production from symbol //sym//.
//sym// must be a nonterminal.

Returns (Production*)NULL if the production is not found or the symbol is
configured differently. */
Production* sym_getprod( Symbol* sym, unsigned int n )
{
	plistel*	e;
	Production*		p;

	if( !( sym ) )
	{
		WRONGPARAM;
		return (Production*)NULL;
	}

	if( SYM_IS_TERMINAL( sym ) )
	{
		fprintf( stderr, "%s, %d: Symbol '%s' is not a non-terminal symbol\n",
				 __FILE__, __LINE__, sym_to_str( sym ) );
		return (Production*)NULL;
	}

	plist_for( sym->grm->prods, e )
	{
		p = (Production*)plist_access( e );
		if( p->lhs == sym && !n-- )
			return p;
	}

	return (Production*)NULL;
}

/** Returns the string representation of symbol //sym//.

	Nonterminals are not expanded, they are just returned as their name.
	The returned pointer is part of //sym// and can be referenced multiple
	times. It may not be freed by the caller. */
char* sym_to_str( Symbol* sym )
{
	if( !sym )
	{
		WRONGPARAM;
		return "";
	}

	if( !sym->strval )
	{
		if( sym->name && !sym->flags.nameless && !sym->flags.generated )
			sym->strval = sym->name;
		else if( SYM_IS_TERMINAL( sym ) && sym->ccl )
			sym->strval = pasprintf( "[%s]", pccl_to_str( sym->ccl, TRUE ) );
		else if( SYM_IS_TERMINAL( sym ) && sym->str )
			sym->strval = pasprintf( "%s", sym->str );
		else if( SYM_IS_TERMINAL( sym ) && sym->ptn )
			sym->strval = pasprintf( "/%s/", pregex_ptn_to_regex( sym->ptn ) );
		else if( strncmp( sym->name, "pos_", 4 ) == 0 )
			sym->strval = pasprintf( "%s+", sym->name + 4 );
		else if( strncmp( sym->name, "opt_", 4 ) == 0 )
			sym->strval = pasprintf( "%s?", sym->name + 4 );
		else if( strncmp( sym->name, "kle_", 4 ) == 0 )
			sym->strval = pasprintf( "%s*", sym->name + 4 );
	}

	return sym->strval;
}

/** Constructs a positive closure from //sym//.

This helper function constructs the productions and symbol
```
@pos_sym: sym pos_sym | sym
```
from //sym//, and returns the symbol //pos_sym//. If //pos_sym// already exists,
the symbol will be returned without any creation.
*/
Symbol* sym_mod_positive( Symbol* sym )
{
	Symbol*		ret;
	char		buf		[ BUFSIZ + 1 ];
	char*		name	= buf;
	size_t		len;

	PROC( "sym_mod_positive" );
	PARMS( "sym", "%p", sym );

	if( !sym )
	{
		WRONGPARAM;
		RETURN( (Symbol*)NULL );
	}

	len = pstrlen( sym->name ) + 4;
	if( len > BUFSIZ )
		name = (char*)pmalloc( ( len + 1 ) * sizeof( char ) );

	sprintf( name, "pos_%s", pstrget( sym->name ) );

	VARS( "name", "%s", name );

	if( !( ret = sym_get_by_name( sym->grm, name ) ) )
	{
		MSG( "Symbol does not exist yet, creating" );

		ret = sym_create( sym->grm, name == buf ? pstrdup( name ) : name );
		ret->flags.defined = TRUE;
		ret->flags.called = TRUE;
		ret->flags.generated = TRUE;
		ret->flags.freename = name == buf;

		if( sym->grm->flags.preventlrec )
			prod_create( sym->grm, ret, sym, ret, (Symbol*)NULL );
		else
			prod_create( sym->grm, ret, ret, sym, (Symbol*)NULL );

		prod_create( sym->grm, ret, sym, (Symbol*)NULL );
	}
	else if( name != buf )
		pfree( name );

	RETURN( ret );
}

/** Constructs an optional closure from //sym//.

This helper function constructs the productions and symbol
```
@opt_sym: sym |
```
from //sym//, and returns the symbol //pos_sym//. If //opt_sym// already exists,
the symbol will be returned without any creation.
*/
Symbol* sym_mod_optional( Symbol* sym )
{
	Symbol*		ret;
	char		buf		[ BUFSIZ + 1 ];
	char*		name	= buf;
	size_t		len;

	PROC( "sym_mod_optional" );
	PARMS( "sym", "%p", sym );

	if( !sym )
	{
		WRONGPARAM;
		RETURN( (Symbol*)NULL );
	}

	len = pstrlen( sym->name ) + 4;
	if( len > BUFSIZ )
		name = (char*)pmalloc( ( len + 1 ) * sizeof( char ) );

	sprintf( name, "opt_%s", pstrget( sym->name ) );

	VARS( "name", "%s", name );

	if( !( ret = sym_get_by_name( sym->grm, name ) ) )
	{
		MSG( "Symbol does not exist yet, creating" );

		ret = sym_create( sym->grm, name == buf ? pstrdup( name ) : name );
		ret->flags.defined = TRUE;
		ret->flags.called = TRUE;
		ret->flags.generated = TRUE;
		ret->flags.freename = name == buf;

		prod_create( sym->grm, ret, sym, (Symbol*)NULL );
		prod_create( sym->grm, ret, (Symbol*)NULL );
	}
	else if( name != buf )
		pfree( name );

	RETURN( ret );
}

/** Constructs an optional positive ("kleene") closure from //sym//.

This helper function constructs the productions and symbol
```
@pos_sym: sym pos_sym | sym
@kle_sym: pos_sym |
```
from //sym//, and returns the symbol //opt_sym//. If any of the given symbols
already exists, they are directly used. The function is a shortcut for a call
```
sym_mod_optional( sym_mod_positive( sym ) )
```
*/
Symbol* sym_mod_kleene( Symbol* sym )
{
	Symbol*		ret;
	char		buf		[ BUFSIZ + 1 ];
	char*		name	= buf;
	size_t		len;

	PROC( "sym_mod_kleene" );
	PARMS( "sym", "%p", sym );

	if( !sym )
	{
		WRONGPARAM;
		RETURN( (Symbol*)NULL );
	}

	len = pstrlen( sym->name ) + 4;
	if( len > BUFSIZ )
		name = (char*)pmalloc( ( len + 1 ) * sizeof( char ) );

	sprintf( name, "kle_%s", pstrget( sym->name ) );

	VARS( "name", "%s", name );

	if( !( ret = sym_get_by_name( sym->grm, name ) ) )
	{
		MSG( "Symbol does not exist yet, creating" );

		ret = sym_create( sym->grm, name == buf ? pstrdup( name ) : name );
		ret->flags.defined = TRUE;
		ret->flags.called = TRUE;
		ret->flags.generated = TRUE;
		ret->flags.freename = name == buf;

		prod_create( sym->grm, ret, sym_mod_positive( sym ), (Symbol*)NULL );
		prod_create( sym->grm, ret, (Symbol*)NULL );
	}

	RETURN( sym );
}


/* -----------------------------------------------------------------------------
	Productions
----------------------------------------------------------------------------- */

/** Creates a new production on left-hand-side //lhs//
	within the grammar //g//. */
Production* prod_create( Grammar* g, Symbol* lhs, ... )
{
	Production*	prod;
	Symbol*	sym;
	va_list	varg;

	if( !( g && lhs ) )
	{
		WRONGPARAM;
		return (Production*)NULL;
	}

	if( g->flags.frozen )
	{
		fprintf( stderr, "Grammar is frozen, can't create productions\n" );
		return (Production*)NULL;
	}

	if( SYM_IS_TERMINAL( lhs ) )
	{
		fprintf( stderr, "%s, %d: Can't create a production; "
						 "'%s' is not a non-terminal symbol\n",
				 __FILE__, __LINE__, sym_to_str( lhs ) );
		return (Production*)NULL;
	}

	prod = (Production*)plist_malloc( g->prods );

	prod->grm = g;

	prod->lhs = lhs;
	prod->rhs = plist_create( 0, PLIST_MOD_PTR );

	va_start( varg, lhs );

	while( ( sym = va_arg( varg, Symbol* ) ) )
		prod_append( prod, sym );

	va_end( varg );

	g->flags.finalized = FALSE;

	return prod;
}

/** Frees the production object //p// and releases any used memory. */
Production* prod_free( Production* p )
{
	if( !p )
		return (Production*)NULL;


	if( p->grm->flags.frozen )
	{
		fprintf( stderr, "Grammar is frozen, can't delete production\n" );
		return p;
	}

	p->grm->flags.finalized = FALSE;

	if( p->flags.freeemit )
		pfree( p->emit );

	pfree( p->strval );

	plist_free( p->rhs );

	plist_remove( p->grm->prods, plist_get_by_ptr( p->grm->prods, p ) );

	return (Production*)NULL;
}

/** Get the //n//th production from grammar //g//.
Returns (Production*)NULL if no symbol was found. */
Production* prod_get( Grammar* g, int n )
{
	if( !( g && n >= 0 ) )
	{
		WRONGPARAM;
		return (Production*)NULL;
	}

	return (Production*)plist_access( plist_get( g->prods, n ) );
}

/** Appends the symbol //sym// to the right-hand-side of production //p//. */
pboolean prod_append( Production* p, Symbol* sym )
{
	if( !( p && sym ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	if( p->grm->flags.frozen )
	{
		fprintf( stderr, "Grammar is frozen, can't add items to production\n" );
		return FALSE;
	}

	plist_push( p->rhs, sym );

	p->grm->flags.finalized = FALSE;
	pfree( p->strval );

	return TRUE;
}

/** Removes all occurrences of symbol //sym// from the right-hand-side of
	production //p//. */
int prod_remove( Production* p, Symbol* sym )
{
	int			cnt 	= 0;
	plistel*	e;

	if( !( p && sym ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	if( p->grm->flags.frozen )
	{
		fprintf( stderr, "Grammar is frozen, can't delete symbol\n" );
		return -1;
	}

	p->grm->flags.finalized = FALSE;

	while( ( e = plist_get_by_ptr( p->rhs, sym ) ) )
	{
		plist_remove( p->rhs, e );
		cnt++;
	}

	return cnt;
}

/** Returns the //off//s element from the right-hand-side of
	production //p//. Returns (Symbol*)NULL if the requested element does
	not exist. */
Symbol* prod_getfromrhs( Production* p, int off )
{
	if( !( p ) )
	{
		WRONGPARAM;
		return (Symbol*)NULL;
	}

	return (Symbol*)plist_access( plist_get( p->rhs, off ) );
}

/** Returns the string representation of production //p//.

	The returned pointer is part of //p// and can be referenced multiple times.
	It may not be freed by the caller. */
char* prod_to_str( Production* p )
{
	plistel*	e;
	Symbol*		sym;

	if( !p )
	{
		WRONGPARAM;
		return "";
	}

	if( !p->strval )
	{
		if( p->lhs )
			p->strval = pstrcatstr( p->strval, sym_to_str( p->lhs ), FALSE );

		p->strval = pstrcatstr( p->strval, " : ", FALSE );
		plist_for( p->rhs, e )
		{
			sym = (Symbol*)plist_access( e );

			if( e != plist_first( p->rhs ) )
				p->strval = pstrcatstr( p->strval, " ", FALSE );

			p->strval = pstrcatstr( p->strval, sym_to_str( sym ), FALSE );
		}
	}

	return p->strval;
}


/* -----------------------------------------------------------------------------
	Grammar
----------------------------------------------------------------------------- */

static int sort_symbols( plist* lst, plistel* el, plistel* er )
{
	Symbol*		l	= (Symbol*)plist_access( el );
	Symbol*		r	= (Symbol*)plist_access( er );

	if( l->flags.special && !r->flags.special )
		return 1;
	else if( !l->flags.special && r->flags.special )
		return -1;
	else if( l->ccl && !r->ccl )
		return 1;
	else if( !l->ccl && r->ccl )
		return -1;
	else if( l->str && !r->str )
		return 1;
	else if( !l->str && r->str )
		return -1;
	else if( l->ptn && !r->ptn )
		return 1;
	else if( !l->ptn && r->ptn )
		return -1;

	if( l->idx < r->idx )
		return 1;

	return 0;
}

/** Creates a new Grammar-object. */
Grammar* gram_create( void )
{
	Grammar*		g;

	g = (Grammar*)pmalloc( sizeof( Grammar ) );

	g->symbols = plist_create( sizeof( Symbol ),
					PLIST_MOD_RECYCLE | PLIST_MOD_EXTKEYS
						| PLIST_MOD_UNIQUE );
	plist_set_sortfn( g->symbols, sort_symbols );

	g->prods = plist_create( sizeof( Production ), PLIST_MOD_RECYCLE );

	g->eof = sym_create( g, SYM_T_EOF );
	g->eof->flags.special = TRUE;

	return g;
}


/** Prepares the grammar //g// by computing all necessary stuff required for
runtime and parser generator.

The preparation process includes:
- Setting up final symbol and production IDs
- Nonterminals FIRST-set computation
- Marking of left-recursions
- The 'lexem'-flag pull-through the grammar.
-

This function is only run internally.
Don't call it if you're unsure ;)... */
pboolean gram_prepare( Grammar* g )
{
	plistel*		e;
	plistel*		f;
	Production*		prod;
	Production*		cprod;
	Symbol*			sym;
	pboolean		nullable;
	plist*			call;
	plist*			done;
	int				i;
	int				cnt;
	int				pcnt;
	unsigned int	idx;

	PROC( "gram_prepare" );

	if( !g )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	if( !g->goal )
	{
		/* No such goal! */
		fprintf( stderr, "Grammar has no goal!\n" );
		RETURN( FALSE );
	}

	/* Reset symbols */
	plist_sort( g->symbols );

	for( idx = 0, e = plist_first( g->symbols ); e; e = plist_next( e ), idx++ )
	{
		sym = (Symbol*)plist_access( e );
		sym->idx = idx;

		if( SYM_IS_TERMINAL( sym ) )
		{
			if( !plist_first( sym->first ) )
				plist_push( sym->first, sym );
		}
		else
			plist_erase( sym->first );
	}

	/* Reset productions */
	for( idx = 0, e = plist_first( g->prods ); e; e = plist_next( e ), idx++ )
	{
		prod = (Production*)plist_access( e );
		prod->idx = idx;
	}

	/* Compute FIRST sets and mark left-recursions */
	cnt = 0;
	call = plist_create( 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );
	done = plist_create( 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );

	do
	{
		pcnt = cnt;
		cnt = 0;

		plist_for( g->prods, e )
		{
			cprod = (Production*)plist_access( e );
			plist_push( call, cprod );

			while( plist_pop( call, &prod ) )
			{
				plist_push( done, prod );

				plist_for( prod->rhs, f )
				{
					sym = (Symbol*)plist_access( f );

					nullable = FALSE;

					if( !SYM_IS_TERMINAL( sym ) )
					{
						/* Union first set */
						plist_union( cprod->lhs->first, sym->first );

						/* Put prods on stack */
						for( i = 0; ( prod = sym_getprod( sym, i ) ); i++ )
						{
							if( plist_count( prod->rhs ) == 0 )
							{
								nullable = TRUE;
								continue;
							}

							if( prod == cprod )
								prod->flags.leftrec =
									prod->lhs->flags.leftrec = TRUE;
							else if( !plist_get_by_ptr( done, prod ) )
								plist_push( call, prod );
						}
					}
					/* Extend first set if required */
					else if( !plist_get_by_ptr( cprod->lhs->first, sym ) )
						plist_push( cprod->lhs->first, sym );

					if( !nullable )
						break;
				}

				/* Flag nullable */
				if( !f )
					cprod->flags.nullable = cprod->lhs->flags.nullable = TRUE;

				cnt += plist_count( cprod->lhs->first );
			}

			plist_erase( done );
		}
	}
	while( pcnt < cnt );

	plist_clear( call );
	plist_clear( done );

	/* Pull-through all lexem symbols */
	plist_for( g->symbols, e )
	{
		sym = (Symbol*)plist_access( e );

		if( SYM_IS_TERMINAL( sym ) || !sym->flags.lexem )
			continue;

		plist_push( call, sym );
	}

	while( plist_pop( call, &sym ) )
	{
		plist_push( done, sym );

		for( i = 0; ( prod = sym_getprod( sym, i ) ); i++ )
		{
			plist_for( prod->rhs, f )
			{
				sym = (Symbol*)plist_access( f );
				sym->flags.lexem = TRUE;

				if( !SYM_IS_TERMINAL( sym ) )
				{
					if( !plist_get_by_ptr( done, sym )
						&& !plist_get_by_ptr( call, sym ) )
						plist_push( call, sym );
				}
			}
		}
	}

	/* Clear all lists */
	plist_free( call );
	plist_free( done );

	/* Inherit precedences */
	plist_for( g->prods, e )
	{
		prod = (Production*)plist_access( e );

		if( prod->prec < prod->lhs->prec )
			prod->prec = prod->lhs->prec;

		for( f = plist_last( prod->rhs ); f; f = plist_prev( f ) )
		{
			sym = (Symbol*)plist_access( f );
			if( sym->prec > prod->prec )
			{
				prod->prec = sym->prec;
				break;
			}
		}
	}

	/* Set finalized */
	g->flags.finalized = TRUE;
	g->strval = pfree( g->strval );

	RETURN( TRUE );
}

/** Dump grammar to trace.

The GRAMMAR_DUMP-macro is used to dump all relevant contents of a Grammar object
into the program trace, for debugging purposes.

GRAMMAR_DUMP() can only be used when the function was trace-enabled by PROC()
before.
*/
/*MACRO:GRAMMAR_DUMP( Grammar* g )*/
void __dbg_gram_dump( char* file, int line, char* function,
						char* name, Grammar* g )
{
	plistel*	e;
	plistel*	f;
	Production*	p;
	Symbol*		s;
	int			maxlhslen	= 0;
	int			maxemitlen	= 0;
	int			maxsymlen	= 0;

	if( !_dbg_trace_enabled( file, function ) )
		return;

	_dbg_trace( file, line, "GRAMMAR", function, "%s = {", name );

	plist_for( g->symbols, e )
	{
		s = (Symbol*)plist_access( e );

		if( pstrlen( s->emit ) > maxemitlen )
			maxemitlen = pstrlen( s->emit );

		if( !SYM_IS_TERMINAL( s ) && pstrlen( s->name ) > maxlhslen )
			maxlhslen = pstrlen( s->name );

		if( pstrlen( s->name ) > maxsymlen )
			maxsymlen = pstrlen( s->name );
	}

	plist_for( g->prods, e )
	{
		p = (Production*)plist_access( e );
		fprintf( stderr,
			"%s%s%s%s%s%s%-*s %-*s : ",
			g->goal == p->lhs ? "$" : " ",
			p->flags.leftrec ? "L" : " ",
			p->flags.nullable ? "N" : " ",
			p->lhs->flags.lexem ? "X" : " ",
			p->lhs->flags.whitespace ? "W" : " ",
			p->emit ? "@" : " ",
			maxemitlen, p->emit ? p->emit : "",
			maxlhslen, p->lhs->name );

		plist_for( p->rhs, f )
		{
			s = (Symbol*)plist_access( f );

			if( f != plist_first( p->rhs ) )
				fprintf( stderr, " " );

			fprintf( stderr, "%s", sym_to_str( s ) );
		}

		fprintf( stderr, "\n" );
	}

	fprintf( stderr, "\n" );

	plist_for( g->symbols, e )
	{
		s = (Symbol*)plist_access( e );

		fprintf( stderr, "%03d  %-*s  %-*s",
			s->idx, maxemitlen, s->emit ? s->emit : "",
				maxsymlen, s->name );

		if( !SYM_IS_TERMINAL( s ) && plist_count( s->first ) )
		{
			fprintf( stderr, " {" );

			plist_for( s->first, f )
			{
				s = (Symbol*)plist_access( f );
				fprintf( stderr, " " );
				fprintf( stderr, "%s", sym_to_str( s ) );
			}

			fprintf( stderr, " }" );
		}
		else if( SYM_IS_TERMINAL( s ) && s->ptn )
			fprintf( stderr, " /%s/", pregex_ptn_to_regex( s->ptn ) );

		fprintf( stderr, "\n" );
	}

	_dbg_trace( file, line, "GRAMMAR", function, "}" );
}

/** Get grammar representation as BNF */
char* gram_to_bnf( Grammar* grm )
{
	Symbol*		sym;
	Symbol*		psym;
	Production*	prod;
	int			maxsymlen	= 0;
	plistel*	e;
	plistel*	f;
	plistel*	g;
	char		name		[ NAMELEN + 3 + 1 ];
	pboolean	first;

	if( !grm )
	{
		WRONGPARAM;
		return "";
	}

	if( !grm->flags.finalized )
		gram_prepare( grm );

	if( grm->strval )
		return grm->strval;

	/* Find longest symbol */
	plist_for( grm->symbols, e )
	{
		sym = (Symbol*)plist_access( e );

		if( pstrlen( sym->name ) > maxsymlen )
			maxsymlen = pstrlen( sym->name );
		else if( !sym->name && maxsymlen < 4 )
			maxsymlen = 4;
	}

	if( maxsymlen > NAMELEN )
		maxsymlen = NAMELEN;

	/* Generate terminals */
	plist_for( grm->symbols, e )
	{
		if( !SYM_IS_TERMINAL( ( sym = (Symbol*)plist_access( e ) ) )
			|| sym->flags.nameless || sym->flags.special )
			continue;

		if( sym->ptn )
		    sprintf( name, "@%-*s : /%s/\n",
		            maxsymlen, sym_to_str( sym ),
		                pregex_ptn_to_regex( sym->ptn ) );
		else
            sprintf( name, "@%-*s : '%s'\n",
                     maxsymlen, sym_to_str( sym ),
                         pstrget( sym->name ) );

		grm->strval = pstrcatstr( grm->strval, name, FALSE );
	}

	/* Generate nonterminals! */
	plist_for( grm->symbols, e )
	{
		if( SYM_IS_TERMINAL( ( sym = (Symbol*)plist_access( e ) ) )
				|| sym->flags.generated )
			continue;

		first = TRUE;
		sprintf( name, "@%-*s : ", maxsymlen, sym->name );

		grm->strval = pstrcatstr( grm->strval, name, FALSE );

		plist_for( grm->prods, f )
		{
			prod = (Production*)plist_access( f );

			if( prod->lhs != sym )
				continue;

			if( !first )
			{
				sprintf( name, "%-*s  | ", maxsymlen, "" );
				grm->strval = pstrcatstr( grm->strval, name, FALSE );
			}
			else
				first = FALSE;

			if( !plist_first( prod->rhs ) )
				grm->strval = pstrcatchar( grm->strval, '\n' );

			plist_for( prod->rhs, g )
			{
				psym = (Symbol*)plist_access( g );

				sprintf( name, "%.*s%s", maxsymlen, sym_to_str( psym ),
					g == plist_last( prod->rhs ) ? "\n" : " ");
				grm->strval = pstrcatstr( grm->strval, name, FALSE );
			}
		}

		grm->strval = pstrcatstr( grm->strval, "\n", FALSE );
	}

	return grm->strval;
}

/** Dump grammar in a JSON format to //stream//. */
pboolean gram_dump_json( FILE* stream, Grammar* grm )
{
	plistel*		e;
	plistel*		f;
	Symbol*			sym;
	Production*		prod;
	char*			ptr;

	PROC( "gram_dump_json" );
	PARMS( "stream", "%p", stream );
	PARMS( "grm", "%p", grm );

	if( !grm )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	if( !stream )
		stream = stdout;

	if( !grm->flags.finalized )
	{
		fprintf( stderr, "Grammar is not finalized, "
			"please run gram_prepare() first!\n" );
		RETURN( FALSE );
	}

	fprintf( stream, "{ \"symbols\": [" );

	/* Symbols */
	plist_for( grm->symbols, e )
	{
		sym = (Symbol*)plist_access( e );

		fprintf( stream, "{ \"id\": %d, ", sym->idx );

		fprintf( stream, "\"type\": \"%s\"",
			SYM_IS_TERMINAL( sym ) ? "terminal" : "non-terminal" );

		if( sym->name && *sym->name )
		{
			fprintf( stream, ", \"name\": \"" );

			for( ptr = sym->name; *ptr; ptr++ )
				if( *ptr == '\"' )
					fprintf( stream, "\\\"" );
				else if( *ptr == '\\' )
					fprintf( stream, "\\\\" );
				else
					fputc( *ptr, stream );

			fprintf( stream, "\"" );
		}

		if( sym->emit && *sym->emit )
		{
			fprintf( stream, ", \"emit\": \"" );

			for( ptr = sym->emit; *ptr; ptr++ )
				if( *ptr == '\"' )
					fprintf( stream, "\\\"" );
				else if( *ptr == '\\' )
					fprintf( stream, "\\\\" );
				else
					fputc( *ptr, stream );

			fprintf( stream, "\"" );
		}

		if( SYM_IS_TERMINAL( sym ) && sym->ptn )
		{
			fprintf( stream, ", \"regexp\": \"" );

			for( ptr = pregex_ptn_to_regex( sym->ptn ); *ptr; ptr++ )
				if( *ptr == '\"' )
					fprintf( stream, "\\\"" );
				else if( *ptr == '\\' )
					fprintf( stream, "\\\\" );
				else
					fputc( *ptr, stream );

			fprintf( stream, "\"" );
		}

		fprintf( stream, "}%s", plist_next( e ) ? ", " : "" );
	}

	fprintf( stream, "], \"productions\": [" );

	/* Productions */
	plist_for( grm->prods, e )
	{
		prod = (Production*)plist_access( e );

		fprintf( stream, "{\"id\": %d, \"lhs\": %d, \"rhs\": [",
			prod->idx, prod->lhs->idx );

		plist_for( prod->rhs, f )
		{
			sym = (Symbol*)plist_access( f );

			fprintf( stream, "%d%s", sym->idx, plist_next( f ) ? ", " : "" );
		}

		fprintf( stream, "]}%s", plist_next( e ) ? ", " : "" );
	}

	fprintf( stream, "]}" );

	RETURN( TRUE );
}


/** Frees grammar //g// and all its related memory. */
Grammar* gram_free( Grammar* g )
{
	if( !g )
		return (Grammar*)NULL;

	g->flags.frozen = FALSE;

	/* Erase symbols */
	while( plist_count( g->symbols ) )
		sym_free( (Symbol*)plist_access( plist_first( g->symbols ) ) );

	/* Erase productions */
	while( plist_count( g->prods ) )
		prod_free( (Production*)plist_access( plist_first( g->prods ) ) );

	plist_free( g->symbols );
	plist_free( g->prods );

	pfree( g->strval );

	pfree( g );

	return (Grammar*)NULL;
}
