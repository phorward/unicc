/* -MODULE----------------------------------------------------------------------
Phorward C/C++ Library
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
https://phorward.info ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	misc.c
Author:	Jan Max Meyer
Usage:	Internal utility and miscelleanous functions for additional usage that
		belong to the regex library.
----------------------------------------------------------------------------- */

#include "phorward.h"

/*NO_DOC*/
/* No documentation for the entire module, all here is only interally used. */

/** Performs an anchor checking within a string. It is used by the internal
matching functions for NFA and DFA state machines.

//all// is the entire string. This can be equal to //str//, but is required to
perform valid line-begin anchor checking. If //all// is (char*)NULL, //str//
is assumed to be //all//.
//str// is the position pointer of the current match within //all//.
//len// is the length of the matched string, in characters.
//anchors// is the anchor configuration to be checked for the string.
//flags// is the flags configuration, e. g. for wide-character enabled anchor
checking.

Returns TRUE, if all anchors flagged as //anchors// match, else FALSE.
*/
pboolean pregex_check_anchors( char* all, char* str, size_t len,
										int anchors, int flags )
{
	wchar_t	ch;
	int		charsize = sizeof( char );

	PROC( "pregex_check_anchors" );
	if( flags & PREGEX_RUN_WCHAR )
	{
		PARMS( "all", "%ls", all );
		PARMS( "str", "%ls", str );
	}
	else
	{
		PARMS( "all", "%s", all );
		PARMS( "str", "%s", str );
	}
	PARMS( "anchors", "%d", anchors );
	PARMS( "flags", "%d", flags );

	/* Perform anchor checkings? */
	if( flags & PREGEX_RUN_NOANCHORS )
	{
		MSG( "Anchor checking is disabled through flags, or not required" );
		RETURN( TRUE );
	}

	/* Check parameter integrity */
	if( !( str || len ) )
	{
		MSG( "Not enough or wrong parameters!" );
		RETURN( FALSE );
	}

	if( !all )
		all = str;

	if( flags & PREGEX_RUN_WCHAR )
		charsize = sizeof( wchar_t );

	/* Begin of line anchor */
	if( anchors & PREGEX_FLAG_BOL )
	{
		MSG( "Begin of line anchor is set" );
		if( all < str )
		{
			if( flags & PREGEX_RUN_WCHAR )
			{
				VARS( "str-1", "%ls", (wchar_t*)( str - 1 ) );
				ch = *( (wchar_t*)( str - 1 ) );
			}
			else
			{
				VARS( "str-1", "%s", str-1 );
				ch = *( str - 1 );
			}

			VARS( "ch", "%lc", ch );
			if( ch != '\n' && ch != '\r' )
				RETURN( FALSE );
		}
	}

	/* End of Line anchor */
	if( anchors & PREGEX_FLAG_EOL )
	{
		MSG( "End of line anchor is set" );
		if( ( ch = *( str + ( len * charsize ) ) ) )
		{
			VARS( "ch", "%lc", ch );
			if( ch != '\n' && ch != '\r' )
				RETURN( FALSE );
		}
	}

	/* Begin of word anchor */
	if( anchors & PREGEX_FLAG_BOW )
	{
		MSG( "Begin of word anchor is set" );
		if( all < str )
		{
			if( flags & PREGEX_RUN_WCHAR )
			{
				VARS( "str-1", "%ls", (wchar_t*)( str - 1 ) );
				ch = *( (wchar_t*)( str - 1 ) );
			}
			else
			{
				VARS( "str-1", "%s", str-1 );
				ch = *( str - 1 );
			}

			VARS( "ch", "%lc", ch );
			if( iswalnum( ch ) || ch == '_' )
				RETURN( FALSE );
		}
	}

	/* End of word anchor */
	if( anchors & PREGEX_FLAG_EOW )
	{
		MSG( "End of word anchor is set" );
		if( ( ch = *( str + ( len * charsize ) ) ) )
		{
			VARS( "ch", "%lc", ch );
			if( iswalnum( ch ) || ch == '_' )
				RETURN( FALSE );
		}
	}

	MSG( "All anchor tests where fine!" );
	RETURN( TRUE );
}

/*COD_ON*/

