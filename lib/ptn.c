/* -MODULE----------------------------------------------------------------------
Phorward C/C++ Library
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
https://phorward.info ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	ptn.c
Author:	Jan Max Meyer
Usage:	Functions for low-level regular expression pattern processing
----------------------------------------------------------------------------- */

#include "phorward.h"

/* Local prototypes */
static pboolean parse_alter( pregex_ptn** ptn, char** pstr, int flags );

/* Create a pattern object of type //type//; Internal constructor! */
static pregex_ptn* pregex_ptn_CREATE( pregex_ptntype type )
{
	pregex_ptn*		ptn;

	ptn = (pregex_ptn*)pmalloc( sizeof( pregex_ptn ) );
	ptn->type = type;

	return ptn;
}

/** Constructs and parses a new pregex_ptn-structure from //pat//.

This function is a shortcut for a call to pregex_ptn_parse().
pregex_ptn_create() directly takes //pat// as its input and returns the parsed
pregex_ptn structure which represents the internal representation of the
regular expression //pat//.

//flags// provides a combination of compile-time modifier flags
(PREGEX_COMP_...) if wanted, or 0 (PREGEX_FLAG_NONE) if no flags should be
used.

Returns an allocated pregex_ptn-node which must be freed using pregex_ptn_free()
when it is not used anymore.
*/
pregex_ptn* pregex_ptn_create( char* pat, int flags )
{
	pregex_ptn*		ptn;

	PROC( "pregex_ptn_create" );
	PARMS( "pat", "%s", pat );
	PARMS( "flags", "%d", flags );

	if( !( pat && *pat ) )
	{
		WRONGPARAM;
		RETURN( (pregex_ptn*)NULL );
	}

	if( !pregex_ptn_parse( &ptn, pat, flags ) )
	{
		MSG( "Parse error" );
		RETURN( (pregex_ptn*)NULL );
	}

	RETURN( ptn );
}

/** Constructs a character-class pattern.

//ccl// is the pointer to a character class. This pointer is not duplicated,
and will be directly assigned to the object.

Returns a pregex_ptn-node which can be child of another pattern construct or
part of a sequence.
*/
pregex_ptn* pregex_ptn_create_char( pccl* ccl )
{
	pregex_ptn*		pattern;

	if( !( ccl ) )
	{
		WRONGPARAM;
		return (pregex_ptn*)NULL;
	}

	pattern = pregex_ptn_CREATE( PREGEX_PTN_CHAR );
	pattern->ccl = ccl;

	return pattern;
}

/** Constructs a pattern for a static string.

//str// is the input string to be converted.
//flags// are optional flags for wide-character support.

Returns a pregex_ptn-node which can be child of another pattern construct or
part of a sequence.
*/
pregex_ptn* pregex_ptn_create_string( char* str, int flags )
{
	char*		ptr;
	wchar_t		ch;
	pregex_ptn*	chr;
	pregex_ptn*	seq		= (pregex_ptn*)NULL;
	pccl*		ccl;

	PROC( "pregex_ptn_create_string" );
	PARMS( "str", "%s", str );
	PARMS( "flags", "%d", flags );

	/* Check parameters */
	if( !( str ) )
	{
		WRONGPARAM;
		RETURN( (pregex_ptn*)NULL );
	}

	/* Convert string to UTF-8, if in wide-character mode */
	if( flags & PREGEX_COMP_WCHAR )
	{
		if( !( str = pwcs_to_str( (wchar_t*)str, FALSE ) ) )
			RETURN( (pregex_ptn*)NULL );
	}

	/* Loop through the string */
	for( ptr = str; *ptr; )
	{
		VARS( "ptr", "%s", ptr );
		ch = putf8_parse_char( &ptr );

		VARS( "ch", "%d", ch );

		ccl = pccl_create( -1, -1, (char*)NULL );

		if( !( ccl && pccl_add( ccl, ch ) ) )
		{
			pccl_free( ccl );
			RETURN( (pregex_ptn*)NULL );
		}

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

			if( !pccl_add( ccl, ch ) )
			{
				pccl_free( ccl );
				RETURN( (pregex_ptn*)NULL );
			}
		}

		if( !( chr = pregex_ptn_create_char( ccl ) ) )
		{
			pccl_free( ccl );
			RETURN( (pregex_ptn*)NULL );
		}

		if( ! seq )
			seq = chr;
		else
			seq = pregex_ptn_create_seq( seq, chr, (pregex_ptn*)NULL );
	}

	/* Free duplicated string */
	if( flags & PREGEX_COMP_WCHAR )
		pfree( str );

	RETURN( seq );
}

