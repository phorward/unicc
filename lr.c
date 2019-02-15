/* -MODULE----------------------------------------------------------------------
UniCC² Parser Generator
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	lr.c
Usage:	LR/LALR/SLR parse table construction and execution.
----------------------------------------------------------------------------- */

#include "unicc.h"

/* Defines */

#define DEBUGLEVEL		0

/* Closure item */
typedef struct
{
	Production*		prod;			/* Production */
	int				dot;			/* Dot offset */
	plist*			lookahead;		/* Lookahead symbols */
} LRitem;

/* LR-State */
typedef struct
{
	int				idx;			/* State index */

	plist*			kernel;			/* Kernel items */
	plist*			epsilon;		/* Empty items */

	plist*			actions;		/* Action row entries */
	plist*			gotos;			/* Goto row entries */

	Production*		def_prod;		/* Default production */

	pboolean		done;			/* Done flag */
	pboolean		closed;			/* Closed flag */
} LRstate;

/* LR-Transition */
typedef struct
{
	Symbol*			symbol;			/* Symbol */

	LRstate*		shift;			/* Shift to state */
	Production*		reduce;			/* Reduce by production */
} LRcolumn;

/* LR/LALR parser */

/* Debug for one lritem */
static void lritem_print( LRitem* it )
{
	int			i;
	Symbol*		sym;
	plistel*	e;

	if( ( !it ) )
	{
		WRONGPARAM;
		return;
	}

	fprintf( stderr, "%s : ", sym_to_str( it->prod->lhs ) );

	for( i = 0; i < plist_count( it->prod->rhs ); i++ )
	{
		sym = prod_getfromrhs( it->prod, i );

		if( i > 0 )
			fprintf( stderr, " " );

		if( i == it->dot )
			fprintf( stderr, ". " );

		fprintf( stderr, "%s", sym_to_str( sym ) );
	}

	if( i == it->dot )
	{
		fprintf( stderr, " ." );

		if( plist_count( it->lookahead ) )
		{
			fprintf( stderr, "   { " );

			plist_for( it->lookahead, e )
			{
				if( e != plist_first( it->lookahead ) )
					fprintf( stderr, " " );

				fprintf( stderr, ">%s<",
					sym_to_str( ( (Symbol*)plist_access( e ) ) ) );
			}

			fprintf( stderr, " }" );
		}
	}

	fprintf( stderr, "\n" );
}

/* Debug for an item set consisting of lritems */
static void lritems_print( plist* items, char* what )
{
	plistel*	e;

	if( ( !items ) )
		return;

	if( what && *what )
		fprintf( stderr, "%s (%d):\n", what, plist_count( items ) );

	if( !plist_count( items ) )
		fprintf( stderr, "\t(empty)\n" );

	plist_for( items, e )
	{
		if( what && *what )
			fprintf( stderr, "\t" );

		lritem_print( (LRitem*)plist_access( e ) );
	}
}

/* Priority sort function for the lookahead-sets */
static int lritem_lookahead_sort( plist* list, plistel* el, plistel* er )
{
	Symbol*		l	= (Symbol*)plist_access( el );
	Symbol*		r	= (Symbol*)plist_access( er );

	/* By idx order */
	if( l->idx < r->idx )
		return 1;

	return 0;
}

static LRitem* lritem_create( plist* list, Production* prod, int dot )
{
	LRitem*		item;

	if( !( list && prod ) )
	{
		WRONGPARAM;
		return (LRitem*)NULL;
	}

	if( dot < 0 )
		dot = 0;
	else if( dot > plist_count( prod->rhs ) )
		dot = plist_count( prod->rhs );

	item = (LRitem*)plist_malloc( list );
	item->prod = prod;
	item->dot = dot;

	item->lookahead = plist_create( 0, PLIST_MOD_PTR | PLIST_MOD_AUTOSORT );
	plist_set_sortfn( item->lookahead, lritem_lookahead_sort );

	return item;
}

static LRitem* lritem_free( LRitem* it )
{
	if( !( it ) )
		return (LRitem*)NULL;

	plist_free( it->lookahead );

	return (LRitem*)NULL;
}

static LRcolumn* lrcolumn_create(
	LRstate* st, Symbol* sym, LRstate* shift, Production* reduce )
{
	LRcolumn*	col;

	if( SYM_IS_TERMINAL( sym ) )
		col = (LRcolumn*)plist_malloc( st->actions );
	else
		col = (LRcolumn*)plist_malloc( st->gotos );

	col->symbol = sym;
	col->shift = shift;
	col->reduce = reduce;

	return col;
}

