/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
https://phorward.info ++ unicc<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	lalr.c
Author:	Jan Max Meyer
Usage:	Performs the LALR(1) parse table construction algorithm
----------------------------------------------------------------------------- */

#include "unicc.h"

#define ON_ALGORITHM_DEBUG 0

/** Performs a test on two kernel item sets if they are equal.

//kernel1// is the The first kernel item set.
//kernel2// is the The second kernel item set.

Returns a int TRUE if the item sets are equal, else FALSE. */
static int test_same_kernel( LIST* kernel1, LIST* kernel2 )
{
	int		ret			= FALSE;
	LIST*	checklist	= (LIST*)NULL;
	LIST*	i			= (LIST*)NULL;
	LIST*	j			= (LIST*)NULL;
	ITEM*	item1		= (ITEM*)NULL;
	ITEM*	item2		= (ITEM*)NULL;

	if( list_count( kernel1 ) == list_count( kernel2 ) )
	{
		for( i = kernel1; i; i = i->next )
		{
			item1 = i->pptr;

			for( j = kernel2; j; j = j->next )
			{
				item2 = j->pptr;

				if( item1->prod == item2->prod
					&& item1->dot_offset == item2->dot_offset
						&& item1->next_symbol == item2->next_symbol )
				{
					checklist = list_push( checklist, j->pptr );
				}
			}
		}

		if( list_count( kernel1 ) == list_count( checklist ) )
			ret = TRUE;

		list_free( checklist );
	}

	return ret;
}

/** This is the key function which performs the major closure from one kernel
item seed to a closure set.

//productions// is the list of all available productions.
//it// is the item to be closed.
//closure_set// is the pointer to the closure set list which can possibly
be enhanced.
*/
static void close_item( plist* productions, ITEM* it, LIST** closure_set )
{
	LIST*		l		= (LIST*)NULL;
	plistel*	e;
	plistel*	f;
	ITEM*		cit		= (ITEM*)NULL;
	PROD*		prod	= (PROD*)NULL;
	plist*		first	= (plist*)NULL;

	/* Only perform closure if the symbol right to the dot
		of the current kernel item is a non-terminal */
	if( it->next_symbol )
	{
		if( it->next_symbol->type == SYM_NON_TERMINAL )
		{
			/* Find all right-hand sides of this non-terminal */
			plist_for( productions, f )
			{
				prod = (PROD*)plist_access( f );

				if( prod->lhs == it->next_symbol )
				{
					/* Check if there is not already such an item
						that uses this production! */
					for( l = *closure_set; l; l = l->next )
					{
						cit = l->pptr;
						if( cit->prod == prod )
							break;
					}

					/* Add new item! */
					if( !l )
					{
#if ON_ALGORITHM_DEBUG
						fprintf( stderr, "\n===> Closure: Creating new "
											"item\n");
						dump_item_set( (FILE*)NULL, "Partial closure:",
							*closure_set );
#endif
						cit = create_item( prod );

						*closure_set = list_push( *closure_set, cit );
					}
#if ON_ALGORITHM_DEBUG
					else
					{
						fprintf( stderr, "\n===> Closure: Using existing "
											"item\n");
						dump_item_set( (FILE*)NULL, "Partial closure:",
							*closure_set );
					}
#endif

#if ON_ALGORITHM_DEBUG
						fprintf( stderr, "\n===> Closure: cit->prod %d "
											"it->prod %d\n",
								cit->prod->id, it->prod->id );
#endif

					/* --- Passing the lookaheads ... --- */

					/* If this is the last symbol... */
					if( plist_count( it->prod->rhs ) > 0 )
					{
#if ON_ALGORITHM_DEBUG
						fprintf( stderr, "\n===> Closure: dot %d, "
											"rhs %d len = %d \n",
								it->dot_offset, it->prod->id,
										plist_count( it->prod->rhs ) );
#endif

						if( !( e = plist_get( it->prod->rhs,
												it->dot_offset + 1 ) ) )
						{
							plist_union( &cit->lookahead, &it->lookahead );
#if ON_ALGORITHM_DEBUG
							fprintf( stderr, "\n===> Closure: Dot at the end, "
												"taking lookahead \n");
							dump_item_set( (FILE*)NULL, "Partial closure:",
								*closure_set );
#endif
						}
						else
						{
							if( !first )
								first = plist_create( 0,
											PLIST_MOD_PTR | PLIST_MOD_RECYCLE );
							else
								plist_erase( first );

							if( seek_rhs_first( first, e ) )
								plist_union( first, &it->lookahead );

							plist_union( &cit->lookahead, first );

#if ON_ALGORITHM_DEBUG
							fprintf( stderr, "\n===> Closure: "
												"Calculated lookahead\n");
							dump_item_set( (FILE*)NULL, "Partial closure:",
								*closure_set );
#endif
						}
					}
				}
			}
		}
	}

	plist_free( first );
}