/** Constructs a sub-pattern (like with parantheses).

//ptn// is the pattern that becomes the sub-ordered pattern.

Returns a pregex_ptn-node which can be child of another pattern construct
or part of a sequence.
*/
pregex_ptn* pregex_ptn_create_sub( pregex_ptn* ptn )
{
	pregex_ptn*		pattern;

	if( !ptn )
	{
		WRONGPARAM;
		return (pregex_ptn*)NULL;
	}

	pattern = pregex_ptn_CREATE( PREGEX_PTN_SUB );
	pattern->child[0] = ptn;

	return pattern;
}

/** Constructs a sub-pattern as backreference (like with parantheses).

//ptn// is the pattern that becomes the sub-ordered pattern.

Returns a pregex_ptn-node which can be child of another pattern construct
or part of a sequence.
*/
pregex_ptn* pregex_ptn_create_refsub( pregex_ptn* ptn )
{
	pregex_ptn*		pattern;

	if( !ptn )
	{
		WRONGPARAM;
		return (pregex_ptn*)NULL;
	}

	pattern = pregex_ptn_CREATE( PREGEX_PTN_REFSUB );
	pattern->child[0] = ptn;

	return pattern;
}

/** Constructs alternations of multiple patterns.

//left// is the first pattern of the alternation.
//...// are multiple pregex_ptn-pointers follow which become part of the
alternation. The last node must be specified as (pregex_ptn*)NULL.

Returns a pregex_ptn-node which can be child of another pattern construct or
part of a sequence. If there is only //left// assigned without other alternation
patterns, //left// will be returned back.
*/
pregex_ptn* pregex_ptn_create_alt( pregex_ptn* left, ...  )
{
	pregex_ptn*		pattern;
	pregex_ptn*		alter;
	va_list			alt;

	if( !( left ) )
	{
		WRONGPARAM;
		return (pregex_ptn*)NULL;
	}

	va_start( alt, left );

	while( ( alter = va_arg( alt, pregex_ptn* ) ) )
	{
		pattern = pregex_ptn_CREATE( PREGEX_PTN_ALT );
		pattern->child[0] = left;
		pattern->child[1] = alter;

		left = pattern;
	}

	va_end( alt );

	return left;
}

/** Constructs a kleene-closure repetition, allowing for multiple or none
repetitions of the specified pattern.

//ptn// is the pattern that will be configured for kleene-closure.

Returns a pregex_ptn-node which can be child of another pattern construct or
part of a sequence.
*/
pregex_ptn* pregex_ptn_create_kle( pregex_ptn* ptn )
{
	pregex_ptn*		pattern;

	if( !ptn )
	{
		WRONGPARAM;
		return (pregex_ptn*)NULL;
	}

	pattern = pregex_ptn_CREATE( PREGEX_PTN_KLE );
	pattern->child[0] = ptn;

	return pattern;
}

/** Constructs an positive-closure, allowing for one or multiple specified
pattern.

//ptn// is the pattern to be configured for positive closure.

Returns a pregex_ptn-node which can be child of another pattern construct or
part of a sequence.
*/
pregex_ptn* pregex_ptn_create_pos( pregex_ptn* ptn )
{
	pregex_ptn*		pattern;

	if( !ptn )
	{
		WRONGPARAM;
		return (pregex_ptn*)NULL;
	}

	pattern = pregex_ptn_CREATE( PREGEX_PTN_POS );
	pattern->child[0] = ptn;

	return pattern;
}

/** Constructs an optional-closure, allowing for one or none specified pattern.

//ptn// is the pattern to be configured for optional closure.

Returns a pregex_ptn-node which can be child of another pattern construct or
part of a sequence.
*/
pregex_ptn* pregex_ptn_create_opt( pregex_ptn* ptn )
{
	pregex_ptn*		pattern;

	if( !ptn )
	{
		WRONGPARAM;
		return (pregex_ptn*)NULL;
	}

	pattern = pregex_ptn_CREATE( PREGEX_PTN_OPT );
	pattern->child[0] = ptn;

	return pattern;
}

