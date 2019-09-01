/* -MODULE----------------------------------------------------------------------
Phorward C/C++ Library
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
https://phorward.info ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	nfa.c
Author:	Jan Max Meyer
Usage:	Internal NFA creation and executable functions.
----------------------------------------------------------------------------- */

#include "phorward.h"

/*NO_DOC*/
/* No documentation for the entire module, all here is only interally used. */

/** Creates a new NFA-state within an NFA state machine. The function first
checks if there are recyclable states in //nfa//. If so, the state is re-used
and re-configured, else a new state is allocated in memory.

//nfa// is the structure pointer to the NFA to process.
//chardef// is an optional charset definition for the new state. If this is
(char*)NULL, then a new epsilon state is created.
//flags// defines the modifier flags that belong to the chardef-parameter.

Returns a pointer to pregex_nfa_st, defining the newly created state. The
function returns (pregex_nfa_st*)NULL on error case.
*/
pregex_nfa_st* pregex_nfa_create_state(
	pregex_nfa* nfa, char* chardef, int flags )
{
	pregex_nfa_st* 	ptr;

	PROC( "pregex_nfa_create_state" );
	PARMS( "nfa", "%p", nfa );
	PARMS( "chardef", "%s", chardef ? chardef : "NULL" );

	/* Get new element */
	ptr = plist_malloc( nfa->states );

	/* Define character edge? */
	if( chardef )
	{
		MSG( "Required to parse chardef" );
		VARS( "chardef", "%s", chardef );

		if( !( ptr->ccl = pccl_create( -1, -1, chardef ) ) )
		{
			MSG( "Out of memory error" );
			RETURN( (pregex_nfa_st*)NULL );
		}

		/* Is case-insensitive flag set? */
		if( flags & PREGEX_COMP_INSENSITIVE )
		{
			pccl*		iccl;
			int			i;
			wchar_t		ch;
			wchar_t		cch;

			iccl = pccl_dup( ptr->ccl );

			MSG( "PREGEX_COMP_INSENSITIVE set" );
			for( i = 0; pccl_get( &ch, (wchar_t*)NULL, ptr->ccl, i ); i++ )
			{
				VARS( "ch", "%d", ch );
#ifdef UNICODE
				if( iswupper( ch ) )
					cch = towlower( ch );
				else
					cch = towupper( ch );
#else
				if( isupper( ch ) )
					cch = tolower( ch );
				else
					cch = toupper( ch );
#endif
				VARS( "cch", "%d", cch );
				if( ch == cch )
					continue;

				if( !pccl_add( iccl, cch ) )
					RETURN( (pregex_nfa_st*)NULL );
			}

			pccl_free( ptr->ccl );
			ptr->ccl = iccl;
		}

		VARS( "ptr->ccl", "%p", ptr->ccl );
	}

	RETURN( ptr );
}

/** Prints an NFA for debug purposes on a output stream.

//stream// is the output stream where to print the NFA to.
//nfa// is the NFA state machine structure to be printed.
*/
void pregex_nfa_print( pregex_nfa* nfa )
{
	plistel*		e;
	pregex_nfa_st*	s;
	int				i;

	fprintf( stderr, " no next next2 accept flags refs\n" );
	fprintf( stderr, "----------------------------------------------------\n" );

	plist_for( nfa->states, e )
	{
		s = (pregex_nfa_st*)plist_access( e );

#define GETOFF( l, s ) 	(s) ? plist_offset( plist_get_by_ptr( l, s ) ) : -1

		fprintf( stderr, "#%2d %4d %5d %6d %5d",
			GETOFF( nfa->states, s ),
			GETOFF( nfa->states, s->next ),
			GETOFF( nfa->states, s->next2 ),
			s->accept, s->flags );

		for( i = 0; i < PREGEX_MAXREF; i++ )
			if( s->refs & ( 1 << i ) )
				fprintf( stderr, " %d", i );

		fprintf( stderr, "\n" );
#undef GETOFF

		if( s->ccl )
			pccl_print( stderr, s->ccl, 0 );

		fprintf( stderr, "\n\n" );
	}

	fprintf( stderr, "----------------------------------------------------\n" );
}

/* Function for deep debugging, not used now. */
/*
static void print_nfa_state_list( char* info, pregex_nfa* nfa, plist* list )
{
	plistel*		e;
	pregex_nfa_st*	st;

	plist_for( list, e )
	{
		st = (pregex_nfa_st*)plist_access( e );
		fprintf( stderr, "%s: #%d %s\n",
			info,
			plist_offset( plist_get_by_ptr( nfa->states, st ) ),
			st->ccl ? pccl_to_str( st->ccl, TRUE ) : "EPSILON" );
	}
}
*/