/** Drops and frees a list of items.

//list// is the item list.

Returns LIST*(NULL) always.
*/
static LIST* drop_item_list( LIST* list )
{
	LIST*		l;
	ITEM*		it;

	for( l = list; l; l = list_next( l ) )
	{
		it = (ITEM*)list_access( l );

		free_item( it );
	}

	list_free( list );

	return (LIST*)NULL;
}

/** Performs an LR(1) closure and merges the lookahead-symbols of items with the
same right-hand side, dot position, and lookahead-subset, making it a LALR(1)
closure.

//parser// is the pointer to the parser information structure.
//st// is the state to be closed. */
static void lalr1_closure( PARSER* parser, int state_id )
{
	STATE*		st;
	LIST*		closure_start;
	LIST*		closure_set			= (LIST*)NULL;
	LIST*		i					= (LIST*)NULL;
	LIST*		j					= (LIST*)NULL;
	LIST*		k					= (LIST*)NULL;
	ITEM*		it					= (ITEM*)NULL;
	ITEM*		cit					= (ITEM*)NULL;
	SYMBOL*		sym_before_move		= (SYMBOL*)NULL;
	STATE*		nstate				= (STATE*)NULL;

	LIST*		part_symbols		= (LIST*)NULL;
	LIST*		partitions			= (LIST*)NULL;

	int			prev_cnt			= 0;
	int			cnt					= 0;

	if( !( st = parray_get( parser->states, state_id ) ) )
	{
		WRONGPARAM;
		return;
	}

	closure_start = st->kernel;

	/*
		03.03.2008	Jan Max Meyer
		Added new SHIFT_REDUCE-transition to build lesser states
		(up to 30% lesser states!)
	*/

#if ON_ALGORITHM_DEBUG
	fprintf( stderr, "================\n");
	fprintf( stderr, "=== State % 2d ===\n", st->state_id );
	fprintf( stderr, "================\n");
	dump_item_set( (FILE*)NULL, "Kernel:", st->kernel );
	dump_item_set( (FILE*)NULL, "Epsilon:", st->epsilon );
#endif

	/*
		Performing the closure:

		First, closure is done on the kernel item set,
		all following closures are done on the closure-set
		resulting from the kernel closure.

		The closure is finished when no more items are added
		to closure_set.
	*/
	do
	{
		prev_cnt = cnt;
		cnt = 0;

		/* Iterating trough the kernel items */
		for( i = closure_start; i; i = i->next )
		{
			it = i->pptr;
			close_item( parser->productions, it, &closure_set );
		}

		closure_start = closure_set;
		cnt = list_count( closure_set );

		/* fprintf( stderr, "prev_cnt = %d, cnt = %d\n", prev_cnt, cnt ); */
	}
	while( prev_cnt != cnt );

	/*dump_item_set( (FILE*)NULL, "Closure:", closure_set );*/

	/*
		Adding all kernel items with outgoing transitions
		to the closure item set now! These are all items
		where next_symbol is not (SYMBOL*)NULL...
	*/
	for( i = st->kernel; i; i = i->next )
	{
		it = i->pptr;

		if( it->next_symbol != (SYMBOL*)NULL )
		{
			/*
				The items must be really copied:
				The complete memory must be mirrored and re-allocated to
				create a single, independend item!
			*/
			cit = create_item( it->prod );

			cit->prod = it->prod;
			cit->dot_offset = it->dot_offset;
			cit->next_symbol = it->next_symbol;

			plist_concat( &cit->lookahead, &it->lookahead );

			closure_set = list_push( closure_set, cit );
		}
	}

	/*
		Moving all epsilon items (items with an epsilon production!)
		to the epsilon item set of this state!
	*/
	for( i = closure_set; i; )
	{
		it = i->pptr;
		if( !plist_count( it->prod->rhs ) )
		{
			/* For all items with the same epsilon transitions,
				merge the lookaheads! */
			for( j = st->epsilon; j; j = j->next )
			{
				cit = j->pptr;
				if( cit->prod == it->prod )
					break;
			}

			if( !j )
			{
				st->epsilon = list_push( st->epsilon, it );
			}
			else
			{
				plist_union( &cit->lookahead, &it->lookahead );
				free_item( it );
			}

			i = i->next;
			closure_set = list_remove( closure_set, it );
		}
		else
		{
			i = i->next;
		}
	}

#if 0
	fprintf( stderr, "\n--- State %d ---\n", st->state_id );
	dump_item_set( (FILE*)NULL, "Kernel:", st->kernel );
	dump_item_set( (FILE*)NULL, "Closure:", closure_set );
	dump_item_set( (FILE*)NULL, "Epsilon:", st->epsilon );
#endif

	/*
		Sorting the closure set by the symbols next to the dot.
	*/
	do
	{
		/* cnt will act as an "I had done something"-flag in this case! */
		cnt = 0;
		for( i = closure_set; i; i = i->next )
		{
			it = i->pptr;
			if( i->next )
				cit = i->next->pptr;
			else
				cit = (ITEM*)NULL;

			if( it && cit )
			{
				if( it->next_symbol && cit->next_symbol )
				{
					if( it->next_symbol > cit->next_symbol
						|| ( it->next_symbol == cit->next_symbol
							&& it->prod->id > cit->prod->id ) )
					{
						cnt = 1;
						i->pptr = cit;
						i->next->pptr = it;
					}
				}
			}
		}
	}
	while( cnt > 0 );

#if ON_ALGORITHM_DEBUG
	fprintf( stderr, "\n--- State %d ---\n", st->state_id );
	dump_item_set( (FILE*)NULL, "Kernel:", st->kernel );
	dump_item_set( (FILE*)NULL, "Closure:", closure_set );
	dump_item_set( (FILE*)NULL, "Epsilon:", st->epsilon );
#endif

	/*
		Partitioning all items with the same symbol right to the dot
		(all items that share the symbol where next_symbol points to...)
	*/
	for( i = closure_set; i; i = i->next )
	{
		it = i->pptr;

		if( it->next_symbol != (SYMBOL*)NULL )
		{
			if( ( cnt = list_find( part_symbols, it->next_symbol ) ) == -1 )
			{
				part_symbols = list_push( part_symbols, it->next_symbol );
				partitions = list_push( partitions,
					list_push( (LIST*)NULL, it ) );
			}
			else
			{
				for( j = partitions; j && cnt > 0; j = j->next, cnt-- )
						;
				j->pptr = list_push( (LIST*)(j->pptr), it );
			}
		}
	}


	/*
		Creating new states from the partitions
	*/
	for( i = partitions; i; i = i->next )
	{
		sym_before_move = (SYMBOL*)NULL;

		/* Move the dot in this partition one to the right! */
		for( j = i->pptr; j; j = j->next )
		{
			it = j->pptr;

			/* Remember the symbol to the right of the dot
				before the dot is moved...*/
			if( sym_before_move == (SYMBOL*)NULL )
				sym_before_move = it->next_symbol;

			if( it->dot_offset < plist_count( it->prod->rhs ) )
			{
				it->dot_offset++;
				it->next_symbol = (SYMBOL*)plist_access(
									plist_get( it->prod->rhs,
													it->dot_offset ) );
			}
		}

		/*
			Jan Max Meyer, 03.03.2008
			SHIFT_REDUCE-feature added, as in min_lalr1

			Jan Max Meyer, 28.05.2008
			Improved, not only for terminals, even for nonterminals :)

			Watch for partitions that are

			x -> y .z

			where x is nonterminal, y is a possible sequence of terminals and/or
			nonterminals or even epsilon, and z is a terminal or nonterminal.
		*/
		if( parser->optimize_states
				/* && ( IS_TERMINAL( sym_before_move ) & SYM_TERMINAL ) */
					&& list_count( (LIST*)(i->pptr) ) == 1
						&& it->next_symbol == (SYMBOL*)NULL )
		{
#if 0
	fprintf( stderr, "\nAdding SHIFT_REDUCE entry\n", st->state_id );
	dump_item_set( (FILE*)NULL, "Partition:", (LIST*)(i->pptr) );
#endif
			/*
				Add a shift-reduce entry
			*/
			if( !( st->closed ) )
			{
				if( IS_TERMINAL( sym_before_move ) )
				{
					st->actions = list_push( st->actions, create_tabcol(
						sym_before_move, SHIFT_REDUCE,
							it->prod->id, (ITEM*)NULL ) );
				}
				else
				{
					st->gotos = list_push( st->gotos, create_tabcol(
						sym_before_move, SHIFT_REDUCE,
							it->prod->id, (ITEM*)NULL ) );
				}
			}

			drop_item_list( (LIST*)( i->pptr ) );
		}
		else
		{
			/*
				Proceed normally
			*/
			parray_for( parser->states, nstate )
				if( test_same_kernel( nstate->kernel, i->pptr ) )
					break;

			if( !nstate )
			{
				nstate = create_state( parser );
				nstate->kernel = i->pptr;
				nstate->derived_from = state_id;

				/* Re-get current state due possibly heap re-allocation */
				st = (STATE*)parray_get( parser->states, state_id );

#if ON_ALGORITHM_DEBUG
				fprintf( stderr, "\n===> Creating new State %d...\n",
					nstate->state_id );
				dump_item_set( (FILE*)NULL, "Kernel:", nstate->kernel );
#endif
			}
			else
			{
#if ON_ALGORITHM_DEBUG
				fprintf( stderr, "\n===> Updating existing State %d...\n",
					nstate->state_id );
				dump_item_set( (FILE*)NULL, "Kernel:", nstate->kernel );
				fprintf( stderr, "\n...from partition set...\n" );
				dump_item_set( (FILE*)NULL, "Partition:", i->pptr );
#endif

				/* Merging the lookaheads */
				cnt = 0;
				prev_cnt = 0;

#if 0
	if( nstate->state_id == 96 && st->state_id == 260 )
	{
		fprintf( stderr, "\n--- NEW/UPDATE STATE %d from STATE %d ---\n",
			nstate->state_id, st->state_id );
		dump_item_set( (FILE*)NULL, "Partition:", i->pptr );
		dump_item_set( (FILE*)NULL, "Kernel:", nstate->kernel );
		getchar();
	}
#endif

				for( j = nstate->kernel, k = i->pptr; j;
						j = j->next, k = k->next )
				{
					it = j->pptr;
					prev_cnt += plist_count( &it->lookahead );

					plist_union( &it->lookahead, &((ITEM*)(k->pptr))->lookahead );

					cnt += plist_count( &it->lookahead );

					free_item( k->pptr );
				}

				/* Had new lookaheads been added? */
				if( cnt > prev_cnt )
					nstate->done = FALSE;

#if ON_ALGORITHM_DEBUG
				fprintf( stderr, "\n...it's now...\n" );
				dump_item_set( (FILE*)NULL, "Kernel:", nstate->kernel );
#endif

				/* drop_item_list( (LIST*)( i->pptr ) ); */
				list_free( (LIST*)( i->pptr ) );
			}

			/* Performing some table creation */
			if( !( st->closed ) )
			{
				if( IS_TERMINAL( sym_before_move ) )
				{
					st->actions = list_push( st->actions, create_tabcol(
						sym_before_move, SHIFT,
							nstate->state_id, (ITEM*)NULL ) );
				}
				else
				{
					st->gotos = list_push( st->gotos, create_tabcol(
						sym_before_move, SHIFT,
							nstate->state_id, (ITEM*)NULL ) );
				}
			}
		}
	}

	st->closed = 1;

	list_free( closure_set );
	list_free( part_symbols );
	list_free( partitions );

#if ON_ALGORITHM_DEBUG
	fprintf( stderr, "\n\n" );
#endif

	/*
	if( debug )
		printf( "\n" );
	*/
}


