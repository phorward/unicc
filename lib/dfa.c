/* -MODULE----------------------------------------------------------------------
Phorward C/C++ Library
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
https://phorward.info ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	dfa.c
Author:	Jan Max Meyer
Usage:	Internal DFA creation and transformation functions.
----------------------------------------------------------------------------- */

#include "phorward.h"

/*NO_DOC*/
/* No documentation for the entire module, all here is only used internally. */

/* For Debug */
void pregex_dfa_print( pregex_dfa* dfa )
{
	plistel*		e;
	plistel*		f;
	pregex_dfa_st*	s;
	pregex_dfa_tr*	t;
	int				i;

	plist_for( dfa->states, e )
	{
		s = (pregex_dfa_st*)plist_access( e );
		fprintf( stderr, "*** STATE %d, accepts %d, flags %d",
			plist_offset( plist_get_by_ptr( dfa->states, s ) ),
				s->accept, s->flags );

		if( s->refs )
		{
			fprintf( stderr, " refs" );

			for( i = 0; i < PREGEX_MAXREF; i++ )
				if( s->refs & ( 1 << i ) )
					fprintf( stderr, " %d", i );
		}

		fprintf( stderr, "\n" );

		plist_for( s->trans, f )
		{
			t = (pregex_dfa_tr*)plist_access( f );

			pccl_print( stderr, t->ccl, 0 );
			fprintf( stderr, "-> %d\n", t->go_to );
		}

		fprintf( stderr, "\n" );
	}
}

/* Sort transitions by characters */
static int pregex_dfa_sort_trans( plist* list, plistel* el, plistel* er )
{
	pregex_dfa_tr*	l = (pregex_dfa_tr*)plist_access( el );
	pregex_dfa_tr*	r = (pregex_dfa_tr*)plist_access( er );

	return pccl_compare( l->ccl, r->ccl ) < 0 ? 1 : 0;
}

/* Sort transitions by characters */
static int pregex_dfa_sort_classes( plist* list, plistel* el, plistel* er )
{
	pccl*	l = (pccl*)plist_access( el );
	pccl*	r = (pccl*)plist_access( er );

	return pccl_compare( l, r ) < 0 ? 1 : 0;
}

/* Creating a new DFA state */
static pregex_dfa_st* pregex_dfa_create_state( pregex_dfa* dfa )
{
	pregex_dfa_st* 	ptr;

	ptr = (pregex_dfa_st*)plist_malloc( dfa->states );

	ptr->trans = plist_create( sizeof( pregex_dfa_tr ), PLIST_MOD_RECYCLE );
	plist_set_sortfn( ptr->trans, pregex_dfa_sort_trans );

	return ptr;
}

/* Freeing a DFA-state */
static void pregex_dfa_delete_state( pregex_dfa_st* st )
{
	plistel*		e;
	pregex_dfa_tr*	tr;

	plist_for( st->trans, e )
	{
		tr = (pregex_dfa_tr*)plist_access( e );
		pccl_free( tr->ccl );
	}

	st->trans = plist_free( st->trans );
}

/** Allocates an initializes a new pregex_dfa-object for a deterministic
finite state automata that can be used for pattern matching. A DFA
is currently created out of an NFA.

The function pregex_dfa_free() shall be used to destruct a pregex_dfa-object. */
pregex_dfa* pregex_dfa_create( void )
{
	pregex_dfa*		dfa;

	dfa = (pregex_dfa*)pmalloc( sizeof( pregex_dfa ) );
	dfa->states = plist_create( sizeof( pregex_dfa_st ), PLIST_MOD_RECYCLE );

	return dfa;
}

