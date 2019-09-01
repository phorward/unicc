/* -MODULE----------------------------------------------------------------------
Phorward C/C++ Library
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
https://phorward.info ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	direct.c
Author:	Jan Max Meyer
Usage:	Direct regular expression access functions running an internal pregex
		object.
----------------------------------------------------------------------------- */

#include "phorward.h"

/** Performs a regular expression match on a string, and returns an array of
matches via prange-structures, which hold pointers to the begin- and
end-addresses of all matches.

//regex// is the regular expression pattern to be processed.

//str// is the string on which the pattern will be executed.

//flags// are for regular expression compile- and runtime-mode switching.
Several of them can be used with the bitwise or-operator (|).

//matches// is the array of results to the matched substrings within //str//,
provided as parray-object existing of one prange-object for every match.
It is optional. //matches// must be released with parray_free() after its usage.

Returns the number of matches, which is the number of result entries in the
returned array //matches//. If the value is negative, an error occurred.
*/
int pregex_qmatch( char* regex, char* str, int flags, parray** matches )
{
	int			count;
	pregex*		re;

	PROC( "pregex_qmatch" );
	PARMS( "regex", "%s", pstrget( regex ) );
	PARMS( "str", "%s", pstrget( str ) );
	PARMS( "flags", "%d", flags );
	PARMS( "matches", "%p", matches );

	if( !( regex && str ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	if( !( re = pregex_create( regex, flags ) ) )
		RETURN( -1 );

	count = pregex_findall( re, str, matches );
	pregex_free( re );

	VARS( "count", "%d", count );
	RETURN( count );
}

/** Performs a regular expression search on a string and uses the expression as
separator; All strings that where split are returned as //matches//-array.

//regex// is the regular expression pattern to be processed.

//str// is the string on which the pattern will be executed.

//flags// are for regular expression compile- and runtime-mode switching.
Several of them can be used with the bitwise or-operator (|).

//matches// is the array of results to the matched substrings within //str//,
provided as parray-object existing of one prange-object for every match.
It is optional. //matches// must be released with parray_free() after its usage.

Returns the number of split substrings, which is the number of result entries in
the returned array //matches//. If the value is negative, an error occured.
*/
int pregex_qsplit( char* regex, char* str, int flags, parray** matches )
{
	int			count;
	pregex*		re;

	PROC( "pregex_qsplit" );
	PARMS( "regex", "%s", pstrget( regex ) );
	PARMS( "str", "%s", pstrget( str ) );
	PARMS( "flags", "%d", flags );
	PARMS( "matches", "%p", matches );

	if( !( regex && str ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	if( !( re = pregex_create( regex, flags ) ) )
		RETURN( -1 );

	count = pregex_splitall( re, str, matches );
	pregex_free( re );

	VARS( "count", "%d", count );
	RETURN( count );
}

/** Replaces all matches of a regular expression pattern within a string with
the replacement. Backreferences can be used with ``$x`` for each opening bracket
within the regular expression.

//regex// is the regular expression pattern to be processed.

//str// is the string on which the pattern will be executed.

//replace// is the string that will be inserted as replacement for each pattern
match. ``$x`` back-references can be used.

//flags// are for regular expression compile- and runtime-mode switching.
Several of them can be used with the bitwise or-operator (|).

Returns an allocated pointer to the generated string with the replacements.
This string must be released after its existence is no longer required by the
caller using pfree().
*/
char* pregex_qreplace( char* regex, char* str, char* replace, int flags )
{
	char*			ret;
	pregex*			re;

	PROC( "pregex_qreplace" );
	PARMS( "regex", "%s", pstrget( regex ) );
	PARMS( "str", "%s", pstrget( str ) );
	PARMS( "replace", "%s", pstrget( replace ) );
	PARMS( "flags", "%d", flags );

	if( !( regex && str ) )
	{
		WRONGPARAM;
		RETURN( (char*)NULL );
	}

	if( !( re = pregex_create( regex, flags ) ) )
	{
		pregex_free( re );
		RETURN( (char*)NULL );
	}

	if( !( ret = pregex_replace( re, str, replace ) ) )
	{
		pregex_free( re );
		RETURN( (char*)NULL );
	}

	pregex_free( re );

	VARS( "ret", "%s", ret );
	RETURN( ret );
}