/** Performs reduction entries into the parse-table and determines shift-reduce
or reduce-reduce conflicts. The reduction-entries can only be added when all
states are closed completely, so this operations must be called as the last
step on creating the parse-tables.

//parser// is the pointer to parser structure.
//st// is the state pointer, defining the state where reduce-entries should be
created for.
//it// is the item where the reduce-entries should be created for. */
static void reduce_item( PARSER* parser, STATE* st, ITEM* it )
{
	plistel* 	e;
	SYMBOL*		sym		= (SYMBOL*)NULL;
	TABCOL*		act		= (TABCOL*)NULL;
	int			resolved;

	/*
	02.03.2011	Jan Max Meyer
	In case of non-associative symbols, write a special error action into the
	parse table. The parser template also had to be changed for this. Due the
	default-production feature, the old way on removing the table entry did not
	work anymore.
	*/

	if( it->next_symbol == (SYMBOL*)NULL )
	{
		plist_for( &it->lookahead, e )
		{
			sym = (SYMBOL*)plist_access( e );

			/*
				Check out if there is already an action!
			*/
			if( ( act = find_tabcol( st->actions, sym ) ) == (TABCOL*)NULL )
			{
				st->actions = list_push( st->actions,
					create_tabcol( sym, REDUCE, it->prod->id, it ) );
			}
			else
			{
				if( act->action == REDUCE )
				{
					if( ( ( !( parser->all_warnings ) &&
							!( it->prod->lhs->whitespace )
							&& !( it->prod->lhs->generated ) )
								|| parser->all_warnings ) )
					{
						print_error( parser, ERR_REDUCE_REDUCE,
							ERRSTYLE_WARNING | ERRSTYLE_STATEINFO
								| ERRSTYLE_SYMBOL, st, sym );

						if( act->index > it->prod->id )
						{
							act->index = it->prod->id;
							act->symbol = sym;
							act->derived_from = it;
						}
					}

					if( sym->assoc == ASSOC_NOASSOC )
					{
						st->actions = list_remove( st->actions,
										(void*)act );
						free_tabcol( act );
					}
				}
				else if( act->action & SHIFT )
				{
					/*
					 * Supress some warnings:
					 * Always shift on "lexem separation" or "fixate"
					 */
					if( ( parser->p_lexem_sep && it->prod->lhs->lexem )
							|| it->prod->lhs->fixated )
						continue;

					if( ( resolved = ( it->prod->prec && sym->prec ) ) )
					{
						if( sym->prec < it->prod->prec ||
							( sym->prec == it->prod->prec
							&& sym->assoc == ASSOC_LEFT ) )
						{
							act->action = REDUCE;
							act->index = it->prod->id;
							act->symbol = sym;
							act->derived_from = it;
						}
						else if( sym->prec == it->prod->prec
							&& sym->assoc == ASSOC_NOASSOC )
						{
							/* 02.03.2011 JMM: Let parser run into an error! */
							act->action = ERROR;
							act->index = 0;
						}
					}

					if( !resolved && ( ( !( parser->all_warnings )
						&& !( it->prod->lhs->whitespace )
							&& !( it->prod->lhs->generated ) )
								|| parser->all_warnings ) )
					{
						print_error( parser, ERR_SHIFT_REDUCE,
							ERRSTYLE_WARNING |
								ERRSTYLE_STATEINFO | ERRSTYLE_SYMBOL,
									st, sym );
					}
				}
			}
		}
	}
}


