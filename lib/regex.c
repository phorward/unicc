/* -MODULE----------------------------------------------------------------------
Phorward C/C++ Library
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
https://phorward.info ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	pregex.c
Author:	Jan Max Meyer
Usage:	Interface for pregex-objects serving regular expressions.
----------------------------------------------------------------------------- */

#include "phorward.h"

/** Constructor function to create a new pregex object.

//pat// is a string providing a regular expression pattern.
//flags// can be a combination of compile- and runtime-flags.

|| Flag | Usage |
| PREGEX_COMP_WCHAR | The regular expression //pat// is provided as wchar_t. |
| PREGEX_COMP_NOANCHORS | Ignore anchor tokens, handle them as normal \
characters |
| PREGEX_COMP_NOREF | Don't compile references. |
| PREGEX_COMP_NONGREEDY | Compile regex to be forced non-greedy. |
| PREGEX_COMP_NOERRORS | Don't report errors, and try to compile as much as \
possible |
| PREGEX_COMP_INSENSITIVE | Parse regular expression as case insensitive. |
| PREGEX_COMP_STATIC | The regular expression passed should be converted 1:1 \
as it where a string-constant. Any regex-specific symbols will be ignored and \
taken as they where escaped. |
| PREGEX_RUN_WCHAR | Run regular expression with wchar_t as input. |
| PREGEX_RUN_NOANCHORS | Ignore anchors while processing the regex. |
| PREGEX_RUN_NOREF | Don't create references. |
| PREGEX_RUN_NONGREEDY | Force run regular expression non-greedy. |
| PREGEX_RUN_DEBUG | Debug mode; output some debug to stderr. |


On success, the function returns the allocated pointer to a pregex-object.
This must be freed later using pregex_free().
*/
pregex* pregex_create( char* pat, int flags )
{
	pregex*			regex;
	pregex_ptn*		ptn;

	PROC( "pregex_create" );
	PARMS( "pat", "%s", pat );
	PARMS( "flags", "%d", flags );

	if( !pregex_ptn_parse( &ptn, pat, flags ) )
		RETURN( (pregex*)NULL );

	ptn->accept = 1;

	regex = (pregex*)pmalloc( sizeof( pregex ) );
	regex->ptn = ptn;
	regex->flags = flags;

	/* Generate a dfatab */
	if( ( regex->trans_cnt = pregex_ptn_to_dfatab( &regex->trans, ptn ) ) < 0 )
		RETURN( pregex_free( regex ) );

	/* Print dfatab */
	/* pregex_ptn_to_dfatab( (wchar_t***)NULL, ptn ); */

	RETURN( regex );
}

/** Destructor function for a pregex-object.

//regex// is the pointer to a pregex-structure that will be released.

Returns always (pregex*)NULL.
*/
pregex* pregex_free( pregex* regex )
{
	int		i;

	PROC( "pregex_free" );
	PARMS( "regex", "%p", regex );

	if( !regex )
		RETURN( (pregex*)NULL );

	/* Freeing the pattern definitions */
	regex->ptn = pregex_ptn_free( regex->ptn );

	/* Drop out the dfatab */
	for( i = 0; i < regex->trans_cnt; i++ )
		pfree( regex->trans[ i ] );

	pfree( regex->trans );
	pfree( regex );

	RETURN( (pregex*)NULL );
}