static LRstate* lrstate_create( plist* states, plist* kernel )
{
	plistel*	e;
	LRstate*	state;

	if( !( states ) )
	{
		WRONGPARAM;
		return (LRstate*)NULL;
	}

	state = plist_malloc( states );

	state->idx = plist_count( states ) - 1;
	state->kernel = plist_create( sizeof( LRitem ), PLIST_MOD_NONE );
	state->epsilon = plist_create( sizeof( LRitem ), PLIST_MOD_NONE );

	state->actions = plist_create( sizeof( LRcolumn ), PLIST_MOD_NONE );
	state->gotos = plist_create( sizeof( LRcolumn ), PLIST_MOD_NONE );

	plist_for( kernel, e )
		plist_push( state->kernel, plist_access( e ) );

	return state;
}

static plist* lr_free( plist* states )
{
	LRstate*	st;
	plistel*	e;
	plistel*	f;

	plist_for( states, e )
	{
		st = (LRstate*)plist_access( e );

		/* Parse tables */
		plist_free( st->actions );
		plist_free( st->gotos );

		/* Kernel */
		plist_for( st->kernel, f )
			lritem_free( (LRitem*)plist_access( f ) );

		/* Epsilons */
		plist_for( st->epsilon, f )
			lritem_free( (LRitem*)plist_access( f ) );
	}

	return plist_free( states );
}

static void lrstate_print( LRstate* st )
{
	plistel*	e;
	LRcolumn*	col;

	fprintf( stderr, "\n-- State %d --\n", st->idx );

	lritems_print( st->kernel, "Kernel" );
	lritems_print( st->epsilon, "Epsilon" );

	fprintf( stderr, "\n" );

	plist_for( st->actions, e )
	{
		col = (LRcolumn*)plist_access( e );

		if( col->shift && col->reduce )
			fprintf( stderr, " <- Shift/Reduce on '%s' by "
								"production '%s'\n",
						col->symbol->name,
							prod_to_str( col->reduce ) );
		else if( col->shift )
			fprintf( stderr, " -> Shift on '%s', goto state %d\n",
						col->symbol->name, col->shift->idx );
		else if( col->reduce )
			fprintf( stderr, " <- Reduce on '%s' by production '%s'\n",
						col->symbol->name,
							prod_to_str( col->reduce ) );
		else
			fprintf( stderr, " XX Error on '%s'\n", col->symbol->name );
	}

	if( st->def_prod )
		fprintf( stderr, " <- Reduce (default) on '%s'\n",
			prod_to_str( st->def_prod ) );

	plist_for( st->gotos, e )
	{
		col = (LRcolumn*)plist_access( e );

		if( col->shift && col->reduce )
			fprintf( stderr, " <- Goto/Reduce by production "
									"'%s' in '%s'\n",
						prod_to_str( col->reduce ),
							col->symbol->name );
		else if( col->shift )
			fprintf( stderr, " -> Goto state %d on '%s'\n",
						col->shift->idx,
							col->symbol->name );
		else
			MISSINGCASE;
	}
}

#if DEBUGLEVEL > 0
static void lr_print( plist* states )
{
	plistel*	e;
	LRstate*	st;

	plist_for( states, e )
	{
		st = (LRstate*)plist_access( e );
		lrstate_print( st );
	}
}
#endif

static pboolean lr_compare( plist* set1, plist* set2 )
{
	plistel*	e;
	plistel*	f;
	LRitem*		it1;
	LRitem*		it2;
	int			same	= 0;

	if( plist_count( set1 ) == plist_count( set2 ) )
	{
		plist_for( set1, e )
		{
			it1 = (LRitem*)plist_access( e );

			plist_for( set2, f )
			{
				it2 = (LRitem*)plist_access( f );

				if( it1->prod == it2->prod && it1->dot == it2->dot )
				{
					/* To become LR(1), uncomment this: */
					/* if( !plist_diff( it1->lookahead, it2->lookahead ) ) */
					same++;
				}
			}
		}

		if( plist_count( set1 ) == same )
			return TRUE;
	}

	return FALSE;
}