/** Resets a DFA state machine.

The object //dfa// can still be used as fresh, empty object after reset. */
pboolean pregex_dfa_reset( pregex_dfa* dfa )
{
	plistel*		e;

	if( !( dfa ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	while( ( e = plist_first( dfa->states ) ) )
	{
		pregex_dfa_delete_state( (pregex_dfa_st*)plist_access( e ) );
		plist_remove( dfa->states, e );
	}

	return TRUE;
}

/** Frees and resets a DFA state machine.

//dfa// is the pointer to a DFA-machine to be reset.

Always returns (pregex_dfa*)NULL.
*/
pregex_dfa* pregex_dfa_free( pregex_dfa* dfa )
{
	if( !( dfa ) )
		return (pregex_dfa*)NULL;

	pregex_dfa_reset( dfa );

	plist_free( dfa->states );
	pfree( dfa );

	return (pregex_dfa*)NULL;
}

/** Performs a check on all DFA state transitions to figure out the default
transition for every dfa state. The default-transition is only filled, when any
character of the entire character-range results in a transition, and is set to
the transition with the most characters in its class.

For example, the regex "[^\"]*" causes a dfa-state with two transitions:
On '"', go to state x, on every character other than '"', go to state y.
y will be selected as default state.
*/
static void pregex_dfa_default_trans( pregex_dfa* dfa )
{
	plistel*		e;
	plistel*		f;
	pregex_dfa_st*	st;
	pregex_dfa_tr*	tr;
	int				max;
	int				all;
	int				cnt;

	PROC( "pregex_dfa_default_trans" );
	PARMS( "dfa", "%p", dfa );

	plist_for( dfa->states, e )
	{
		st = (pregex_dfa_st*)plist_access( e );

		if( !st->trans )
			continue;

		/* Sort transitions */
		plist_sort( st->trans );

		/* Find default transition */
		max = all = 0;
		plist_for( st->trans, f )
		{
			tr = (pregex_dfa_tr*)plist_access( f );

			if( max < ( cnt = pccl_count( tr->ccl ) ) )
			{
				max = cnt;
				st->def_trans = tr;
			}

			all += cnt;
		}

		if( all < PCCL_MAX )
			st->def_trans = (pregex_dfa_tr*)NULL;
	}

	VOIDRET;
}

/** Collects all references by the NFA-states forming a DFA-state, and puts them
into an dynamically allocated array for later re-use.

//st// is the DFA-state, for which references shall be collected.
*/
static pboolean pregex_dfa_collect_ref( pregex_dfa_st* st, plist* nfa_set )
{
	plistel*		e;
	pregex_nfa_st*	nfa_st;

	PROC( "pregex_dfa_collect_ref" );

	/* Find out number of references */
	MSG( "Searching for references in the NFA transitions" );
	plist_for( nfa_set, e )
	{
		nfa_st = (pregex_nfa_st*)plist_access( e );
		st->refs |= nfa_st->refs;
	}

	RETURN( TRUE );
}

/* Checks for DFA-states with same NFA-epsilon transitions than the specified
	one within the DFA-state machine. If an equal item is found, the offset of
		that DFA-state is returned, else -1. */
static pregex_dfa_st* pregex_dfa_same_transitions(
							pregex_dfa* dfa, plist* trans, parray* sets )
{
	plistel*		e;
	pregex_dfa_st*	ptr;
	plist*			nfa_set;

	plist_for( dfa->states, e )
	{
		ptr = (pregex_dfa_st*)plist_access( e );

		nfa_set = *( (plist**)parray_get( sets, plist_offset( e ) ) );
		if( plist_diff( nfa_set, trans ) == 0 )
			return ptr;
	}

	return (pregex_dfa_st*)NULL;
}

/** Turns a NFA-state machine into a DFA-state machine using the
subset-construction algorithm.

//dfa// is the pointer to the DFA-machine that will be constructed by this
function. The pointer is set to zero before it is used.
//nfa// is the pointer to the NFA-Machine where the DFA-machine should be
constructed from.

Returns the number of DFA states that where constructed.
In case of an error, -1 is returned.
*/
int pregex_dfa_from_nfa( pregex_dfa* dfa, pregex_nfa* nfa )
{
	plist*			transitions;
	plist*			classes;
	plist*			done;
	parray*			sets;

	plist*			nfa_set;
	plist*			current_nfa_set;

	plistel*		e;
	plistel*		f;
	pregex_dfa_tr*	trans;
	pregex_dfa_st*	current;
	pregex_dfa_st*	st;
	pregex_nfa_st*	nfa_st;
	int				state_next	= 0;
	pboolean		changed;
	int				i;
	wchar_t			begin;
	wchar_t			end;
	pccl*			ccl;
	pccl*			test;
	pccl*			del;
	pccl*			subset;

	PROC( "pregex_dfa_from_nfa" );
	PARMS( "dfa", "%p", dfa );
	PARMS( "nfa", "%p", nfa );

	if( !( dfa && nfa ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	/* Initialize */
	classes = plist_create( 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );
	plist_set_sortfn( classes, pregex_dfa_sort_classes );

	transitions = plist_create( 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );

	done = plist_create( 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );

	sets = parray_create( sizeof( plist* ), 0 );

	/* Starting seed */

	if( !( current = pregex_dfa_create_state( dfa ) ) )
		RETURN( -1 );

	nfa_set = plist_create( 0, PLIST_MOD_PTR );
	plist_push( nfa_set, plist_access( plist_first( nfa->states ) ) );
	parray_push( sets, &nfa_set );

	pregex_nfa_epsilon_closure( nfa, nfa_set, (unsigned int*)NULL, (int*)NULL );
	pregex_dfa_collect_ref( current, nfa_set );

	/* Perform algorithm until all states are done */
	while( TRUE )
	{
		MSG( "WHILE" );

		/* Get next undone state */
		plist_for( dfa->states, e )
		{
			current = (pregex_dfa_st*)plist_access( e );
			if( !plist_get_by_ptr( done, current ) )
				break;
		}

		if( !e )
		{
			MSG( "No more undone states found" );
			break;
		}

		plist_push( done, current );
		current->accept = 0;
		current_nfa_set = *( (plist**)parray_get( sets, plist_offset( e ) ) );

		/* Assemble all character sets in the alphabet list */
		plist_erase( classes );

		plist_for( current_nfa_set, e )
		{
			nfa_st = (pregex_nfa_st*)plist_access( e );

			if( nfa_st->accept )
			{
				MSG( "NFA is an accepting state" );
				if( !current->accept || current->accept >= nfa_st->accept )
				{
					MSG( "Copying accept information" );
					current->accept = nfa_st->accept;
					current->flags = nfa_st->flags;
				}
			}

			/* Generate list of character classes */
			if( nfa_st->ccl )
			{
				VARS( "nfa_st->ccl", "%s", pccl_to_str( nfa_st->ccl, TRUE ) );
				MSG( "Adding character class to list" );
				if( !( ccl = pccl_dup( nfa_st->ccl ) ) )
					RETURN( -1 );

				if( !plist_push( classes, ccl ) )
					RETURN( -1 );

				VARS( "plist_count( classes )", "%d", plist_count( classes ) );
			}
		}

		VARS( "current->accept", "%d", current->accept );

		MSG( "Removing intersections within character classes" );
		do
		{
			changed = FALSE;

			plist_for( classes, e )
			{
				ccl = (pccl*)plist_access( e );

				plist_for( classes, f )
				{
					if( e == f )
						continue;

					test = (pccl*)plist_access( f );

					if( pccl_count( ccl ) > pccl_count( test ) )
						continue;

					if( ( subset = pccl_intersect( ccl, test ) ) )
					{
						test = pccl_diff( test, subset );
						pccl_free( subset );

						del = (pccl*)plist_access( f );
						pccl_free( del );

						plist_remove( classes, f );
						plist_push( classes, test );

						changed = TRUE;
					}
				}
			}

			VARS( "changed", "%s", BOOLEAN_STR( changed ) );
		}
		while( changed );

		/* Sort classes */
		plist_sort( classes );

		MSG( "Make transitions on constructed alphabet" );
		/* Make transitions on constructed alphabet */
		plist_for( classes, e )
		{
			ccl = (pccl*)plist_access( e );

			MSG( "Check char class" );
			for( i = 0; pccl_get( &begin, &end, ccl, i ); i++ )
			{
				VARS( "begin", "%d", begin );
				VARS( "end", "%d", end );

				plist_for( current_nfa_set, f )
				{
					if( !plist_push( transitions, plist_access( f ) ) )
						RETURN( -1 );
				}

				if( pregex_nfa_move( nfa, transitions, begin, end ) < 0 )
				{
					MSG( "pregex_nfa_move() failed" );
					break;
				}

				if( pregex_nfa_epsilon_closure( nfa, transitions,
								(unsigned int*)NULL, (int*)NULL ) < 0 )
				{
					MSG( "pregex_nfa_epsilon_closure() failed" );
					break;
				}

				if( !plist_count( transitions ) )
				{
					/* There is no move on this character! */
					MSG( "transition set is empty, will continue" );
					continue;
				}
				else if( ( st = pregex_dfa_same_transitions(
									dfa, transitions, sets ) ) )
				{
					MSG( "State with same transitions exists" );
					/* This transition is already existing in the DFA
						- discard the transition table! */
					plist_erase( transitions );
					state_next = plist_offset(
									plist_get_by_ptr( dfa->states, st ) );

					nfa_set = *( (plist**)parray_get( sets, state_next ) );
					pregex_dfa_collect_ref( st, nfa_set );
				}
				else
				{
					MSG( "Creating new DFA state" );
					/* Create a new DFA as undone with this transition! */
					if( !( st = pregex_dfa_create_state( dfa ) ) )
						RETURN( -1 );

					state_next = plist_count( dfa->states ) - 1;

					nfa_set = plist_dup( transitions );
					plist_erase( transitions );

					pregex_dfa_collect_ref( st, nfa_set );

					parray_push( sets, &nfa_set );
				}

				VARS( "state_next", "%d", state_next );

				/* Find transition entry with same follow state */
				plist_for( current->trans, f )
				{
					trans = (pregex_dfa_tr*)plist_access( f );

					if( trans->go_to == state_next )
						break;
				}

				VARS( "trans", "%p", trans );

				if( !f )
				{
					MSG( "Need to create new transition entry" );

					/* Set the transition into the transition state dfatab... */
					trans = plist_malloc( current->trans );
					trans->ccl = pccl_create( -1, -1, (char*)NULL );
					trans->go_to = state_next;
				}

				VARS( "Add range:begin", "%d", begin );
				VARS( "Add range:end", "%d", end );

				/* Append current range */
				if( !pccl_addrange( trans->ccl, begin, end ) )
					RETURN( -1 );
			}

			pccl_free( ccl );
		}
	}

	/* Clear temporary allocated memory */
	plist_free( done );
	plist_free( classes );
	plist_free( transitions );

	while( parray_count( sets ) )
		plist_free( *( (plist**)parray_pop( sets ) ) );

	parray_free( sets );

	/* Set default transitions */
	pregex_dfa_default_trans( dfa );

	RETURN( plist_count( dfa->states ) );
}

/** Checks for transition equality within two dfa states.

//dfa// is the pointer to DFA state machine.
//groups// is the list of DFA groups
//first// is the first state to check.
//second// is the Second state to check.

Returns TRUE if both states are totally equal, FALSE else.
*/
static pboolean pregex_dfa_equal_states(
	pregex_dfa* dfa, plist* groups,
		pregex_dfa_st* first, pregex_dfa_st* second )
{
	plistel*		e;
	plistel*		f;
	plistel*		g;
	pregex_dfa_tr*	tr	[ 2 ];

	PROC( "pregex_dfa_equal_states" );
	PARMS( "first", "%p", first );
	PARMS( "second", "%p", second );

	if( plist_count( first->trans ) != plist_count( second->trans ) )
	{
		MSG( "Number of transitions is different" );
		RETURN( FALSE );
	}

	for( e = plist_first( first->trans ), f = plist_first( second->trans );
			e && f; e = plist_next( e ), f = plist_next( f ) )
	{
		tr[0] = (pregex_dfa_tr*)plist_access( e );
		tr[1] = (pregex_dfa_tr*)plist_access( f );

		/* Equal Character class selection? */
		if( pccl_compare( tr[0]->ccl, tr[1]->ccl ) )
		{
			MSG( "Character classes are not equal" );
			RETURN( FALSE );
		}

		/* Search for goto-state group equality */
		first = (pregex_dfa_st*)plist_access(
					plist_get( dfa->states, tr[0]->go_to ) );
		second = (pregex_dfa_st*)plist_access(
					plist_get( dfa->states, tr[1]->go_to ) );

		plist_for( groups, g )
		{
			if( plist_get_by_ptr( (plist*)plist_access( g ), first )
				&& !plist_get_by_ptr( (plist*)plist_access( g ), second ) )
			{
				MSG( "Transition state are in different groups\n" );
				RETURN( FALSE );
			}
		}
	}

	RETURN( TRUE );
}

/** Minimizes a DFA to lesser states by grouping equivalent states to new
states, and transforming transitions to them.

//dfa// is the pointer to the DFA-machine that will be minimized. The content of
//dfa// will be replaced with the reduced machine.

Returns the number of DFA states that where constructed in the minimized version
of //dfa//. Returns -1 on error.
*/
int pregex_dfa_minimize( pregex_dfa* dfa )
{
	pregex_dfa_st*	dfa_st;
	pregex_dfa_st*	grp_dfa_st;
	pregex_dfa_tr*	ent;

	plist*			min_states;
	plist*			group;
	plist*			groups;
	plist*			newgroup;

	plistel*		e;
	plistel*		f;
	plistel*		g;
	plistel*		first;
	plistel*		next;
	plistel*		next_next;
	int				i;
	pboolean		changes		= TRUE;

	PROC( "pregex_dfa_minimize" );
	PARMS( "dfa", "%p", dfa );

	if( !dfa )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	groups = plist_create( 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );

	MSG( "First, all states are grouped by accepting id" );
	plist_for( dfa->states, e )
	{
		dfa_st = (pregex_dfa_st*)plist_access( e );

		plist_for( groups, f )
		{
			group = (plist*)plist_access( f );
			grp_dfa_st = (pregex_dfa_st*)plist_access( plist_first( group ) );

			if( grp_dfa_st->accept == dfa_st->accept )
				break;
		}

		if( !f )
		{
			group = plist_create( 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );
			if( !plist_push( group, dfa_st ) )
				RETURN( -1 );

			if( !plist_push( groups, group ) )
				RETURN( -1 );
		}
		else if( !plist_push( group, dfa_st ) )
			RETURN( -1 );
	}

	MSG( "Perform the algorithm" );
	while( changes )
	{
		MSG( "LOOP" );
		VARS( "groups", "%d", plist_count( groups ) );
		changes = FALSE;

		plist_for( groups, e )
		{
			MSG( "Examining group" );
			newgroup = (plist*)NULL;

			group = (plist*)plist_access( e );

			first = plist_first( group );

			dfa_st = (pregex_dfa_st*)plist_access( first );
			next_next = plist_next( first );

			while( ( next = next_next ) )
			{
				next_next = plist_next( next );
				grp_dfa_st = (pregex_dfa_st*)plist_access( next );

				VARS( "dfa_st", "%d", plist_offset(
										plist_get_by_ptr(
											dfa->states, dfa_st ) ) );
				VARS( "grp_dfa_st", "%d", plist_offset(
										plist_get_by_ptr(
											dfa->states, grp_dfa_st ) ) );
				VARS( "dfa_st->trans", "%p", dfa_st->trans );
				VARS( "grp_dfa_st->trans", "%p", grp_dfa_st->trans );

				/* Check for state equality */
				if( !pregex_dfa_equal_states( dfa, groups,
							dfa_st, grp_dfa_st ) )
				{
					MSG( "States aren't equal" );
					MSG( "Removing state from current group" );
					plist_remove( group, plist_get_by_ptr(
											group, grp_dfa_st ) );

					if( !plist_count( group ) )
						plist_remove( groups,
							plist_get_by_ptr( groups, first ) );

					MSG( "Appending state into new group" );

					if( !newgroup )
					{
						if( !( newgroup =
								plist_create( 0,
									PLIST_MOD_PTR | PLIST_MOD_RECYCLE ) ) )
							RETURN( -1 );
					}

					if( !plist_push( newgroup, grp_dfa_st ) )
						RETURN( -1 );
				}
			}

			VARS( "newgroup", "%p", newgroup );
			if( newgroup )
			{
				MSG( "Engaging new group into list of groups" );
				if( !plist_push( groups, newgroup ) )
					RETURN( -1 );

				changes = TRUE;
			}
		}

		VARS( "changes", "%s", BOOLEAN_STR( changes ) );
	}

	/* Now that we have all groups, reduce each group to one state */
	plist_for( groups, e )
	{
		group = (plist*)plist_access( e );
		grp_dfa_st = (pregex_dfa_st*)plist_access( plist_first( group ) );

		plist_for( grp_dfa_st->trans, f )
		{
			ent = (pregex_dfa_tr*)plist_access( f );

			dfa_st = (pregex_dfa_st*)plist_access(
										plist_get( dfa->states, ent->go_to ) );

			for( g = plist_first( groups ), i = 0;
					g; g = plist_next( g ), i++ )
			{
				if( plist_get_by_ptr( (plist*)plist_access( g ), dfa_st ) )
				{
					ent->go_to = i;
					break;
				}
			}
		}
	}

	/* Put leading group states into new, minimized dfa state machine */
	min_states = plist_create( sizeof( pregex_dfa_st ), PLIST_MOD_RECYCLE );

	plist_for( groups, e )
	{
		group = (plist*)plist_access( e );

		grp_dfa_st = (pregex_dfa_st*)plist_access( plist_first( group ) );

		/* Delete all states except the first one in the group */
		for( f = plist_next( plist_first( group ) ); f; f = plist_next( f ) )
		{
			dfa_st = (pregex_dfa_st*)plist_access( f );
			grp_dfa_st->refs |= dfa_st->refs;
			pregex_dfa_delete_state( dfa_st );
		}

		/* Push the first one into the minimized list */
		plist_push( min_states, grp_dfa_st );

		/* Free this group */
		plist_free( group );
	}

	plist_free( groups );

	/* Replace states by minimized list */
	plist_free( dfa->states );
	dfa->states = min_states;

	/* Set default transitions */
	pregex_dfa_default_trans( dfa );

	RETURN( plist_count( dfa->states ) );
}

/* !!!OBSOLETE!!! */
/** Tries to match a pattern using a DFA state machine.

//dfa// is the DFA state machine to be executed.
//str// is the test string where the DFA should work on.
//len// is the length of the match, -1 on error or no match.

//flags// are the flags to modify the DFA state machine matching behavior.

Returns 0, if no match was found, else the id of the bestmost (=longest) match.
*/
int pregex_dfa_match( pregex_dfa* dfa, char* str, size_t* len,
		int* mflags, prange** ref, int* ref_count, int flags )
{
	pregex_dfa_st*	dfa_st;
	pregex_dfa_st*	next_dfa_st;
	pregex_dfa_st*	last_accept = (pregex_dfa_st*)NULL;
	pregex_dfa_tr*	ent;
	plistel*		e;
	char*			pstr		= str;
	size_t			plen		= 0;
	wchar_t			ch;

	PROC( "pregex_dfa_match" );
	PARMS( "dfa", "%p", dfa );

	if( flags & PREGEX_RUN_WCHAR )
		PARMS( "str", "%ls", str );
	else
		PARMS( "str", "%s", str );

	PARMS( "len", "%p", len );
	PARMS( "mflags", "%p", mflags );
	PARMS( "ref", "%p", ref );
	PARMS( "ref_count", "%p", ref_count );
	PARMS( "flags", "%d", flags );

	if( !( dfa && str && len ) )
	{
		WRONGPARAM;
		RETURN( 0 );
	}

	/* Initialize! */
	if( mflags )
		*mflags = PREGEX_FLAG_NONE;

	/*
	if( !pregex_ref_init( ref, ref_count, dfa->ref_count, flags ) )
		RETURN( 0 );
	*/

	*len = 0;
	dfa_st = (pregex_dfa_st*)plist_access( plist_first( dfa->states ) );

	while( dfa_st )
	{
		MSG( "At begin of loop" );

		VARS( "dfa_st->accept", "%d", dfa_st->accept );
		if( dfa_st->accept )
		{
			MSG( "This state has an accept" );
			last_accept = dfa_st;
			*len = plen;

			if(	( last_accept->flags & PREGEX_FLAG_NONGREEDY )
					|| ( flags & PREGEX_RUN_NONGREEDY ) )
			{
				MSG( "This match is not greedy, so matching will stop now" );
				break;
			}
		}

		/*
		MSG( "Handling References" );
		if( ref && dfa->ref_count && dfa_st->ref_cnt )
		{
			VARS( "Having ref_cnt", "%d", dfa_st->ref_cnt );

			for( i = 0; i < dfa_st->ref_cnt; i++ )
			{
				VARS( "i", "%d", i );
				VARS( "State", "%d", plist_offset(
										plist_get_by_ptr(
											dfa->states, dfa_st ) ) );
				VARS( "dfa_st->ref[i]", "%d", dfa_st->ref[ i ] );

				pregex_ref_update( &( ( *ref )[ dfa_st->ref[ i ] ] ),
									pstr, plen );
			}
		}
		*/

		/* Get next character */
		if( flags & PREGEX_RUN_WCHAR )
		{
			VARS( "pstr", "%ls", (wchar_t*)pstr );
			ch = *((wchar_t*)pstr);
			pstr += sizeof( wchar_t );

			if( flags & PREGEX_RUN_DEBUG )
				fprintf( stderr, "reading wchar_t >%lc< %d\n", ch, ch );
		}
		else
		{
			VARS( "pstr", "%s", pstr );
#ifdef UTF8
			ch = putf8_char( pstr );
			pstr += putf8_seqlen( pstr );
#else
			ch = *pstr++;
#endif

			if( flags & PREGEX_RUN_DEBUG )
				fprintf( stderr, "reading char >%c< %d\n", ch, ch );
		}

		VARS( "ch", "%d", ch );

		next_dfa_st = (pregex_dfa_st*)NULL;
		plist_for( dfa_st->trans, e )
		{
			ent = (pregex_dfa_tr*)plist_access( e );

			if( pccl_test( ent->ccl, ch ) )
			{
				MSG( "Having a character match!" );
				next_dfa_st = (pregex_dfa_st*)plist_access(
									plist_get( dfa->states, ent->go_to ) );
				break;
			}
		}

		if( !next_dfa_st )
		{
			MSG( "No transitions match!" );
			break;
		}

		plen++;
		VARS( "plen", "%ld", plen );

		dfa_st = next_dfa_st;
	}

	MSG( "Done scanning" );

	if( mflags && last_accept )
	{
		*mflags = last_accept->flags;
		VARS( "*mflags", "%d", *mflags );
	}

	VARS( "*len", "%d", *len );
	VARS( "last_accept->accept", "%d",
		( last_accept ? last_accept->accept : 0 ) );

	RETURN( ( last_accept ? last_accept->accept : 0 ) );
}

/* DFA Table Structure */

/** Compiles the significant state table of a pregex_dfa-structure into a
two-dimensional array //dfatab//.

//dfatab// is a pointer to a variable that receives the allocated DFA state machine, where
each row forms a state that is made up of columns described in the table below.

|| Column / Index | Content |
| 0 | Total number of columns in the current row |
| 1 | Match ID if > 0, or 0 if the state is not an accepting state |
| 2 | Match flags (anchors, greedyness, (PREGEX_FLAG_*)) |
| 3 | Reference flags; The index of the flagged bits defines the number of \
reference |
| 4 | Default transition from the current state. If there is no transition, \
its value is set to the number of all states. |
| 5 | Transition: from-character |
| 6 | Transition: to-character |
| 7 | Transition: Goto-state |
| ... | more triples follow for each transition |


Example for a state machine that matches the regular expression ``@[a-z0-9]+``
that has match 1 and no references:

```
8 0 0 0 3 64 64 2
11 1 0 0 3 48 57 1 97 122 1
11 0 0 0 3 48 57 1 97 122 1
```

Interpretation:

```
00: col= 8 acc= 0 flg= 0 ref= 0 def= 3 tra=064(@);064(@):02
01: col=11 acc= 1 flg= 0 ref= 0 def= 3 tra=048(0);057(9):01 tra=097(a);122(z):01
02: col=11 acc= 0 flg= 0 ref= 0 def= 3 tra=048(0);057(9):01 tra=097(a);122(z):01
```

A similar dump like this interpretation above will be printed to stderr by the
function when //dfatab// is provided as (long***)NULL.

The pointer assigned to //dfatab// must be freed after usage using a for-loop:

```
for( i = 0; i < dfatab_cnt; i++ )
	pfree( dfatab[i] );

pfree( dfatab );
```

The function returns the number of states of //dfa//, or -1 in error case.
*/
int pregex_dfa_to_dfatab( wchar_t*** dfatab, pregex_dfa* dfa )
{
	wchar_t**		trans;
	pregex_dfa_st*	st;
	pregex_dfa_tr*	tr;
	int				i;
	int				j;
	int				k;
	int				cnt;
	plistel*		e;
	plistel*		f;
	wchar_t			from;
	wchar_t			to;
	char			pr_from	[ 10 + 1 ];
	char			pr_to	[ 10 + 1 ];

	PROC( "pregex_dfa_to_dfatab" );
	PARMS( "dfatab", "%p", dfatab );
	PARMS( "dfa", "%p", dfa );

	if( !( dfa ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	if( !( trans = (wchar_t**)pmalloc( plist_count( dfa->states )
										* sizeof( wchar_t* ) ) ) )
		RETURN( -1 );

	for( i = 0, e = plist_first( dfa->states ); e; e = plist_next( e ), i++ )
	{
		VARS( "state( i )", "%d", i );
		st = (pregex_dfa_st*)plist_access( e );

		/*
			Row formatting:

			Number-of-columns;Accept;Flags;Ref-Flags;Default-Goto;

			Then, dynamic columns follow:
			Ref;...;From-Char;To-Char;Goto;...

			The rest consists of triples containing
			"From-Char;To-Char;Goto" each.
		*/

		MSG( "Examine required number of columns" );
		for( cnt = 5, f = plist_first( st->trans ); f; f = plist_next( f ) )
		{
			tr = (pregex_dfa_tr*)plist_access( f );
			if( st->def_trans == tr )
				continue;

			cnt += pccl_size( tr->ccl ) * 3;
		}

		VARS( "required( cnt )", "%d", cnt );

		trans[ i ] = (wchar_t*)pmalloc( cnt * sizeof( wchar_t ) );

		trans[ i ][ 0 ] = cnt;
		trans[ i ][ 1 ] = st->accept;
		trans[ i ][ 2 ] = st->flags;
		trans[ i ][ 3 ] = st->refs;
		trans[ i ][ 4 ] = st->def_trans ? st->def_trans->go_to :
												plist_count( dfa->states );

		MSG( "Fill columns" );
		for( j = 5, f = plist_first( st->trans ); f; f = plist_next( f ) )
		{
			tr = (pregex_dfa_tr*)plist_access( f );
			if( st->def_trans == tr )
				continue;

			for( k = 0; pccl_get( &from, &to, tr->ccl, k ); k++ )
			{
				trans[ i ][ j++ ] = from;
				trans[ i ][ j++ ] = to;
				trans[ i ][ j++ ] = tr->go_to;
			}
		}
	}

	/* Print it to stderr? */
	if( !dfatab )
	{
		for( i = 0; i < plist_count( dfa->states ); i++ )
		{
			fprintf( stderr, "%02d: col=%2d acc=%2d flg=%2x ref=%2d def=%2d",
								i, trans[i][0], trans[i][1], trans[i][2],
									trans[i][3], trans[i][4] );

			for( j = 5; j < trans[i][0]; j += 3 )
			{
				if( isprint( trans[i][j] ) )
					sprintf( pr_from, "(%lc)", trans[i][j] );
				else
					*pr_from = '\0';

				if( isprint( trans[i][j+1] ) )
					sprintf( pr_to, "(%lc)", trans[i][j+1] );
				else
					*pr_to = '\0';


				fprintf( stderr, " tra=%03d%s;%03d%s:%02d",
					trans[i][j], pr_from,
						trans[i][j+1], pr_to,
							trans[i][j+2] );
			}

			fprintf( stderr, "\n" );

			pfree( trans[i] );
		}

		pfree( trans );
	}
	else
		*dfatab = trans;

	RETURN( plist_count( dfa->states ) );
}


/*COD_ON*/