/** Tries to match the regular expression //regex// at pointer //start//.

If the expression can be matched, the function returns TRUE and //end// receives
the pointer to the last matched character. */
pboolean pregex_match( pregex* regex, char* start, char** end )
{
	int		i;
	int		state		= 0;
	int		next_state;
	char*	match		= (char*)NULL;
	char*	ptr			= start;
	wchar_t	ch;

	PROC( "pregex_match" );
	PARMS( "regex", "%p", regex );
	PARMS( "start", "%s", start );
	PARMS( "end", "%p", end );

	if( !( regex && start ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	memset( regex->ref, 0, PREGEX_MAXREF * sizeof( prange ) );

	while( state >= 0 )
	{
		/* State accepts? */
		if( regex->trans[ state ][ 1 ] )
		{
			MSG( "This state accepts the input" );
			match = ptr;

			if( ( regex->flags & PREGEX_RUN_NONGREEDY
					|| regex->trans[ state ][ 2 ] & PREGEX_FLAG_NONGREEDY ) )
			{
				if( regex->flags & PREGEX_RUN_DEBUG )
					fprintf( stderr,
						"state %d accepted %d, end of recognition\n",
							state, regex->trans[ state ][ 1 ] );

				break;
			}
		}

		/* References */
		if( regex->trans[ state ][ 3 ] )
		{
			for( i = 0; i < PREGEX_MAXREF; i++ )
			{
				if( regex->trans[ state ][ 3 ] & ( 1 << i ) )
				{
					if( !regex->ref[ i ].start )
						regex->ref[ i ].start = ptr;

					regex->ref[ i ].end = ptr;
				}
			}
		}

		/* Get next character */
		if( regex->flags & PREGEX_RUN_WCHAR )
		{
			VARS( "pstr", "%ls", (wchar_t*)ptr );
			ch = *( (wchar_t*)ptr );
			ptr += sizeof( wchar_t );
		}
		else
		{
			VARS( "pstr", "%s", ptr );

			if( ( regex->flags & PREGEX_RUN_UCHAR ) )
				ch = (unsigned char)*ptr++;
			else
			{
#ifdef UTF8
				ch = putf8_char( ptr );
				ptr += putf8_seqlen( ptr );
#else
				ch = *ptr++;
#endif
			}
		}

		if( !ch )
			break;

		/* Initialize default transition */
		next_state = regex->trans[ state ][ 4 ];

		/* Find transition according to current character */
		for( i = 5; i < regex->trans[ state ][ 0 ]; i += 3 )
		{
			if( regex->trans[ state ][ i ] <= ch
					&& regex->trans[ state ][ i + 1 ] >= ch )
			{
				next_state = regex->trans[ state ][ i + 2 ];
				break;
			}
		}

		if( next_state == regex->trans_cnt )
			break;

		if( regex->flags & PREGEX_RUN_DEBUG )
		{
			if( regex->flags & PREGEX_RUN_WCHAR )
				fprintf( stderr,
					"state %d, wchar_t %d (>%lc<), next state %d\n",
						state, ch, ch, next_state );
			else
				fprintf( stderr,
					"state %d, char %d (>%c<), next state %d\n",
						state, ch, ch, next_state );
		}

		state = next_state;
	}

	if( match )
	{
		if( end )
			*end = match;

		/*
		for( i = 0; i < PREGEX_MAXREF; i++ )
			if( regex->ref[ i ].start )
				fprintf( stderr, "%2d: >%.*s<\n",
					i, regex->ref[ i ].end - regex->ref[ i ].start,
						regex->ref[ i ].start );
		*/

		RETURN( TRUE );
	}

	RETURN( FALSE );
}

/** Find a match for the regular expression //regex// from begin of pointer
//start//.

//start// has to be a zero-terminated string or wide-character string (according
to the configuration of the pregex-object).

If the expression can be matched, the function returns the pointer to the
position where the match begins. //end// receives the end pointer of the match,
when provided.

The function returns (char*)NULL in case that there is no match.
*/
char* pregex_find( pregex* regex, char* start, char** end )
{
	wchar_t		ch;
	char*		ptr 	= start;
	char*		lptr;
	int			i;

	PROC( "pregex_find" );
	PARMS( "regex", "%p", regex );
	PARMS( "start", "%s", start );
	PARMS( "end", "%p", end );

	if( !( regex && start ) )
	{
		WRONGPARAM;
		RETURN( (char*)NULL );
	}

	do
	{
		lptr = ptr;

		/* Get next character */
		if( regex->flags & PREGEX_RUN_WCHAR )
		{
			ch = *( (wchar_t*)ptr );
			ptr += sizeof( wchar_t );
		}
		else
		{
			if( ( regex->flags & PREGEX_RUN_UCHAR ) )
				ch = (unsigned char)*ptr++;
			else
			{
#ifdef UTF8
				ch = putf8_char( ptr );
				ptr += putf8_seqlen( ptr );
#else
				ch = *ptr++;
#endif
			}
		}

		if( !ch )
			break;

		/* Find a transition according to current character */
		for( i = 5; i < regex->trans[ 0 ][ 0 ]; i += 3 )
			if( ( ( regex->trans[ 0 ][ 4 ] < regex->trans_cnt )
					|| ( regex->trans[ 0 ][ i ] <= ch
							&& regex->trans[ 0 ][ i + 1 ] >= ch ) )
					&& pregex_match( regex, lptr, end ) )
						RETURN( lptr );
	}
	while( ch );

	RETURN( (char*)NULL );
}

/** Find all matches for the regular expression //regex// from begin of pointer
//start//, and optionally return matches as an array.

//start// has to be a zero-terminated string or wide-character string (according
to the configuration of the pregex-object).

The function fills the array //matches//, if provided, with items of size
prange. It returns the total number of matches.
*/
int pregex_findall( pregex* regex, char* start, parray** matches )
{
	char*			end;
	int				count	= 0;
	prange*	r;

	PROC( "pregex_findall" );
	PARMS( "regex", "%p", regex );
	PARMS( "start", "%s", start );
	PARMS( "matches", "%p", matches );

	if( !( regex && start ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	if( matches )
		*matches = (parray*)NULL;

	while( ( start = pregex_find( regex, start, &end ) ) )
	{
		if( matches )
		{
			if( ! *matches )
				*matches = parray_create( sizeof( prange ), 0 );

			r = (prange*)parray_malloc( *matches );
			r->id = 1;
			r->start = start;
			r->end = end;
		}

		start = end;
		count++;
	}

	RETURN( count );
}

/** Returns the range between string //start// and the next match of //regex//.

This function can be seen as a "negative match", so the substrings that are
not part of the match will be returned.

//start// has to be a zero-terminated string or wide-character string (according
to the configuration of the pregex-object).
//end// receives the last position of the string before the regex.
//next// receives the pointer of the next split element behind the matched
substring, so //next// should become the next //start// when pregex_split() is
called in a loop.

The function returns (char*)NULL in case there is no more string to split, else
it returns //start//.
*/
char* pregex_split( pregex* regex, char* start, char** end, char** next )
{
	wchar_t		ch;
	char*		ptr 	= start;
	char*		lptr;
	int			i;

	PROC( "pregex_split" );
	PARMS( "regex", "%p", regex );
	PARMS( "start", "%s", start );
	PARMS( "end", "%p", end );

	if( !( regex && start ) )
	{
		WRONGPARAM;
		RETURN( (char*)NULL );
	}

	if( next )
		*next = (char*)NULL;

	do
	{
		lptr = ptr;

		/* Get next character */
		if( regex->flags & PREGEX_RUN_WCHAR )
		{
			ch = *( (wchar_t*)ptr );
			ptr += sizeof( wchar_t );
		}
		else
		{
			if( ( regex->flags & PREGEX_RUN_UCHAR ) )
				ch = (unsigned char)*ptr++;
			else
			{
#ifdef UTF8
				ch = putf8_char( ptr );
				ptr += putf8_seqlen( ptr );
#else
				ch = *ptr++;
#endif
			}
		}

		if( !ch )
			break;

		/* Find a transition according to current character */
		for( i = 5; i < regex->trans[ 0 ][ 0 ]; i += 3 )
			if( regex->trans[ 0 ][ i ] <= ch
				&& regex->trans[ 0 ][ i + 1 ] >= ch
					&& pregex_match( regex, lptr, next ) )
						ch = 0;
	}
	while( ch );

	if( lptr > start )
	{
		if( end )
			*end = lptr;

		RETURN( start );
	}

	RETURN( (char*)NULL );
}

/** Split a string at all matches of the regular expression //regex// from
begin of pointer //start//, and optionally returns the substrings found as an
array.

//start// has to be a zero-terminated string or wide-character string (according
to the configuration of the pregex-object).

The function fills the array //matches//, if provided, with items of size
prange. It returns the total number of matches.
*/
int pregex_splitall( pregex* regex, char* start, parray** matches )
{
	char*			end;
	char*			next;
	int				count	= 0;
	prange*	r;

	PROC( "pregex_splitall" );
	PARMS( "regex", "%p", regex );
	PARMS( "start", "%s", start );
	PARMS( "matches", "%p", matches );

	if( !( regex && start ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	if( matches )
		*matches = (parray*)NULL;

	while( start )
	{
		if( pregex_split( regex, start, &end, &next ) && matches )
		{
			if( ! *matches )
				*matches = parray_create( sizeof( prange ), 0 );

			r = (prange*)parray_malloc( *matches );
			r->start = start;
			r->end = end;
		}

		count++;

		start = next;
	}

	RETURN( count );
}

/** Replaces all matches of a regular expression object within a string //str//
with //replacement//. Backreferences in //replacement// can be used with //$x//
for each opening bracket within the regular expression.

//regex// is the pregex-object used for pattern matching.
//str// is the string on which //regex// will be executed.
//replacement// is the string that will be inserted as the replacement for each
match of a pattern described in //regex//. The notation //$x// can be used for
backreferences, where x is the offset of opening brackets in the pattern,
beginning at 1.

The function returns the string with the replaced elements, or (char*)NULL
in case of an error.
*/
char* pregex_replace( pregex* regex, char* str, char* replacement )
{
	prange*	refer;
	char*			ret			= (char*)NULL;
	char*			sstart		= str;
	char*			start;
	char*			end;
	char*			replace;
	char*			rprev;
	char*			rpstr;
	char*			rstart;
	int				ref;

	PROC( "pregex_replace" );
	PARMS( "regex", "%p", regex );

	if( !( regex && str ) )
	{
		WRONGPARAM;
		RETURN( (char*)NULL );
	}

#ifdef DEBUG
	if( regex && regex->flags & PREGEX_RUN_WCHAR )
	{
		PARMS( "str", "%ls", pstrget( str ) );
		PARMS( "replacement", "%ls", pstrget( replacement ) );
	}
	else
	{
		PARMS( "str", "%s", pstrget( str ) );
		PARMS( "replacement", "%s", pstrget( replacement ) );
	}
#endif

	MSG( "Starting loop" );

	while( TRUE )
	{
		if( !( start = pregex_find( regex, sstart, &end ) ) )
		{
			if( regex->flags & PREGEX_RUN_WCHAR )
				start = (char*)( (wchar_t*)sstart +
									wcslen( (wchar_t*)sstart ) );
			else
				start = sstart + strlen( sstart );

			end = (char*)NULL;
		}

		if( start > sstart )
		{
			MSG( "Extending string" );
			if( regex->flags & PREGEX_RUN_WCHAR )
			{
				if( !( ret = (char*)pwcsncatstr(
								(wchar_t*)ret, (wchar_t*)sstart,
									( (wchar_t*)start - (wchar_t*)sstart ) ) ) )
				{
					OUTOFMEM;
					RETURN( (char*)NULL );
				}
			}
			else
			{
				if( !( ret = pstrncatstr( ret, sstart, start - sstart ) ) )
				{
					OUTOFMEM;
					RETURN( (char*)NULL );
				}
			}
		}

		if( !end )
			break;

		MSG( "Constructing a replacement" );
		if( !( regex->flags & PREGEX_RUN_NOREF ) )
		{
			rprev = rpstr = replacement;
			replace = (char*)NULL;

			while( *rpstr )
			{
				VARS( "*rpstr", "%c", *rpstr );
				if( *rpstr == '$' )
				{
					rstart = rpstr;

					if( regex->flags & PREGEX_RUN_WCHAR )
					{
						wchar_t*		_end;
						wchar_t*		_rpstr = (wchar_t*)rpstr;

						MSG( "Switching to wide-character mode" );

						if( iswdigit( *( ++_rpstr ) ) )
						{
							ref = wcstol( _rpstr, &_end, 0 );

							VARS( "ref", "%d", ref );
							VARS( "end", "%ls", _end );

							/* Skip length of the number */
							_rpstr = _end;

							/* Extend first from prev of replacement */
							if( !( replace = (char*)pwcsncatstr(
									(wchar_t*)replace, (wchar_t*)rprev,
										(wchar_t*)rstart - (wchar_t*)rprev ) ) )
							{
								OUTOFMEM;
								RETURN( (char*)NULL );
							}

							VARS( "replace", "%ls", replace );
							VARS( "ref", "%d", ref );

							if( ref > 0 && ref < PREGEX_MAXREF )
								refer = &( regex->ref[ ref ] );
							/* TODO: Ref $0 */
							else
								refer = (prange*)NULL;

							if( refer )
							{
								MSG( "There is a reference!" );
								VARS( "refer->start", "%ls",
										(wchar_t*)refer->start );
								VARS( "refer->end", "%ls",
										(wchar_t*)refer->start );

								if( !( replace = (char*)pwcsncatstr(
										(wchar_t*)replace,
											(wchar_t*)refer->start,
												(wchar_t*)refer->end -
													(wchar_t*)refer->start ) ) )
								{
									OUTOFMEM;
									RETURN( (char*)NULL );
								}
							}

							VARS( "replace", "%ls", (wchar_t*)replace );
							rprev = rpstr = (char*)_rpstr;
						}
						else
							rpstr = (char*)_rpstr;
					}
					else
					{
						char*		_end;

						MSG( "Byte-character mode (Standard)" );

						if( isdigit( *( ++rpstr ) ) )
						{
							ref = strtol( rpstr, &_end, 0 );

							VARS( "ref", "%d", ref );
							VARS( "end", "%s", _end );

							/* Skip length of the number */
							rpstr = _end;

							/* Extend first from prev of replacement */
							if( !( replace = pstrncatstr( replace,
										rprev, rstart - rprev ) ) )
							{
								OUTOFMEM;
								RETURN( (char*)NULL );
							}

							VARS( "replace", "%s", replace );
							VARS( "ref", "%d", ref );

							if( ref > 0 && ref < PREGEX_MAXREF )
								refer = &( regex->ref[ ref ] );
							/* TODO: Ref $0 */
							else
								refer = (prange*)NULL;

							if( refer )
							{
								MSG( "There is a reference!" );
								VARS( "refer->start", "%ls",
										(wchar_t*)refer->start );
								VARS( "refer->end", "%ls",
										(wchar_t*)refer->end );

								if( !( replace = (char*)pstrncatstr(
											replace, refer->start,
												refer->end - refer->start ) ) )
								{
									OUTOFMEM;
									RETURN( (char*)NULL );
								}
							}

							VARS( "replace", "%s", replace );
							rprev = rpstr;
						}
					}
				}
				else
					rpstr += ( regex->flags & PREGEX_RUN_WCHAR )
								? sizeof( wchar_t ) : 1;
			}

			VARS( "rpstr", "%p", rpstr );
			VARS( "rprev", "%p", rprev );

#ifdef DEBUG
			if( regex->flags & PREGEX_RUN_WCHAR )
			{
				VARS( "rpstr", "%ls", (wchar_t*)replace );
				VARS( "rprev", "%ls", (wchar_t*)rprev );
			}
			else
			{
				VARS( "rpstr", "%s", replace );
				VARS( "rprev", "%s", rprev );
			}
#endif

			if( regex->flags & PREGEX_RUN_WCHAR )
			{
				if( rpstr != rprev &&
						!( replace = (char*)pwcscatstr(
								(wchar_t*)replace, (wchar_t*)rprev, FALSE ) ) )
				{
					OUTOFMEM;
					RETURN( (char*)NULL );
				}
			}
			else
			{
				if( rpstr != rprev && !( replace = pstrcatstr(
											replace, rprev, FALSE ) ) )
				{
					OUTOFMEM;
					RETURN( (char*)NULL );
				}
			}
		}
		else
			replace = replacement;

		if( replace )
		{
#ifdef DEBUG
			if( regex->flags & PREGEX_RUN_WCHAR )
				VARS( "replace", "%ls", (wchar_t*)replace );
			else
				VARS( "replace", "%s", replace );
#endif
			if( regex->flags & PREGEX_RUN_WCHAR )
			{
				if( !( ret = (char*)pwcsncatstr(
						(wchar_t*)ret, (wchar_t*)replace,
							pwcslen( (wchar_t*)replace ) ) ) )
				{
					OUTOFMEM;
					RETURN( (char*)NULL );
				}
			}
			else
			{
				if( !( ret = pstrncatstr( ret, replace, pstrlen( replace ) ) ) )
				{
					OUTOFMEM;
					RETURN( (char*)NULL );
				}
			}

			if( !( regex->flags & PREGEX_RUN_NOREF ) )
				pfree( replace );
		}

		sstart = end;
	}

#ifdef DEBUG
	if( regex->flags & PREGEX_RUN_WCHAR )
		VARS( "ret", "%ls", (wchar_t*)ret );
	else
		VARS( "ret", "%s", ret );
#endif

	RETURN( ret );
}