/** This is the entry function for performing the table reduction entry
creation. This must be called as the last step on computing the parse tables.

//parser// is the pointer to parser structure
//st// is the state pointer, defining the state where reduce-entries should be
created for. */
static void perform_reductions( PARSER* parser, STATE* st )
{
	LIST*	i;

	/* First, perform the reductions */
	for( i = st->kernel; i; i = i->next )
		reduce_item( parser, st, i->pptr );

	for( i = st->epsilon; i; i = i->next )
		reduce_item( parser, st, i->pptr );
}

/** This is the entry function for generating the LALR(1) parse tables for a
parsed grammar definition.

//parser// is the pointer to the parser information structure. */
void generate_tables( PARSER* parser )
{
	STATE*	st		= (STATE*)NULL;
	ITEM*	it		= (ITEM*)NULL;

	if( !( parser->symbols || parser->productions ) )
		return;

	if( !( parser->goal ) )
	{
		print_error( parser, ERR_NO_GOAL_SYMBOL, ERRSTYLE_FATAL );
		return;
	}

	st = create_state( parser );
	it = create_item( (PROD*)plist_access( plist_first(
							parser->goal->productions ) ) );
	st->kernel = list_push( st->kernel, it );

	/* The goal item's lookahead is the end_of_input symbol */
	plist_push( &it->lookahead, parser->end_of_input );

	/* Perform closure algorithm until no more undone states are found */
	do
	{
		parray_for( parser->states, st )
		{
			if( !st->done )
			{
				st->done = TRUE;
				lalr1_closure( parser, st->state_id );
				break;
			}
		}
	}
	while( st );

	/* Perform parse table generation. */
	parray_for( parser->states, st )
		perform_reductions( parser, st );
}