/** Constructs a sequence of multiple patterns.

//first// is the beginning pattern of the sequence. //...// follows as parameter
list of multiple patterns that become part of the sequence. The last pointer
must be specified as (pregex_ptn*)NULL to mark the end of the list.

Always returns the pointer to //first//.
*/
pregex_ptn* pregex_ptn_create_seq( pregex_ptn* first, ... )
{
	pregex_ptn*		prev	= first;
	pregex_ptn*		next;
	va_list			chain;

	if( !first )
	{
		WRONGPARAM;
		return (pregex_ptn*)NULL;
	}

	va_start( chain, first );

	while( ( next = va_arg( chain, pregex_ptn* ) ) )
	{
		for( ; prev->next; prev = prev->next )
			;

		/* Alternatives must be put in brackets */
		if( next->type == PREGEX_PTN_ALT )
			next = pregex_ptn_create_sub( next );

		prev->next = next;
		prev = next;
	}

	va_end( chain );

	return first;
}

/** Duplicate //ptn// into a stand-alone 1:1 copy. */
pregex_ptn* pregex_ptn_dup( pregex_ptn* ptn )
{
	pregex_ptn*		start	= (pregex_ptn*)NULL;
	pregex_ptn*		prev	= (pregex_ptn*)NULL;
	pregex_ptn*		dup;

	PROC( "pregex_ptn_dup" );
	PARMS( "ptn", "%p", ptn );

	while( ptn )
	{
		dup = pregex_ptn_CREATE( ptn->type );

		if( !start )
			start = dup;
		else
			prev->next = dup;

		if( ptn->type == PREGEX_PTN_CHAR )
			dup->ccl = pccl_dup( ptn->ccl );

		if( ptn->child[0] )
			dup->child[0] = pregex_ptn_dup( ptn->child[0] );

		if( ptn->child[1] )
			dup->child[1] = pregex_ptn_dup( ptn->child[1] );

		dup->accept = ptn->accept;
		dup->flags = ptn->flags;

		prev = dup;
		ptn = ptn->next;
	}

	RETURN( start );
}

/** Releases memory of a pattern including all its subsequent and following
patterns.

//ptn// is the pattern object to be released.

Always returns (pregex_ptn*)NULL.
*/
pregex_ptn* pregex_ptn_free( pregex_ptn* ptn )
{
	pregex_ptn*		next;

	if( !ptn )
		return (pregex_ptn*)NULL;

	do
	{
		if( ptn->type == PREGEX_PTN_CHAR )
			pccl_free( ptn->ccl );

		if( ptn->child[0] )
			pregex_ptn_free( ptn->child[0] );

		if( ptn->child[1] )
			pregex_ptn_free( ptn->child[1] );

		pfree( ptn->str );

		next = ptn->next;
		pfree( ptn );

		ptn = next;
	}
	while( ptn );

	return (pregex_ptn*)NULL;
}

/** A debug function to print a pattern's hierarchical structure to stderr.

//ptn// is the pattern object to be printed.
//rec// is the recursion depth, set this to 0 at initial call.
*/
void pregex_ptn_print( pregex_ptn* ptn, int rec )
{
	int			i;
	char		gap		[ 100+1 ];
	static char	types[][20]	= { "PREGEX_PTN_NULL", "PREGEX_PTN_CHAR",
								"PREGEX_PTN_SUB", "PREGEX_PTN_REFSUB",
								"PREGEX_PTN_ALT",
								"PREGEX_PTN_KLE", "PREGEX_PTN_POS",
									"PREGEX_PTN_OPT"
							};

	for( i = 0; i < rec; i++ )
		gap[i] = '.';
	gap[i] = 0;

	do
	{
		fprintf( stderr, "%s%s", gap, types[ ptn->type ] );

		if( ptn->type == PREGEX_PTN_CHAR )
			fprintf( stderr, " %s", pccl_to_str( ptn->ccl, FALSE ) );

		fprintf( stderr, "\n" );

		if( ptn->child[0] )
			pregex_ptn_print( ptn->child[0], rec + 1 );

		if( ptn->child[1] )
		{
			if( ptn->type == PREGEX_PTN_ALT )
				fprintf( stderr, "%s\n", gap );

			pregex_ptn_print( ptn->child[1], rec + 1 );
		}

		ptn = ptn->next;
	}
	while( ptn );
}

/* Internal function for pregex_ptn_to_regex() */
static void pregex_char_to_REGEX( char* str, int size,
 				wchar_t ch, pboolean escape, pboolean in_range )
{
	if( ( !in_range && ( strchr( "|()[]*+?", ch ) || ch == '.' ) ) ||
			( in_range && ch == ']' ) )
		sprintf( str, "\\%c", (char)ch );
	else if( escape )
		putf8_escape_wchar( str, size, ch );
	else
		putf8_toutf8( str, size, &ch, 1 );
}

