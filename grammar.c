/* -MODULE----------------------------------------------------------------------
UniCC² Parser Generator
Copyright (C) 2006-2020 by Phorward Software Technologies, Jan Max Meyer
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

	parray_init( &sym->first, sizeof( Symbol* ), 64 );
	plist_init( &sym->prods, 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );

	g->flags.finalized = FALSE;
	return sym;
}

/** Find or optionally create a derivative of symbol //origin//.

This function checks for, or creates a new symbol, as a derivation of //sym//.

If //unique// is TRUE, the function will always create a new, unique symbol.
If //unique// is FALSE, the function checks for an existing derivation of
//origin//, and only creates a new derivative if no derivation exists yet.
*/
Symbol* sym_obtain_derivative( Symbol* origin, pboolean unique )
{
	Symbol*			sym;
	static char		deriv   [ NAMELEN * 2 + 1 ];

	if( !( origin && origin->name ) )
	{
		WRONGPARAM;
		return (Symbol*)NULL;
	}

	/* fixme: Allow for dynamically sized names! */
	if( pstrlen( origin->name ) > NAMELEN )
		CORE;

	sprintf( deriv, "%s%c", origin->name, DERIVCHAR );

	/* Create unique symbol name */
	while( ( sym = sym_get_by_name( origin->grm, deriv ) )
		   && ( unique || sym->origin != origin ) )
		sprintf( deriv + strlen( deriv ), "%c", DERIVCHAR );

	if( !sym )
	{
		sym = sym_create( origin->grm, pstrdup( deriv ) );

		sym->flags.freename = TRUE;
		sym->flags.generated = TRUE;

		sym->origin = origin;
		sym->prec = origin->prec;
		sym->assoc = origin->assoc;
	}

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

	parray_erase( &sym->first );

	plist_iter_access( &sym->prods, (plistfn)prod_free );
	plist_erase( &sym->prods );

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
Production* sym_getprod( Symbol* sym, size_t n )
{
	if( !( sym ) )
	{
		WRONGPARAM;
		return (Production*)NULL;
	}

	if( SYM_IS_TERMINAL( sym ) )
		return (Production*)NULL;

	return (Production*)plist_access( plist_get( &sym->prods, n ) );
}

/** Returns the string representation of symbol //sym//.

	Nonterminals are not expanded, they are just returned as their name.
	The returned pointer is part of //sym// and can be referenced multiple
	times. It may not be freed by the caller. */
char* sym_to_str( Symbol* sym )
{
	char*	name;

	if( !sym )
	{
		WRONGPARAM;
		return "";
	}

	if( !sym->strval )
	{
		if( SYM_IS_TERMINAL( sym ) && sym->ccl )
			sym->strval = pasprintf( "[%s]", pccl_to_str( sym->ccl, TRUE ) );
		else if( SYM_IS_TERMINAL( sym ) && sym->str )
			sym->strval = pasprintf( "%s%s%s",
							sym->emit == sym->str ? "\"" : "'",
								sym->str, sym->emit == sym->str ? "\"" : "'" );
		else if( SYM_IS_TERMINAL( sym ) && sym->ptn )
			sym->strval = pasprintf( "/%s/", pregex_ptn_to_regex( sym->ptn ) );
		else if( ( name = sym->strval = sym->name ) && !sym->grm->flags.debug )
		{
			if( sym->flags.generated && !sym->origin )
			{
				int			i;
				plistel*	e;
				Production*	prod;
				Symbol*		item;

				name = pstrdup( "(" );

				for( i = 0; ( prod = sym_getprod(
								( sym->origin ? sym->origin : sym ), i ) ); i++ )
				{
					if( i > 0 )
						name = pstrcatstr( name, " | ", FALSE );

					plist_for( prod->rhs, e )
					{
						item = (Symbol*)plist_access( e );

						if( e != plist_first( prod->rhs ) )
							name = pstrcatstr( name, " ", FALSE );


						name = pstrcatstr( name, sym_to_str( item->origin ? item->origin : item ), FALSE );
					}
				}

				name = pstrcatstr( name, ")", FALSE );
			}

			if( strncmp( sym->name, "pos_", 4 ) == 0 )
				sym->strval = pasprintf( "%s+", name == sym->name ? name + 4 : name );
			else if( strncmp( sym->name, "opt_", 4 ) == 0 )
				sym->strval = pasprintf( "%s?", name == sym->name ? name + 4 : name  );
			else if( strncmp( sym->name, "kle_", 4 ) == 0 )
				sym->strval = pasprintf( "%s*", name == sym->name ? name + 4 : name  );
			else
				sym->strval = name;

			if( sym->strval != name && name != sym->name )
				pfree( name );
		}
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
		ret->flags.nameless = TRUE;
		ret->flags.generated = TRUE;
		ret->origin = sym->origin ? sym->origin : sym;
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
		ret->flags.nameless = TRUE;
		ret->flags.generated = TRUE;
		ret->origin = sym->origin ? sym->origin : sym;
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
		ret->flags.nameless = TRUE;
		ret->flags.generated = TRUE;
		ret->origin = sym->origin ? sym->origin : sym;
		ret->flags.freename = name == buf;

		prod_create( sym->grm, ret, sym_mod_positive( sym ), (Symbol*)NULL );
		prod_create( sym->grm, ret, (Symbol*)NULL );

		sym = ret;
	}

	RETURN( ret );
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
	plist_push( &lhs->prods, prod );

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

	if( plist_get_by_ptr( &p->lhs->prods, p ) )
		plist_remove( &p->lhs->prods, plist_get_by_ptr( &p->lhs->prods, p ) );

	plist_remove( p->grm->prods, plist_get_by_ptr( p->grm->prods, p ) );

	return (Production*)NULL;
}

/** Get the //n//th production from grammar //g//.
Returns (Production*)NULL if no symbol was found. */
Production* prod_get( Grammar* g, size_t n )
{
	if( !( g ) )
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
	sym->usages++;

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

		if( sym->usages )
			sym->usages--;
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
		return -1;
	else if( !l->flags.special && r->flags.special )
		return 1;
	else if( l->ccl && !r->ccl )
		return -1;
	else if( !l->ccl && r->ccl )
		return 1;
	else if( l->str && !r->str )
		return -1;
	else if( !l->str && r->str )
		return 1;
	else if( l->ptn && !r->ptn )
		return -1;
	else if( !l->ptn && r->ptn )
		return 1;

	if( l->idx < r->idx )
		return -1;

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
	g->eof->flags.terminal = TRUE;
	g->eof->flags.special = TRUE;
	g->eof->flags.nameless = TRUE;

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
	plistel*		h;
	Production*		prod;
	Production*		cprod;
	Symbol*			sym;
	pboolean		nullable;
	pboolean		changes;
	plist			call;
	plist			done;
	unsigned int	idx;
#if TIMEMEASURE
	clock_t			start;
	size_t			count	= 0;
#endif

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

	/* -------------------------------------------------------------------------
		Set consecutive index IDs for symbols and productions
	------------------------------------------------------------------------- */

	/* Set IDs for symbols */
	plist_sort( g->symbols );

	for( idx = 0, e = plist_first( g->symbols ); e; e = plist_next( e ), idx++ )
	{
		sym = (Symbol*)plist_access( e );
		sym->idx = idx;

		if( SYM_IS_TERMINAL( sym ) )
		{
			if( !parray_count( &sym->first ) )
				parray_push( &sym->first, &sym );
		}
		else
			parray_erase( &sym->first );
	}

	/* Set IDs for productions */
	for( idx = 0, e = plist_first( g->prods ); e; e = plist_next( e ), idx++ )
	{
		prod = (Production*)plist_access( e );
		prod->idx = idx;
	}

	/* -------------------------------------------------------------------------
		Compute FIRST sets and mark left-recursions
	------------------------------------------------------------------------- */

	plist_init( &call, 0, PLIST_MOD_RECYCLE );
	plist_init( &done, 0, PLIST_MOD_RECYCLE );

#if TIMEMEASURE
	start = clock();
#endif

	do
	{
		changes = FALSE;

		plist_for( g->prods, e )
		{
			cprod = (Production*)plist_access( e );
			plist_push( &call, cprod );

			while( plist_pop( &call, &prod ) )
			{
				plist_push( &done, prod );

				plist_for( prod->rhs, f )
				{
					sym = (Symbol*)plist_access( f );

					nullable = FALSE;

					/* Union first set */
					if( parray_union( &cprod->lhs->first, &sym->first ) )
						changes = TRUE;
#if TIMEMEASURE
					count++;
#endif

					if( !SYM_IS_TERMINAL( sym ) )
					{
						/* Put prods on stack */
						plist_for( &sym->prods, h )
						{
							prod = (Production*)plist_access( h );

							if( plist_count( prod->rhs ) == 0 )
							{
								nullable = TRUE;
								continue;
							}

							if( prod == cprod )
								prod->flags.leftrec =
									prod->lhs->flags.leftrec = TRUE;
							else if( !plist_get_by_ptr( &done, prod ) )
								plist_push( &call, prod );
						}
					}

					if( !nullable )
						break;
				}

				/* Flag nullable */
				if( !f )
					cprod->flags.nullable = cprod->lhs->flags.nullable = TRUE;
			}

			plist_erase( &done );
		}
	}
	while( changes );

#if TIMEMEASURE
	fprintf( stderr, "FIRST %ld %f\n", count, (double)(clock() - start) / CLOCKS_PER_SEC );
#endif

	/* Erase lists, re-use list items */
	plist_erase( &call );
	plist_erase( &done );

	/* -------------------------------------------------------------------------
		Pull-through lexeme symbol configurations to superior non-terminals
	------------------------------------------------------------------------- */

	/* Push all non-terminals configured as lexeme onto call stack */
	plist_for( g->symbols, e )
	{
		sym = (Symbol*)plist_access( e );

		if( SYM_IS_TERMINAL( sym ) || !sym->flags.lexem )
			continue;

		plist_push( &call, sym );
	}

	/* Pull through lexeme configurations to superior non-terminals */
	while( plist_pop( &call, &sym ) )
	{
		plist_push( &done, sym );

		plist_for( &sym->prods, e )
		{
			prod = (Production*)plist_access( e );

			plist_for( prod->rhs, f )
			{
				sym = (Symbol*)plist_access( f );
				sym->flags.lexem = TRUE;

				if( !SYM_IS_TERMINAL( sym ) )
				{
					if( !plist_get_by_ptr( &done, sym )
						&& !plist_get_by_ptr( &call, sym ) )
						plist_push( &call, sym );
				}
			}
		}
	}

	/* Clear all lists */
	plist_clear( &call );
	plist_clear( &done );

	/* -------------------------------------------------------------------------
		Inherit precedences & emits
	------------------------------------------------------------------------- */
	do
	{
		changes = FALSE;

		plist_for( g->prods, e )
		{
			prod = (Production*)plist_access( e );

			/* Mark productions's left-hand side as emitting when either
				the production or the nonterminal emits something */
			if( ( prod->emit || prod->lhs->emit ) && !prod->lhs->flags.emits )
			{
				prod->lhs->flags.emits = TRUE;
				changes = TRUE;
			}

			/* Pass left-hand side precedence to productions */
			if( prod->prec < prod->lhs->prec )
			{
				prod->prec = prod->lhs->prec;
				changes = TRUE;
			}

			/* Inherit production's precedence from rightmost symbol */
			for( f = plist_last( prod->rhs ); f; f = plist_prev( f ) )
			{
				sym = (Symbol*)plist_access( f );
				if( sym->prec > prod->prec )
				{
					prod->prec = sym->prec;
					changes = TRUE;
					break;
				}
			}
		}
	}
	while( changes );

	/* Set finalized */
	g->flags.finalized = TRUE;
	g->strval = pfree( g->strval );

	RETURN( TRUE );
}


/** Transform grammar's whitespace symbol handling to make it parseable
using a scannerless parser. */
pboolean gram_transform_to_scannerless( Grammar* g )
{
	Symbol		*	ws		= (Symbol*)NULL,
				*	ws_pos,
				*	ws_kle,
				*	sym,
				*	nsym;

	Production	*	p;

	plist		stack,
				done,
				rewritten;

	plistel		*	e,
				*	f;

	PROC( "gram_transform_to_scannerless" );

	if( !g )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	/* Create productions for all whitespaces */
	plist_for( g->symbols, e )
	{
		sym = (Symbol*)plist_access( e );

		if( sym->flags.whitespace )
		{
			if( !ws )
			{
				ws = sym_create( g, SYM_T_WHITESPACE );

				ws->flags.special = TRUE;
				ws->flags.lexem = TRUE;
				ws->flags.generated = TRUE;
			}

			p = prod_create( g, ws, sym, NULL );
		}
	}

	if( !ws )
		RETURN( FALSE );

	ws->flags.whitespace = TRUE;

	ws_kle = sym_mod_kleene( ws );
	ws_kle->flags.lexem = TRUE;
	ws_kle->flags.whitespace = TRUE;

	ws_pos = sym_mod_positive( ws );
	ws_pos->flags.lexem = TRUE;
	ws_pos->flags.whitespace = TRUE;

	/*
		Find out all lexeme non-terminals and those
		which belong to them.
	*/
	plist_init( &done, 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );
	plist_init( &stack, 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );
	plist_init( &rewritten, 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );

	plist_for( g->symbols, e )
	{
		sym = (Symbol*)plist_access( e );

		if( sym->flags.lexem && !SYM_IS_TERMINAL( sym ) )
		{
			plist_push( &done, sym );
			plist_push( &stack, sym );
		}
	}

	while( plist_pop( &stack, &sym ) )
	{
		plist_for( &sym->prods, e )
		{
			p = (Production*)plist_access( e );

			plist_for( p->rhs, f )
			{
				sym = (Symbol*)plist_access( f );

				if( !SYM_IS_TERMINAL( sym ) )
				{
					if( !plist_get_by_ptr( &done, sym ) )
					{
						sym->flags.lexem = TRUE;

						plist_push( &done, sym );
						plist_push( &stack, sym );
					}
				}
			}
		}
	}

	/*
		Find all non-terminals from goal; If there is a call to a
		lexem non-terminal or a terminal, rewrite their rules and
		replace them.
	*/
	if( !( g->goal->flags.lexem ) )
	{
		plist_clear( &done );
		plist_clear( &stack );

		plist_push( &done, g->goal );
		plist_push( &stack, g->goal );

		while( plist_pop( &stack, &sym ) )
		{
			plist_for( &sym->prods, e )
			{
				p = (Production*)plist_access( e );

				/* Don't rewrite a production twice! */
				if( plist_get_by_ptr( &rewritten, p ) )
					continue;

				plist_for( p->rhs, f )
				{
					sym = (Symbol*)plist_access( f );

					if( !SYM_IS_TERMINAL( sym ) && !( sym->flags.lexem ) )
					{
						if( !plist_get_by_ptr( &done, sym ) )
						{
							plist_push( &done, sym );
							plist_push( &stack, sym );
						}
					}
					else if( ( !SYM_IS_TERMINAL( sym )
								&& sym->flags.lexem )
							 || SYM_IS_TERMINAL( sym ) )
					{
						/* Do not rewrite special symbols! */
						if( sym->flags.special )
							continue;

						nsym = sym_obtain_derivative( sym, FALSE );

						/* If you already found a symbol, don't do anything! */
						if( !sym_getprod( nsym, 0 ) )
						{
							prod_create( g, nsym, sym, ws_kle, NULL );

							//fixme?
							//nsym->keyword = sym->keyword;
							//nsym->vtype = sym->vtype;
						}

						/* Replace the rewritten symbol with the
						  		production's symbol! */
						memcpy( f + 1, &nsym, sizeof( Symbol* ) );
					}
				}

				/* Mark this production as already rewritten! */
				plist_push( &rewritten, p );
			}
		}
	}

	plist_erase( &done );
	plist_erase( &rewritten );
	plist_erase( &stack );

	/* Add whitespace behind goal symbol */
	sym = sym_obtain_derivative( g->goal, TRUE );
	prod_create( g, sym, g->goal, ws_kle, NULL );
	g->goal = sym;

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
	Symbol*		l;
	int			maxlhslen	= 0;
	int			maxemitlen	= 0;
	int			maxsymlen	= 0;

	if( !_dbg_trace_enabled( file, function, "GRAMMAR" ) )
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

			fprintf( stderr, "%s",
						sym_to_str( s ) ? sym_to_str( s ) : "(null)" );
		}

		fprintf( stderr, "\n" );
	}

	fprintf( stderr, "\n" );

	plist_for( g->symbols, e )
	{
		s = (Symbol*)plist_access( e );

		fprintf( stderr, "%c%03d  %-*s  %-*s",
			SYM_IS_TERMINAL( s ) ? 'T' : 'N',
			s->idx, maxemitlen, s->emit ? s->emit : "",
				maxsymlen, s->name );

		if( !SYM_IS_TERMINAL( s ) && parray_count( &s->first ) )
		{
			fprintf( stderr, " {" );

			parray_for( &s->first, l )
			{
				fprintf( stderr, " " );
				fprintf( stderr, "%s", sym_to_str( *( (Symbol**)l ) ) );
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
		if( ( !grm->flags.debug && sym->flags.nameless ) )
			continue;

		if( pstrlen( sym_to_str( sym ) ) > maxsymlen )
			maxsymlen = pstrlen( sym_to_str( sym ) );
		else if( !sym->name && maxsymlen < 4 )
			maxsymlen = 4;
	}

	if( maxsymlen > NAMELEN )
		maxsymlen = NAMELEN;

	/* Generate terminals */
	plist_for( grm->symbols, e )
	{
		if( !SYM_IS_TERMINAL( ( sym = (Symbol*)plist_access( e ) ) )
				|| sym->flags.nameless )
			continue;

		if( sym->ptn )
		    sprintf( name, "@ %-*s  : /%s/\n",
		            maxsymlen, sym_to_str( sym ),
		                pregex_ptn_to_regex( sym->ptn ) );
		else
            sprintf( name, "@ %-*s  : '%s'\n",
                     maxsymlen, sym_to_str( sym ),
                         pstrget( sym->name ) );

		grm->strval = pstrcatstr( grm->strval, name, FALSE );
	}

	/* Generate nonterminals! */
	plist_for( grm->symbols, e )
	{
		if( SYM_IS_TERMINAL( ( sym = (Symbol*)plist_access( e ) ) )
				|| ( !grm->flags.debug && sym->flags.nameless ) )
			continue;

		first = TRUE;
		sprintf( name, "@ %-*s%s : ", maxsymlen,
										sym->name,
											sym == grm->goal ? "$" : " " );

		grm->strval = pstrcatstr( grm->strval, name, FALSE );

		plist_for( grm->prods, f )
		{
			prod = (Production*)plist_access( f );

			if( prod->lhs != sym )
				continue;

			if( !first )
			{
				sprintf( name, "  %-*s  | ", maxsymlen, "" );
				grm->strval = pstrcatstr( grm->strval, name, FALSE );
			}
			else
				first = FALSE;

			if( !plist_first( prod->rhs ) )
				grm->strval = pstrcatchar( grm->strval, '\n' );

			plist_for( prod->rhs, g )
			{
				psym = (Symbol*)plist_access( g );

				sprintf( name, "%s%s", sym_to_str( psym ),
					g == plist_last( prod->rhs ) ? "\n" : " ");
				grm->strval = pstrcatstr( grm->strval, name, FALSE );
			}
		}

		grm->strval = pstrcatstr( grm->strval, "\n", FALSE );
	}

	return grm->strval;
}

/** Dump grammar in a JSON format to //stream//. */
pboolean gram_dump_json( FILE* stream, Grammar* grm, char* prefix )
{
	plistel*		e;
	plistel*		f;
	Symbol*			sym;
	Production*		prod;
	char*			ptr;

	PROC( "gram_dump_json" );
	PARMS( "stream", "%p", stream );
	PARMS( "grm", "%p", grm );
	PARMS( "prefix", "%s", prefix );

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

	fprintf( stream, "%s{\n\t\"symbols\": [", prefix );

	/* Symbols */
	plist_for( grm->symbols, e )
	{
		sym = (Symbol*)plist_access( e );

		fprintf( stream, "\n%s\t\t{\n%s\t\t\t\"symbol\": %d, \n",
			prefix, prefix, sym->idx );

		fprintf( stream, "%s\t\t\t\"type\": \"%s\"",
			prefix,
			SYM_IS_TERMINAL( sym ) ? "terminal" : "non-terminal" );

		if( sym->name && *sym->name )
		{
			fprintf( stream, ",\n%s\t\t\t\"name\": \"", prefix );

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
			fprintf( stream, ",\n%s\t\t\t\"emit\": \"", prefix );

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
			fprintf( stream, ",\n%s\t\t\t\"regexp\": \"", prefix );

			for( ptr = pregex_ptn_to_regex( sym->ptn ); *ptr; ptr++ )
				if( *ptr == '\"' )
					fprintf( stream, "\\\"" );
				else if( *ptr == '\\' )
					fprintf( stream, "\\\\" );
				else
					fputc( *ptr, stream );

			fprintf( stream, "\"" );
		}

		fprintf( stream, "\n%s\t\t}%s", prefix, plist_next( e ) ? ", " : "" );
	}

	fprintf( stream, "\n%s\t],\n%s\t\"productions\": [", prefix, prefix );

	/* Productions */
	plist_for( grm->prods, e )
	{
		prod = (Production*)plist_access( e );

		fprintf( stream,
			"\n%s\t\t{"
			"\n%s\t\t\t\"production\": %d,"
			"\n%s\t\t\t\"lhs\": %d,",
			prefix, prefix, prod->idx, prefix, prod->lhs->idx );

		if( prod->emit && *prod->emit && prod->emit != prod->lhs->emit )
		{
			fprintf( stream, "\n%s\t\t\t\"emit\": \"", prefix );

			for( ptr = prod->emit; *ptr; ptr++ )
				if( *ptr == '\"' )
					fprintf( stream, "\\\"" );
				else if( *ptr == '\\' )
					fprintf( stream, "\\\\" );
				else
					fputc( *ptr, stream );

			fprintf( stream, "\"," );
		}

		fprintf( stream, "\n%s\t\t\t\"rhs\": [", prefix );

		plist_for( prod->rhs, f )
		{
			sym = (Symbol*)plist_access( f );

			fprintf( stream, "\n%s\t\t\t\t%d%s", prefix, sym->idx,
						plist_next( f ) ? "," : "" );
		}

		fprintf( stream, "\n%s\t\t\t]\n%s\t\t}%s",
			prefix, prefix, plist_next( e ) ? "," : "" );
	}

	fprintf( stream, "\n%s\t]\n%s}", prefix, prefix );

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