static LRstate* lr_get_undone( plist* states )
{
	LRstate*	st;
	plistel*	e;

	plist_for( states, e )
	{
		st = (LRstate*)plist_access( e );

		if( !st->done )
			return st;
	}

	return (LRstate*)NULL;
}

static plist* lr_closure( Grammar* gram, pboolean optimize, pboolean resolve )
{
	plist*			states;
	LRstate*		st;
	LRstate*		nst;
	LRitem*			it;
	LRitem*			kit;
	LRitem*			cit;
	Symbol*			sym;
	Symbol*			lhs;
	Production*		prod;
	LRcolumn*		col;
	LRcolumn*		ccol;
	plist*			closure;
	plist*			part;
	plistel*		e;
	plistel*		f;
	plistel*		g;
	int				i;
	int				j;
	int				cnt;
	int				prev_cnt;
	int*			prodcnt;
	pboolean		printed;

	PROC( "lr_closure" );
	PARMS( "gram", "%p", gram );
	PARMS( "optimize", "%s", BOOLEAN_STR( optimize ) );
	PARMS( "resolve", "%s", BOOLEAN_STR( resolve ) );

	if( !( gram ) )
	{
		WRONGPARAM;
		RETURN( (plist*)NULL );
	}

	MSG( "Initializing states list" );
	states = plist_create( sizeof( LRstate ), PLIST_MOD_RECYCLE );

	MSG( "Creating closure seed" );
	nst = lrstate_create( states, (plist*)NULL );
	it = lritem_create( nst->kernel, sym_getprod( gram->goal, 0 ), 0 );

	plist_push( it->lookahead, gram->eof );

	MSG( "Initializing part and closure lists" );
	part = plist_create( sizeof( LRitem ), PLIST_MOD_RECYCLE );
	closure = plist_create( sizeof( LRitem ), PLIST_MOD_RECYCLE );

	MSG( "Run the closure loop" );
	while( ( st = lr_get_undone( states ) ) )
	{
		st->done = TRUE;

		LOG( "--- Closing state %d",
				plist_offset( plist_get_by_ptr( states, st ) ) );

		MSG( "Closing state" );
		VARS( "State", "%d", plist_offset(
								plist_get_by_ptr( states, st ) ) );

		/* Close all items of the current state */
		cnt = 0;
		plist_clear( closure );

#if DEBUGLEVEL > 1
		lritems_print( st->kernel, "Kernel" );
#endif

		/* Duplicate state kernel to closure */
		MSG( "Duplicate current kernel to closure" );
		plist_for( st->kernel, e )
		{
			kit = (LRitem*)plist_access( e );

			/* Add only items that have a symbol on the right of the dot */
			if( prod_getfromrhs( kit->prod, kit->dot ) )
			{
				it = lritem_create( closure, kit->prod, kit->dot );
				it->lookahead = plist_dup( kit->lookahead );
			}
		}

		/* Close the closure! */
		MSG( "Performing closure" );
		do
		{
			prev_cnt = cnt;
			cnt = 0;

			/* Loop throught all items of the current state */
			plist_for( closure, e )
			{
				it = (LRitem*)plist_access( e );

				/* Check if symbol right to the dot is a nonterminal */
				if( !( lhs = prod_getfromrhs( it->prod, it->dot ) )
						|| SYM_IS_TERMINAL( lhs ) )
					continue;

				/* Add all prods of the nonterminal to the closure,
					if not already in */
				for( i = 0; ( prod = sym_getprod( lhs, i ) ); i++ )
				{
					plist_for( closure, f )
					{
						cit = (LRitem*)plist_access( f );
						if( cit->prod == prod && cit->dot == 0 )
							break;
					}

					if( !f )
						cit = lritem_create( closure, prod, 0 );

					/* Merge lookahead */

					/*
						Find out all lookahead symbols by merging the
						FIRST-sets of all nullable and the first
						non-nullable items on the prods right-hand
						side.
					*/
					for( j = it->dot + 1;
							( sym = prod_getfromrhs( it->prod, j ) );
								j++ )
					{
						plist_union( cit->lookahead, sym->first );

						if( !( sym->flags & FLAG_NULLABLE ) )
							break;
					}

					/*
						If all symbols right to the dot are nullable
						(or there are simply none), then add the current
						items lookahead to the closed items lookahead.
					*/
					if( !sym )
						plist_union( cit->lookahead, it->lookahead );
				}
			}

			cnt = plist_count( closure );
		}
		while( prev_cnt != cnt );
		MSG( "Closure algorithm done" );

		/* Move all epsilon closures into state's epsilon list */
		for( e = plist_first( closure ); e; )
		{
			it = (LRitem*)plist_access( e );

			if( plist_count( it->prod->rhs ) > 0 )
				e = plist_next( e );
			else
			{
				plist_for( st->epsilon, f )
				{
					kit = (LRitem*)plist_access( f );
					if( kit->prod == it->prod )
						break;
				}

				if( !f )
					plist_push( st->epsilon, it );
				else
					plist_union( kit->lookahead, it->lookahead );

				f = e;
				e = plist_next( e );
				plist_remove( closure, f );
			}
		}

		MSG( "Closure finished!" );
#if DEBUGLEVEL > 2
		lritems_print( closure, "Closure" );
#endif

		/* Create new states from the items in the closure having a symbol
			right to their dot */
		do
		{
			sym = (Symbol*)NULL;
			plist_clear( part );

			for( e = plist_first( closure ); e; )
			{
				it = (LRitem*)plist_access( e );

				/* Check if symbol right to the dot is a nonterminal */
				if( !sym && !( sym = prod_getfromrhs( it->prod, it->dot ) ) )
				{
					e = plist_next( e );
					continue;
				}

				/* Add item to new state kernel */
				if( sym == prod_getfromrhs( it->prod, it->dot ) )
				{
					it->dot++;
					plist_push( part, it );

					f = plist_prev( e );
					plist_remove( closure, e );

					if( !( e = f ) )
						e = plist_first( closure );
				}
				else
					e = plist_next( e );
			}

			/* Stop if no more partitions found (there is no first item) */
			if( !( it = (LRitem*)plist_access( plist_first( part ) ) ) )
				break;

			/*
				Can we do a shift and reduce in one transition?

				Watch for partitions that are

					x -> y .z

				where x is nonterminal, y is a possible sequence of
				terminals and/or nonterminals, or even epsilon, and z is a
				terminal or nonterminal.
			*/
			if( optimize && ( plist_count( part ) == 1
								&& !prod_getfromrhs( it->prod, it->dot ) ) )
			{
				MSG( "State optimization" );

				if( !st->closed )
					lrcolumn_create( st, sym,
							(LRstate*)it->prod, it->prod );

				/* Forget current partition; its not needed anymore... */
				plist_for( part, e )
					lritem_free( (LRitem*)plist_access( e ) );

				continue;
			}

			MSG( "Check in state pool for same kernel configuration" );
			plist_for( states, e )
			{
				nst = (LRstate*)plist_access( e );

				if( lr_compare( part, nst->kernel ) )
					break;
			}

			/* State does not already exists?
				Create it as new! */
			if( !e )
			{
				MSG( "No such state, creating new state from current config" );
#if DEBUGLEVEL > 2
				lritems_print( part, "NEW Kernel" );
#endif
				nst = lrstate_create( states, part );
			}
			else
			/* State already exists?
				Merge lookaheads (if needed). */
			{
				MSG( "There is a state with such configuration" );

				/* Merge lookahead */
				cnt = 0;
				prev_cnt = 0;

				for( e = plist_first( nst->kernel ),
						f = plist_first( part ); e;
							e = plist_next( e ), f = plist_next( f ) )
				{
					it = (LRitem*)plist_access( e );
					cit = (LRitem*)plist_access( f );

					prev_cnt += plist_count( it->lookahead );
					plist_union( it->lookahead, cit->lookahead );
					cnt += plist_count( it->lookahead );
				}

				if( cnt != prev_cnt )
					nst->done = FALSE;

#if DEBUGLEVEL > 2
				lritems_print( st->kernel, "EXT Kernel" );
#endif
			}

			if( sym && !st->closed )
				lrcolumn_create( st, sym, nst, (Production*)NULL );
		}
		while( TRUE );

		st->closed = TRUE;
		MSG( "State closed" );
	}

	plist_free( closure );
	plist_free( part );

	MSG( "Performing reductions" );

	prodcnt = (int*)pmalloc( plist_count( gram->prods ) * sizeof( int ) );

	plist_for( states, e )
	{
		printed = FALSE;
		st = (LRstate*)plist_access( e );

		LOG( "State %d", plist_offset( plist_get_by_ptr( states, st ) ) );

		/* Reductions */
		for( part = st->kernel; part;
				part = ( part == st->kernel ? st->epsilon : (plist*)NULL ) )
		{
			plist_for( part, f )
			{
				it = (LRitem*)plist_access( f );

				/* Only for items which have the dot at the end */
				if( prod_getfromrhs( it->prod, it->dot ) )
					continue;

				/* Put entries for each lookahead */
				plist_for( it->lookahead, g )
				{
					sym = (Symbol*)plist_access( g );
					lrcolumn_create( st, sym, (LRstate*)NULL, it->prod );
				}
			}
		}

		MSG( "Detect and report, or resolve conflicts" );

		for( f = plist_first( st->actions ); f; )
		{
			col = (LRcolumn*)plist_access( f );

			for( g = plist_next( f ); g; g = plist_next( g ) )
			{
				ccol = (LRcolumn*)plist_access( g );

				if( ccol->symbol == col->symbol
						&& ccol->reduce ) /* Assertion: Shift-shift entries
														are never possible! */
				{
					LOG( "State %d encounters %s/reduce-conflict on %s",
							plist_offset( e ),
								col->reduce ? "reduce" : "shift",
									col->symbol->name );

					if( resolve )
					{
						/* Try to resolve reduce-reduce conflict */
						if( col->reduce )
						{
							/* Resolve by lower production! */
							if( col->reduce->idx > ccol->reduce->idx )
							{
								MSG( "Conflict resolved in favor of "
										"lower production" );
								col->reduce = ccol->reduce;

								LOG( "Conflict resolved by lower production %d",
										ccol->reduce );
							}

							plist_remove( st->actions, g );

							/* Clear non-associative entry! */
							if( col->symbol->assoc == ASSOC_NOT )
							{
								LOG( "Symbol '%s' is non-associative, "
										"removing entry",
										sym_to_str( col->symbol ) );

								plist_remove( st->actions, f );
								f = g = (plistel*)NULL;
							}

							break; /* Restart search! */
						}
						/* Try to resolve shift-reduce conflict */
						else
						{
							/* In case there are precedences,
								resolving is possible */
							if( col->symbol->prec && ccol->reduce->prec )
							{
								/* Resolve by symbol/production precedence,
									or by associativity */
								if( col->symbol->prec < ccol->reduce->prec
									|| ( col->symbol->prec == ccol->reduce->prec
										&& col->symbol->assoc == ASSOC_LEFT )
										)
								{
									MSG( "Conflict resolved "
											"in favor of reduce" );
									col->reduce = ccol->reduce;
									col->shift = (LRstate*)NULL;
								}
								/* Clear non-associative entry! */
								else if( col->symbol->assoc == ASSOC_NOT )
								{
									LOG( "Symbol '%s' is non-associative, "
											"removing entry",
											sym_to_str( col->symbol ) );

									plist_remove( st->actions, f );
									f = g = (plistel*)NULL;
									break;
								}
								else
								{
									MSG( "Conflict resolved in favor"
											"of shift" );
								}

								plist_remove( st->actions, g );
								break;
							}
						}
					}

					/* If no resolution is possible or wanted,
						report conflict */
					if( !printed )
					{
#if DEBUGLEVEL > 0
						fprintf( stderr, "\n\n--- CONFLICTS ---\n\n" );
#endif
						lrstate_print( (LRstate*)plist_access( e ) );
						printed = TRUE;
					}

					fprintf( stderr,
						"State %d encounters %s/reduce-conflict on %s\n",
							plist_offset( e ),
								col->reduce ? "reduce" : "shift",
									col->symbol->name );
				}
			}

			if( g )
				continue;

			if( f )
				f = plist_next( f );
			else
				f = plist_first( st->actions );
		}

		#if 0
		MSG( "Detect default productions" );

		memset( prodcnt, 0, plist_count( gram->prods ) * sizeof( int ) );
		cnt = 0;

		plist_for( st->actions, f )
		{
			col = (LRcolumn*)plist_access( f );

			if( col->reduce && prodcnt[ col->reduce->idx ]++ > cnt )
			{
				cnt = prodcnt[ col->reduce->idx ];
				st->def_prod = col->reduce;
			}
		}

		/* Remove all parser actions that match the default production */
		if( st->def_prod )
		{
			for( f = plist_first( st->actions ); f; )
			{
				col = (LRcolumn*)plist_access( f );

				if( col->reduce == st->def_prod )
				{
					g = plist_next( f );
					plist_remove( st->actions, f );
					f = g;
				}
				else
					f = plist_next( f );
			}
		}
		#endif
	}

	pfree( prodcnt );

	MSG( "Finished" );
	VARS( "States generated", "%d", plist_count( states ) );

	RETURN( states );
}