/* Internal function for pregex_ptn_to_regex() */
static void pccl_to_REGEX( char** str, pccl* ccl )
{
	int				i;
	wchar_t			cfrom;
	char			from	[ 40 + 1 ];
	wchar_t			cto;
	char			to		[ 20 + 1 ];
	pboolean		range	= FALSE;
	pboolean		neg		= FALSE;
	pboolean		is_dup	= FALSE;

	/*
	 * If this caracter class contains PCCL_MAX characters, then simply
	 * print a dot.
	 */
	if( pccl_count( ccl ) > PCCL_MAX )
	{
		*str = pstrcatchar( *str, '.' );
		return;
	}

	/* Better negate if more than 128 chars */
	if( pccl_count( ccl ) > 128 )
	{
		ccl = pccl_dup( ccl );
		is_dup = TRUE;

		neg = TRUE;
		pccl_negate( ccl );
	}

	/* Char-class or single char? */
	if( neg || pccl_count( ccl ) > 1 )
	{
		range = TRUE;
		*str = pstrcatchar( *str, '[' );
		if( neg )
			*str = pstrcatchar( *str, '^' );
	}

	/*
	 * If ccl contains a dash,
	 * this must be printed first!
	 */
	if( pccl_test( ccl, '-' ) )
	{
		if( !is_dup )
		{
			ccl = pccl_dup( ccl );
			is_dup = TRUE;
		}

		*str = pstrcatchar( *str, '-' );
		pccl_del( ccl, '-' );
	}

	/* Iterate ccl... */
	for( i = 0; pccl_get( &cfrom, &cto, ccl, i ); i++ )
	{
		pregex_char_to_REGEX( from, (int)sizeof( from ), cfrom, TRUE, range );

		if( cfrom < cto )
		{
			pregex_char_to_REGEX( to, (int)sizeof( to ), cto, TRUE, range );
			sprintf( from + strlen( from ), "%s%s",
				cfrom + 1 < cto ? "-" : "", to );
		}

		*str = pstrcatstr( *str, from, FALSE );
	}

	if( range )
		*str = pstrcatchar( *str, ']' );

	if( is_dup )
		pccl_free( ccl );
}