/** Allocates an initializes a new pregex_nfa-object for a nondeterministic
finite state automata that can be used for pattern matching or to construct
a determinisitic finite automata out of.

The function pregex_nfa_free() shall be used to destruct a pregex_dfa-object. */
pregex_nfa* pregex_nfa_create( void )
{
	pregex_nfa*		nfa;

	nfa = (pregex_nfa*)pmalloc( sizeof( pregex_nfa ) );
	nfa->states = plist_create( sizeof( pregex_nfa_st ), PLIST_MOD_RECYCLE );

	return nfa;
}

/** Reset a pregex_nfa state machine.

The object can still be used after reset. */
pboolean pregex_nfa_reset( pregex_nfa* nfa )
{
	pregex_nfa_st*	st;

	PROC( "pregex_nfa_free" );
	PARMS( "nfa", "%p", nfa );

	if( !nfa )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	MSG( "Resetting states" );
	while( plist_first( nfa->states ) )
	{
		st = (pregex_nfa_st*)plist_access( plist_first( nfa->states ) );
		pccl_free( st->ccl );

		plist_remove( nfa->states, plist_first( nfa->states ) );
	}

	RETURN( TRUE );
}

/** Releases a pregex_nfa state machine.

//nfa// is the pointer to the NFA state machine structure. All allocated
elements of this structure as well as the structure itself will be freed.

The function always returns (pregex_nfa*)NULL.
*/
pregex_nfa* pregex_nfa_free( pregex_nfa* nfa )
{
	PROC( "pregex_nfa_free" );
	PARMS( "nfa", "%p", nfa );

	if( !nfa )
		RETURN( (pregex_nfa*)NULL );

	MSG( "Clearing states" );
	pregex_nfa_reset( nfa );

	MSG( "Dropping memory" );
	plist_free( nfa->states );
	pfree( nfa );

	RETURN( (pregex_nfa*)NULL );
}