/** Performs a default production detection. This must be done immediatelly
before the code is generated, but behind all the other stuff, e.g. state-based
lexical analysis generation.

//parser// is the pointer to the parser information structure. */
void detect_default_productions( PARSER* parser )
{
	STATE*		st;
	PROD*		cur;
	TABCOL*		act;

	plistel*	e;
	LIST*		a_list;
	LIST*		n_list;

	int			max;
	int			count;

	parray_for( parser->states, st )
	{
		max = 0;

		/* Find the most common reduction and use this as default
			(quick and dirty...) */
		plist_for( parser->productions, e )
		{
			cur = (PROD*)plist_access( e );

			for( a_list = st->actions, count = 0; a_list;
					a_list = list_next( a_list ) )
			{
				act = (TABCOL*)list_access( a_list );

				if( act->action == REDUCE && act->index == cur->id )
					count++;
			}

			if( count > max )
			{
				max = count;
				st->def_prod = cur;
			}
		}

		/* Remove all entries that already match the default production */
		if( st->def_prod )
		{
			n_list = (LIST*)NULL;

			for( a_list = st->actions; a_list;
					a_list = list_next( a_list ) )
			{
				act = (TABCOL*)list_access( a_list );

				if( act->action == REDUCE &&
						act->index == st->def_prod->id )
					free_tabcol( act );
				else
					n_list = list_push( n_list, act );
			}

			list_free( st->actions );
			st->actions = n_list;
		}
	}
}