/* Internal function for pregex_ptn_to_regex() */
static pboolean pregex_ptn_to_REGEX( char** regex, pregex_ptn* ptn )
{
	if( !( regex && ptn ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	while( ptn )
	{
		switch( ptn->type )
		{
			case PREGEX_PTN_NULL:
				return TRUE;

			case PREGEX_PTN_CHAR:
				pccl_to_REGEX( regex, ptn->ccl );
				break;

			case PREGEX_PTN_SUB:
			case PREGEX_PTN_REFSUB:
				*regex = pstrcatchar( *regex, '(' );

				if( !pregex_ptn_to_REGEX( regex, ptn->child[ 0 ] ) )
					return FALSE;

				*regex = pstrcatchar( *regex, ')' );
				break;

			case PREGEX_PTN_ALT:
				if( !pregex_ptn_to_REGEX( regex, ptn->child[ 0 ] ) )
					return FALSE;

				*regex = pstrcatchar( *regex, '|' );

				if( !pregex_ptn_to_REGEX( regex, ptn->child[ 1 ] ) )
					return FALSE;

				break;

			case PREGEX_PTN_KLE:

				if( !pregex_ptn_to_REGEX( regex, ptn->child[ 0 ] ) )
					return FALSE;

				*regex = pstrcatchar( *regex, '*' );
				break;

			case PREGEX_PTN_POS:

				if( !pregex_ptn_to_REGEX( regex, ptn->child[ 0 ] ) )
					return FALSE;

				*regex = pstrcatchar( *regex, '+' );
				break;

			case PREGEX_PTN_OPT:

				if( !pregex_ptn_to_REGEX( regex, ptn->child[ 0 ] ) )
					return FALSE;

				*regex = pstrcatchar( *regex, '?' );
				break;

			default:
				MISSINGCASE;
				return FALSE;
		}

		ptn = ptn->next;
	}

	return TRUE;
}

/** Turns a regular expression pattern back into a regular expression string.

//ptn// is the pattern object to be converted into a regex.

The returned pointer is dynamically allocated but part of //ptn//, so it should
not be freed by the caller. It is automatically freed when the pattern object
is released.
*/
char* pregex_ptn_to_regex( pregex_ptn* ptn )
{
	if( !( ptn ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	ptn->str = pfree( ptn->str );

	if( !pregex_ptn_to_REGEX( &ptn->str, ptn ) )
	{
		ptn->str = pfree( ptn->str );
		return (char*)NULL;
	}

	return ptn->str;
}

/* Internal recursive processing function for pregex_ptn_to_nfa() */
static pboolean pregex_ptn_to_NFA( pregex_nfa* nfa, pregex_ptn* pattern,
	pregex_nfa_st** start, pregex_nfa_st** end, int* ref_count )
{
	pregex_nfa_st*	n_start	= (pregex_nfa_st*)NULL;
	pregex_nfa_st*	n_end	= (pregex_nfa_st*)NULL;
	int				ref		= 0;

	if( !( pattern && nfa && start && end ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	*end = *start = (pregex_nfa_st*)NULL;

	while( pattern )
	{
		switch( pattern->type )
		{
			case PREGEX_PTN_NULL:
				return TRUE;

			case PREGEX_PTN_CHAR:
				n_start = pregex_nfa_create_state( nfa, (char*)NULL, 0 );
				n_end = pregex_nfa_create_state( nfa, (char*)NULL, 0 );

				n_start->ccl = pccl_dup( pattern->ccl );
				n_start->next = n_end;
				break;

			case PREGEX_PTN_REFSUB:
				if( !ref_count || ( ref = ++(*ref_count) ) > PREGEX_MAXREF )
					ref = 0;
				/* NO break! */

			case PREGEX_PTN_SUB:
				if( !pregex_ptn_to_NFA( nfa,
						pattern->child[ 0 ], &n_start, &n_end, ref_count ) )
					return FALSE;

				if( ref )
				{
					n_start->refs |= 1 << ref;
					n_end->refs |= 1 << ref;
				}

				break;

			case PREGEX_PTN_ALT:
			{
				pregex_nfa_st*	a_start;
				pregex_nfa_st*	a_end;

				n_start = pregex_nfa_create_state( nfa, (char*)NULL, 0 );
				n_end = pregex_nfa_create_state( nfa, (char*)NULL, 0 );

				if( !pregex_ptn_to_NFA( nfa,
						pattern->child[ 0 ], &a_start, &a_end, ref_count ) )
					return FALSE;

				n_start->next = a_start;
				a_end->next= n_end;

				if( !pregex_ptn_to_NFA( nfa,
						pattern->child[ 1 ], &a_start, &a_end, ref_count ) )
					return FALSE;

				n_start->next2 = a_start;
				a_end->next= n_end;
				break;
			}

			case PREGEX_PTN_KLE:
			case PREGEX_PTN_POS:
			case PREGEX_PTN_OPT:
			{
				pregex_nfa_st*	m_start;
				pregex_nfa_st*	m_end;

				n_start = pregex_nfa_create_state( nfa, (char*)NULL, 0 );
				n_end = pregex_nfa_create_state( nfa, (char*)NULL, 0 );

				if( !pregex_ptn_to_NFA( nfa,
						pattern->child[ 0 ], &m_start, &m_end, ref_count ) )
					return FALSE;

				/* Standard chain linking */
				n_start->next = m_start;
				m_end->next = n_end;

				switch( pattern->type )
				{
					case PREGEX_PTN_KLE:
						/*
								 ____________________________
								|                            |
								|                            v
							n_start -> m_start -> m_end -> n_end
										  ^         |
										  |_________|
						*/
						n_start->next2 = n_end;
						m_end->next2 = m_start;
						break;

					case PREGEX_PTN_POS:
						/*
								n_start -> m_start -> m_end -> n_end
											  ^         |
											  |_________|
						*/
						m_end->next2 = m_start;
						break;

					case PREGEX_PTN_OPT:
						/*
									_____________________________
								   |                             |
								   |                             v
								n_start -> m_start -> m_end -> n_end
						*/
						n_start->next2 = n_end;
						break;

					default:
						break;
				}
				break;
			}

			default:
				MISSINGCASE;
				return FALSE;
		}

		if( ! *start )
		{
			*start = n_start;
			*end = n_end;
		}
		else
		{
			/*
				In sequences, when start has already been assigned,
				end will always be the pointer to an epsilon node.
				So the previous end can be replaced by the current
				start, to minimize the usage of states.

				This only if the state does not contain any additional
				informations (e.g. references).
			*/
			if( !( (*end)->refs ) )
			{
				memcpy( *end, n_start, sizeof( pregex_nfa_st ) );

				plist_remove( nfa->states,
					plist_get_by_ptr( nfa->states, n_start ) );
			}
			else
				(*end)->next = n_start;

			*end = n_end;
		}

		pattern = pattern->next;
	}

	return TRUE;
}

/** Converts a pattern-structure into a NFA state machine.

//nfa// is the NFA state machine structure that receives the compiled result of
the pattern. This machine will be extended to the pattern if it already contains
states. //nfa// must be previously initialized!

//ptn// is the pattern structure that will be converted and extended into
the NFA state machine.

//flags// are compile-time flags.

Returns TRUE on success.
*/
pboolean pregex_ptn_to_nfa( pregex_nfa* nfa, pregex_ptn* ptn )
{
	pregex_nfa_st*	start;
	pregex_nfa_st*	end;
	pregex_nfa_st*	first;
	pregex_nfa_st*	n_first;
	int				ref_count	= 0;

	PROC( "pregex_ptn_to_nfa" );
	PARMS( "nfa", "%p", nfa );
	PARMS( "ptn", "%p", ptn );

	if( !( nfa && ptn ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	/* Find last first node ;) ... */
	for( n_first = (pregex_nfa_st*)plist_access( plist_first( nfa->states ) );
		n_first && n_first->next2; n_first = n_first->next2 )
			;

	/* Create first epsilon node */
	if( !( first = pregex_nfa_create_state( nfa, (char*)NULL, 0 ) ) )
		RETURN( FALSE );

	/* Turn pattern into NFA */
	if( !pregex_ptn_to_NFA( nfa, ptn, &start, &end, &ref_count ) )
		RETURN( FALSE );

	/* start is next of first */
	first->next = start;

	/* Chaining into big state machine */
	if( n_first )
		n_first->next2 = first;

	/* end becomes the accepting state */
	end->accept = ptn->accept;
	end->flags = ptn->flags;

	RETURN( TRUE );
}

/** Converts a pattern-structure into a DFA state machine.

//dfa// is the DFA state machine structure that receives the compiled result of
the pattern. //dfa// must be initialized!
//ptn// is the pattern structure that will be converted and extended into
the DFA state machine.

Returns TRUE on success.
*/
pboolean pregex_ptn_to_dfa( pregex_dfa* dfa, pregex_ptn* ptn )
{
	pregex_nfa*	nfa;

	PROC( "pregex_ptn_to_dfa" );
	PARMS( "dfa", "%p", dfa );
	PARMS( "ptn", "%p", ptn );

	if( !( dfa && ptn ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	pregex_dfa_reset( dfa );
	nfa = pregex_nfa_create();

	if( !pregex_ptn_to_nfa( nfa, ptn ) )
	{
		pregex_nfa_free( nfa );
		RETURN( FALSE );
	}

	if( pregex_dfa_from_nfa( dfa, nfa ) < 0 )
	{
		pregex_nfa_free( nfa );
		RETURN( FALSE );
	}

	pregex_nfa_free( nfa );

	if( pregex_dfa_minimize( dfa ) < 0 )
	{
		pregex_dfa_free( dfa );
		RETURN( FALSE );
	}

	RETURN( TRUE );
}

/** Converts a pattern-structure into a DFA state machine //dfatab//.

//ptn// is the pattern structure that will be converted into a DFA state
machine.

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

Returns the number of rows in //dfatab//, or a negative value in error case.
*/
int pregex_ptn_to_dfatab( wchar_t*** dfatab, pregex_ptn* ptn )
{
	pregex_dfa*	dfa;
	int			states;

	PROC( "pregex_ptn_to_dfatab" );
	PARMS( "dfatab", "%p", dfatab );
	PARMS( "ptn", "%p", ptn );

	if( !( ptn ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	dfa = pregex_dfa_create();

	if( !pregex_ptn_to_dfa( dfa, ptn ) )
	{
		pregex_dfa_free( dfa );
		RETURN( FALSE );
	}

	if( ( states = pregex_dfa_to_dfatab( dfatab, dfa ) ) < 0 )
	{
		pregex_dfa_free( dfa );
		RETURN( FALSE );
	}

	pregex_dfa_free( dfa );

	VARS( "states", "%d", states );
	RETURN( states );
}

/** Parse a regular expression pattern string into a pregex_ptn structure.

//ptn// is the return pointer receiving the root node of the generated pattern.

//str// is the pointer to the string which contains the pattern to be parsed. If
PREGEX_COMP_WCHAR is assigned in //flags//, this pointer must be set to a
wchar_t-array holding a wide-character string.

//flags// provides compile-time modifier flags (PREGEX_COMP_...).

Returns TRUE on success.
*/
pboolean pregex_ptn_parse( pregex_ptn** ptn, char* str, int flags )
{
	char*			ptr;
	int				aflags	= 0;

	PROC( "pregex_ptn_parse" );
	PARMS( "ptn", "%p", ptn );
	PARMS( "str", "%s", str );
	PARMS( "flags", "%d", flags );

	if( !( ptn && str ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	/* If PREGEX_COMP_STATIC is set, parsing is not required! */
	if( flags & PREGEX_COMP_STATIC )
	{
		MSG( "PREGEX_COMP_STATIC" );

		if( !( *ptn = pregex_ptn_create_string( str, flags ) ) )
			RETURN( FALSE );

		RETURN( TRUE );
	}

	/* Copy input string - this is required,
		because of memory modification during the parse */
	if( flags & PREGEX_COMP_WCHAR )
	{
		if( !( ptr = str = pwcs_to_str( (wchar_t*)str, FALSE ) ) )
			RETURN( FALSE );
	}
	else
	{
		if( !( ptr = str = pstrdup( str ) ) )
			RETURN( FALSE );
	}

	VARS( "ptr", "%s", ptr );

	/* Parse anchor at begin of regular expression */
	if( !( flags & PREGEX_COMP_NOANCHORS ) )
	{
		MSG( "Anchors at begin" );

		if( *ptr == '^' )
		{
			aflags |= PREGEX_FLAG_BOL;
			ptr++;
		}
		else if( !strncmp( ptr, "\\<", 2 ) )
			/* This is a GNU-like extension */
		{
			aflags |= PREGEX_FLAG_BOW;
			ptr += 2;
		}
	}

	/* Run the recursive descent parser */
	MSG( "Starting the parser" );
	VARS( "ptr", "%s", ptr );

	if( !parse_alter( ptn, &ptr, flags ) )
		RETURN( FALSE );

	VARS( "ptr", "%s", ptr );

	/* Parse anchor at end of regular expression */
	if( !( flags & PREGEX_COMP_NOANCHORS ) )
	{
		MSG( "Anchors at end" );
		if( !strcmp( ptr, "$" ) )
			aflags |= PREGEX_FLAG_EOL;
		else if( !strcmp( ptr, "\\>" ) )
			/* This is a GNU-style extension */
			aflags |= PREGEX_FLAG_EOW;
	}

	/* Force nongreedy matching */
	if( flags & PREGEX_COMP_NONGREEDY )
		aflags |= PREGEX_FLAG_NONGREEDY;

	/* Free duplicated string */
	pfree( str );

	/* Assign flags */
	(*ptn)->flags = aflags;

	RETURN( TRUE );
}

/******************************************************************************
 *      RECURSIVE DESCENT PARSER FOR REGULAR EXPRESSIONS FOLLOWS HERE...      *
 ******************************************************************************/

static pboolean parse_char( pregex_ptn** ptn, char** pstr, int flags )
{
	pccl*		ccl;
	pregex_ptn*	alter;
	wchar_t		single;
	char		restore;
	char*		zero;
	pboolean	neg		= FALSE;

	PROC( "parse_char" );
	VARS( "**pstr", "%c", **pstr );

	switch( **pstr )
	{
		case '(':
			MSG( "Sub expression" );
			(*pstr)++;

			if( !parse_alter( &alter, pstr, flags ) )
				RETURN( FALSE );

			if( flags & PREGEX_COMP_NOREF &&
					!( *ptn = pregex_ptn_create_sub( alter ) ) )
				RETURN( FALSE );
			else if( !( *ptn = pregex_ptn_create_refsub( alter ) ) )
				RETURN( FALSE );

			/* Report error? */
			if( **pstr != ')' && !( flags & PREGEX_COMP_NOERRORS ) )
			{
				fprintf( stderr, "Missing closing bracket in regex\n" );
				RETURN( FALSE );
			}

			(*pstr)++;
			break;

		case '.':
			MSG( "Any character" );

			ccl = pccl_create( -1, -1, (char*)NULL );

			if( !( ccl && pccl_addrange( ccl, PCCL_MIN, PCCL_MAX ) ) )
			{
				pccl_free( ccl );
				RETURN( FALSE );
			}

			if( !( *ptn = pregex_ptn_create_char( ccl ) ) )
			{
				pccl_free( ccl );
				RETURN( FALSE );
			}

			(*pstr)++;
			break;

		case '[':
			MSG( "Character-class definition" );

			/* Find next UNESCAPED! closing bracket! */
			for( zero = *pstr + 1; *zero && *zero != ']'; zero++ )
				if( *zero == '\\' && *(zero + 1) )
					zero++;

			if( *zero )
			{
				restore = *zero;
				*zero = '\0';

				if( (*pstr) + 1 < zero && *((*pstr)+1) == '^' )
				{
					neg = TRUE;
					(*pstr)++;
				}

				if( !( ccl = pccl_create( -1, -1, (*pstr) + 1 ) ) )
					RETURN( FALSE );

				if( neg )
					pccl_negate( ccl );

				if( !( *ptn = pregex_ptn_create_char( ccl ) ) )
				{
					pccl_free( ccl );
					RETURN( FALSE );
				}

				*zero = restore;
				*pstr = zero + 1;
				break;
			}
			/* No break here! */

		default:
			MSG( "Default case" );

			if( !( ccl = pccl_create( -1, -1, (char*)NULL ) ) )
			{
				OUTOFMEM;
				RETURN( FALSE );
			}

			if( !pccl_parseshorthand( ccl, pstr ) )
			{
				*pstr += pccl_parsechar( &single, *pstr, TRUE );

				MSG( "Singe character" );
				VARS( "single", "%c", single );
				VARS( "**pstr", "%c", **pstr );

				if( !pccl_add( ccl, single ) )
					RETURN( FALSE );
			}
			else
			{
				MSG( "Shorthand parsed" );
				VARS( "**pstr", "%c", **pstr );
			}

			VARS( "ccl", "%s", pccl_to_str( ccl, TRUE ) );

			if( !( *ptn = pregex_ptn_create_char( ccl ) ) )
			{
				pccl_free( ccl );
				RETURN( FALSE );
			}

			break;
	}

	RETURN( TRUE );
}

static pboolean parse_factor( pregex_ptn** ptn, char** pstr, int flags )
{
	PROC( "parse_factor" );

	if( !parse_char( ptn, pstr, flags ) )
		RETURN( FALSE );

	VARS( "**pstr", "%c", **pstr );

	switch( **pstr )
	{
		case '*':
		case '+':
		case '?':

			MSG( "Modifier matched" );

			switch( **pstr )
			{
				case '*':
					MSG( "Kleene star" );
					*ptn = pregex_ptn_create_kle( *ptn );
					break;
				case '+':
					MSG( "Positive plus" );
					*ptn = pregex_ptn_create_pos( *ptn );
					break;
				case '?':
					MSG( "Optional quest" );
					*ptn = pregex_ptn_create_opt( *ptn );
					break;

				default:
					break;
			}

			if( ! *ptn )
				RETURN( FALSE );

			(*pstr)++;
			break;

		default:
			break;
	}

	RETURN( TRUE );
}

static pboolean parse_sequence( pregex_ptn** ptn, char** pstr, int flags )
{
	pregex_ptn*	next;

	PROC( "parse_sequence" );

	if( !parse_factor( ptn, pstr, flags ) )
		RETURN( FALSE );

	while( !( **pstr == '|' || **pstr == ')' || **pstr == '\0' ) )
	{
		if( !( flags & PREGEX_COMP_NOANCHORS ) )
		{
			if( !strcmp( *pstr, "$" ) || !strcmp( *pstr, "\\>" ) )
				break;
		}

		if( !parse_factor( &next, pstr, flags ) )
			RETURN( FALSE );

		*ptn = pregex_ptn_create_seq( *ptn, next, (pregex_ptn*)NULL );
	}

	RETURN( TRUE );
}

static pboolean parse_alter( pregex_ptn** ptn, char** pstr, int flags )
{
	pregex_ptn*	seq;

	PROC( "parse_alter" );

	if( !parse_sequence( ptn, pstr, flags ) )
		RETURN( FALSE );

	while( **pstr == '|' )
	{
		MSG( "Alternative" );
		(*pstr)++;

		if( !parse_sequence( &seq, pstr, flags ) )
			RETURN( FALSE );

		if( !( *ptn = pregex_ptn_create_alt( *ptn, seq, (pregex_ptn*)NULL ) ) )
			RETURN( FALSE );
	}

	RETURN( TRUE );
}