/** Performs a move operation on a given input character from a set of NFA
states.

//nfa// is the state machine.
//hits// must be the pointer to an inizialized list of pointers to
pregex_nfa_st* that both contains the input seed for the move operation and
also receives all states that can be reached on the given
character-range. States from the input configuration will be removed, states
from the move operation will be inserted.
//from// is the character-range begin from which the move-operation should be
processed on.
//to// is the character-range end until the move-operation should be processed.

Returns the number of elements in //result//, or -1 on error.
*/
int pregex_nfa_move( pregex_nfa* nfa, plist* hits, wchar_t from, wchar_t to )
{
	plistel*		first;
	plistel*		end;
	pregex_nfa_st*	st;

	PROC( "pregex_nfa_move" );
	PARMS( "nfa", "%p", nfa );
	PARMS( "hits", "%p", hits );
	PARMS( "from", "%d", from );
	PARMS( "to", "%d", to );

	if( !( nfa && hits ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	if( !( end = plist_last( hits ) ) )
	{
		MSG( "Nothing to do, hits is empty" );
		RETURN( 0 );
	}

	/* Loop through the input items */
	do
	{
		first = plist_first( hits );
		st = (pregex_nfa_st*)plist_access( first );
		VARS( "st", "%p", st );

		/* Not an epsilon edge? */
		if( st->ccl )
		{
			/* Fine, test for range! */
			if( pccl_testrange( st->ccl, from, to ) )
			{
				MSG( "State matches range!" );
				plist_push( hits, st->next );
			}
		}

		plist_remove( hits, first );
	}
	while( first != end );

	VARS( "plist_count( hits )", "%d", plist_count( hits ) );
	RETURN( plist_count( hits ) );
}

/** Performs an epsilon closure from a set of NFA states.

//nfa// is the NFA state machine
//closure// is a list of input NFA states, which will be extended on the closure
after the function returned.
//accept// is the return pointer which receives possible information about a
pattern match. This parameter is optional, and can be left empty by providing
(unsigned int*)NULL.
//flags// is the return pointer which receives possible flagging about a
pattern match. This parameter is optional, and can be left empty by providing
(int*)NULL.

Returns a number of elements in //input//.
*/
int pregex_nfa_epsilon_closure( pregex_nfa* nfa, plist* closure,
									unsigned int* accept, int* flags )
{
	pregex_nfa_st*	top;
	pregex_nfa_st*	next;
	pregex_nfa_st*	last_accept	= (pregex_nfa_st*)NULL;
	plist*			stack;
	short			i;

	PROC( "pregex_nfa_epsilon_closure" );
	PARMS( "nfa", "%p", nfa );
	PARMS( "closure", "%p", closure );
	PARMS( "accept", "%p", accept );

	if( !( nfa && closure ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	if( accept )
		*accept = 0;
	if( flags )
		*flags = 0;

	stack = plist_dup( closure );

	/* Loop through the items */
	while( plist_pop( stack, &top ) )
	{
		if( accept && top->accept
				&& ( !last_accept
					|| last_accept->accept > top->accept ) )
			last_accept = top;

		if( !top->ccl )
		{
			for( i = 0; i < 2; i++ )
			{
				next = ( !i ? top->next : top->next2 );
				if( next && !plist_get_by_ptr( closure, next ) )
				{
					plist_push( closure, next );
					plist_push( stack, next );
				}
			}
		}
		else if( top->next2 )
		{
			/* This may not happen! */
			fprintf( stderr,
				"%s, %d: Impossible character-node "
					"has two outgoing transitions!\n", __FILE__, __LINE__ );
			exit( 1 );
		}
	}

	stack = plist_free( stack );

	if( accept && last_accept )
	{
		*accept = last_accept->accept;
		VARS( "*accept", "%d", *accept );
	}

	if( flags && last_accept )
	{
		*flags = last_accept->flags;
		VARS( "*flags", "%d", *flags );
	}

	VARS( "Closed states", "%d", plist_count( closure ) );
	RETURN( plist_count( closure ) );
}

/* !!!OBSOLETE!!! */
/** Tries to match a pattern using a NFA state machine.

//nfa// is the NFA state machine to be executed.
//str// is a test string where the NFA should work on.
//len// receives the length of the match, -1 on error or no match.

//mflags// receives the match flags configuration of the matching state, if it
provides flags. If this is (int*)NULL, any match state configurations will
be ignored.

//ref// receives a return array of references; If this pointer is not NULL, the
function will allocate memory for a reference array. This array is only
allocated if the following dependencies are met:
# The NFA has references
# //ref_count// is zero
# //ref// points to a prange*

//ref_count// receives the number of references. This value MUST be zero, if the
function should allocate refs. A positive value indicates the number of elements
in //ref//, so the array can be re-used in multiple calls.

//flags// are the flags to modify the NFA state machine matching behavior.

Returns 0, if no match was found, else the number of the match that was found
relating to a pattern in //nfa//.
*/
int pregex_nfa_match( pregex_nfa* nfa, char* str, size_t* len, int* mflags,
		prange** ref, int* ref_count, int flags )
{
	plist*			res;
	char*			pstr		= str;
	int				plen		= 0;
	int				last_accept = 0;
	wchar_t			ch;
	unsigned int	accept;
	int				aflags;

	PROC( "pregex_nfa_match" );
	PARMS( "nfa", "%p", nfa );

	if( flags & PREGEX_RUN_WCHAR )
		PARMS( "str", "%ls", str );
	else
		PARMS( "str", "%s", str );

	PARMS( "len", "%p", len );
	PARMS( "mflags", "%p", mflags );
	PARMS( "ref", "%p", ref );
	PARMS( "ref_count", "%p", ref_count );
	PARMS( "flags", "%d", flags );

	if( !( nfa && str ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	/*
	if( !pregex_ref_init( ref, ref_count, nfa->ref_count, flags ) )
		RETURN( -1 );
	*/

	*len = 0;
	if( mflags )
		*mflags = PREGEX_FLAG_NONE;

	res = plist_create( 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );
	plist_push( res, plist_access( plist_first( nfa->states ) ) );

	/* Run the engine! */
	while( plist_count( res ) )
	{
		MSG( "Performing epsilon closure" );
		if( pregex_nfa_epsilon_closure( nfa, res, &accept, &aflags ) < 0 )
		{
			MSG( "pregex_nfa_epsilon_closure() failed" );
			break;
		}

		MSG( "Handling References" );
		/*
		if( ref && nfa->ref_count )
		{
			plist_for( res, e )
			{
				st = (pregex_nfa_st*)plist_access( e );
				if( st->ref > -1 )
				{
					MSG( "Reference found" );
					VARS( "State", "%d",
							plist_offset(
								plist_get_by_ptr( nfa->states, st ) ) );
					VARS( "st->ref", "%d", st->ref );

					pregex_ref_update( &( ( *ref )[ st->ref ] ), pstr, plen );
				}
			}
		}
		*/

		VARS( "accept", "%d", accept );
		if( accept )
		{
			if( flags & PREGEX_RUN_DEBUG )
			{
				if( flags & PREGEX_RUN_WCHAR )
					fprintf( stderr, "accept %d, len %d >%.*ls<\n",
						accept, plen, plen, (wchar_t*)str );
				else
					fprintf( stderr, "accept %d, len %d >%.*s<\n",
						accept, plen, plen, str );
			}

			MSG( "New accepting state takes place!" );
			last_accept = accept;
			*len = plen;

			if( mflags )
				*mflags = aflags;

			VARS( "last_accept", "%d", last_accept );
			VARS( "*len", "%d", *len );

			if(	( aflags & PREGEX_FLAG_NONGREEDY )
					|| ( flags & PREGEX_RUN_NONGREEDY ) )
			{
				if( flags & PREGEX_RUN_DEBUG )
					fprintf( stderr, "greedy set, match terminates\n" );

				MSG( "Greedy is set, will stop recognition with this match" );
				break;
			}
		}

		if( flags & PREGEX_RUN_WCHAR )
		{
			MSG( "using wchar_t" );
			VARS( "pstr", "%ls", (wchar_t*)pstr );

			ch = *((wchar_t*)pstr);
			pstr += sizeof( wchar_t );

			if( flags & PREGEX_RUN_DEBUG )
				fprintf( stderr, "reading wchar_t >%lc< %d\n", ch, ch );
		}
		else
		{
			MSG( "using char" );
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
		VARS( "ch", "%lc", ch );

		if( pregex_nfa_move( nfa, res, ch, ch ) < 0 )
		{
			MSG( "pregex_nfa_move() failed" );
			break;
		}

		plen++;
		VARS( "plen", "%ld", plen );
	}

	plist_free( res );

	VARS( "*len", "%d", *len );
	VARS( "last_accept", "%d", last_accept );
	RETURN( last_accept );
}

/** Builds or extends a NFA state machine from a string. The string is simply
converted into a state machine that exactly matches the desired string.

//nfa// is the pointer to the NFA state machine to be extended.
//str// is the sequence of characters to be converted one-to-one into //nfa//.
//flags// are flags for regular expresson processing.
//acc// is the acceping identifier that is returned on pattern match.

Returns TRUE on success.
*/
pboolean pregex_nfa_from_string( pregex_nfa* nfa, char* str, int flags, int acc )
{
	pregex_nfa_st*	nfa_st;
	pregex_nfa_st*	append_to;
	pregex_nfa_st*	first_nfa_st;
	pregex_nfa_st*	prev_nfa_st;
	char*			pstr;
	wchar_t			ch;

	PROC( "pregex_nfa_from_string" );
	PARMS( "nfa", "%p", nfa );
	PARMS( "str", "%s", str );
	PARMS( "flags", "%d", flags );
	PARMS( "acc", "%d", acc );

	if( !( nfa && str ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	/* For wide-character execution, copy string content */
	if( flags & PREGEX_COMP_WCHAR )
	{
		if( !( str = pwcs_to_str( (wchar_t*)str, FALSE ) ) )
			RETURN( FALSE );
	}

	/* Find node to integrate into existing machine */
	for( append_to = (pregex_nfa_st*)plist_access( plist_first( nfa->states ) );
		append_to && append_to->next2; append_to = append_to->next2 )
			; /* Find last first node ;) ... */

	/* Create first state - this is an epsilon node */
	if( !( first_nfa_st = prev_nfa_st =
			pregex_nfa_create_state( nfa, (char*)NULL, flags ) ) )
		RETURN( FALSE );

	for( pstr = str; *pstr; )
	{
		/* Then, create all states that form the string */
		MSG( "Adding new state" );
		VARS( "pstr", "%s", pstr );
		if( !( nfa_st = pregex_nfa_create_state(
				nfa, (char*)NULL, flags ) ) )
			RETURN( FALSE );

		ch = putf8_parse_char( &pstr );
		VARS( "ch", "%d", ch );

		nfa_st->ccl = pccl_create( -1, -1, (char*)NULL );

		if( !( nfa_st->ccl && pccl_add( nfa_st->ccl, ch ) ) )
			RETURN( FALSE );

		/* Is case-insensitive flag set? */
		if( flags & PREGEX_COMP_INSENSITIVE )
		{
#ifdef UNICODE
			MSG( "UNICODE mode, trying to convert" );
			if( iswupper( ch ) )
				ch = towlower( ch );
			else
				ch = towupper( ch );
#else
			MSG( "non UNICODE mode, trying to convert" );
			if( isupper( ch ) )
				ch = (char)tolower( (int)ch );
			else
				ch = (char)toupper( (int)ch );
#endif

			MSG( "Case-insensity set, new character evaluated is:" );
			VARS( "ch", "%d", ch );
			if( !pccl_add( nfa_st->ccl, ch ) )
				RETURN( FALSE );
		}

		prev_nfa_st->next = nfa_st;
		prev_nfa_st = nfa_st;
	}

	/* Add accepting node */
	VARS( "acc", "%d", acc );
	if( !( nfa_st = pregex_nfa_create_state( nfa,
			(char*)NULL, flags ) ) )
		RETURN( FALSE );

	nfa_st->accept = acc;
	prev_nfa_st->next = nfa_st;

	/* Append to existing machine, if required */
	VARS( "append_to", "%p", append_to );
	if( append_to )
		append_to->next2 = first_nfa_st;

	/* Free copied string, in wide character mode */
	if( flags & PREGEX_COMP_WCHAR )
		pfree( str );

	RETURN( TRUE );
}

/*COD_ON*/