/** Build parse tables */
pboolean lr_build( unsigned int* cnt, unsigned int*** dfa, Grammar* grm )
{
	plist*			states;
	unsigned int**	tab;
	LRstate*		st;
	LRcolumn*		col;
	unsigned int	total;
	int				i;
	int				j;
	plistel*		e;
	plistel*		f;

	PROC( "lr_build" );
	PARMS( "cnt", "%p", cnt );
	PARMS( "dfa", "%p", dfa );
	PARMS( "grm", "%p", grm );

	if( !grm )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	if( !( grm->flags & FLAG_FINALIZED ) )
	{
		fprintf( stderr, "Grammar is not finalized, "
			"please run gram_prepare() first!\n" );
		RETURN( FALSE );
	}

	/* Compute LALR(1) states */
	states = lr_closure( grm, TRUE, TRUE );

	#if DEBUGLEVEL > 0
	fprintf( stderr, "\n\n--- FINAL GLR STATES ---\n\n" );
	lr_print( states );
	#endif

	/* Allocate and fill return tables, free states */
	MSG( "Filling return array" );

	tab = (unsigned int**)pmalloc( plist_count( states ) * sizeof( int* ) );

	for( i = 0, e = plist_first( states ); e; e = plist_next( e ), i++ )
	{
		st = (LRstate*)plist_access( e );
		VARS( "State", "%d", i );

		total = plist_count( st->actions ) * 3
					+ plist_count( st->gotos ) * 3
						+ 2;

		tab[ i ] = (unsigned int*)pmalloc( total * sizeof( int ) );
		tab[ i ][ 0 ] = total;
		tab[ i ][ 1 ] = st->def_prod ? st->def_prod->idx + 1 : 0;

		/* Actions */
		for( j = 2, f = plist_first( st->actions );
				f; f = plist_next( f ), j += 3 )
		{
			col = (LRcolumn*)plist_access( f );

			tab[ i ][ j ] = col->symbol->idx + 1;

			if( col->shift )
				tab[ i ][ j + 1 ] = LR_SHIFT;

			if( col->reduce )
			{
				tab[ i ][ j + 1 ] |= LR_REDUCE;
				tab[ i ][ j + 2 ] = col->reduce->idx + 1;
			}
			else
				tab[ i ][ j + 2 ] = col->shift->idx + 1;
		}

		/* Gotos */
		for( f = plist_first( st->gotos );
				f; f = plist_next( f ), j += 3 )
		{
			col = (LRcolumn*)plist_access( f );

			tab[ i ][ j ] = col->symbol->idx + 1;

			if( col->shift )
				tab[ i ][ j + 1 ] = LR_SHIFT;

			if( col->reduce )
			{
				tab[ i ][ j + 1 ] |= LR_REDUCE;
				tab[ i ][ j + 2 ] = col->reduce->idx + 1;
			}
			else
				tab[ i ][ j + 2 ] = col->shift->idx + 1;
		}
	}

	total = plist_count( states );

	/* Clean-up */
	lr_free( states );

	/* Fill return pointer or print */
	if( !dfa )
	{
		/* DEBUG */
		fprintf( stderr, "count = %d\n", total );

		for( i = 0; i < plist_count( states ); i++ )
		{
			fprintf( stderr, "%02d:", i );

			fprintf( stderr, " def:%02d",  tab[ i ][ 1 ] );

			for( j = 2; j < tab[ i ][ 0 ]; j += 3 )
				fprintf( stderr, " %02d:%s%s:%02d",
					tab[ i ][ j ],
					tab[ i ][ j + 1 ] & LR_SHIFT ? "s" : "-",
					tab[ i ][ j + 1 ] & LR_REDUCE ? "r" : "-",
					tab[ i ][ j + 2 ] );

			fprintf( stderr, "\n" );
		}

		pfree( tab );
	}
	else
		*dfa = tab;

	if( cnt )
		*cnt = total;

	VARS( "states total", "%d", total );
	RETURN( TRUE );
}

