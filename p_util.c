/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2017 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com ++ unicc<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	p_util.c
Author:	Jan Max Meyer
Usage:	Utility functions
----------------------------------------------------------------------------- */

#include "unicc.h"

/** Creates a name derivation. The derivation of a name is just a string, where
an character is appended to, and which is unique.

//name// is the original name to be derived.
//append_char// is the character to be appended.

Returns the derived name; New allocated memory, must be freed!
*/
char* p_derivation_name( char* name, char append_char )
{
	char*		ret;
	size_t		len;

	ret = (char*)pmalloc( ( strlen( name ) + 1 + 1 ) * sizeof( char ) );
	strcpy( ret, name );

	len = strlen( ret );
	ret[ len ] = append_char;
	ret[ len + 1 ] = '\0';

	/* Some name styling - this is currently onle for one  case, the whitespace
	symbol ... other cases should not appear... */
	switch( append_char )
	{
		case P_OPTIONAL_CLOSURE:
			if( ret[ len - 1 ] == P_POSITIVE_CLOSURE )
			{
				ret[ len - 1 ] = P_KLEENE_CLOSURE;
				ret[ len ] = '\0';
			}
			break;

		default:
			break;
	}

	return ret;
}

/** Parses a single character, even escaped ones.

//str// is the Pointer where the character parse starts at.
//strfix// is the optional return pointer for the new position next to parsed
character definition.

Returns the character value */
int p_unescape_char( char* str, char** strfix )
{
	char*	ptr = str;
	int		ch = 0;
	short	cnt = 0;
	BOOLEAN	neg = FALSE;

	/*
	18.07.2009 Jan Max Meyer:
	Negative escaped characters
	*/

	if( *ptr == '\\' )
	{
		ptr++;
		switch( *ptr )
		{
			case 'n':
				ch = '\n';
				ptr++;
				break;
			case 'r':
				ch = '\r';
				ptr++;
				break;
			case 't':
				ch = '\t';
				ptr++;
				break;
			case 'v':
				ch = '\v';
				ptr++;
				break;
			case 'a':
				ch = '\a';
				ptr++;
				break;
			case 'b':
				ch = '\b';
				ptr++;
				break;
			case 'f':
				ch = '\f';
				ptr++;
				break;
			case '\'':
				ch = '\'';
				ptr++;
				break;
			case '\"':
				ch = '\"';
				ptr++;
				break;
			case '\\':
				ch = '\\';
				ptr++;
				break;
			case '\?':
				ch = '\?';
				ptr++;
				break;
			case 'x':
				ptr++;

				while( ( ( *ptr >= '0' && *ptr <= '9' )
					|| ( *ptr >= 'A' && *ptr <= 'F' )
					|| ( *ptr >= 'a' && *ptr <= 'f' ) )
					&& cnt < 2 )
				{
					ch *= 16;

					if( ( *ptr >= 'A' && *ptr <= 'F' )
							|| ( *ptr >= 'a' && *ptr <= 'f' ) )
						ch += ( *ptr & 7 ) + 9;
					else
						ch += ( *ptr - '0' );

					ptr++;
					cnt++;
				}
				/* printf( "ch = %d\n", ch ); */
				break;

			default:
				if( *ptr == '-' )
				{
					neg = TRUE;
					ptr++;
				}

				while( *ptr >= '0' && *ptr <= '9' )
				{
					ch *= 10;
					ch += ( *ptr - '0' );
					ptr++;
				}

				if( neg && ch )
					ch *= -1;

				/* if( *ptr != '\0' )
					ptr++; */
				break;
		}
	}
	else if( *ptr != '\0' )
	{
		ch = *ptr;
		ptr++;
	}

	if( strfix )
		*strfix = ptr;

	return ch;
}

/** Finds out the base symbol for a possibly derived symbol, and returns it. */
SYMBOL* p_find_base_symbol( SYMBOL* sym )
{
	while( sym->derived_from )
		sym = sym->derived_from;

	return sym;
}

/** Construct a C-identifier from a file-name. */
char* p_gen_c_identifier( char* str, BOOLEAN to_upper )
{
	char*	p;

	if( !( str = pstrdup( str ) ) )
	 	OUTOFMEM;

	for( p = str; *p; p++ )
	{
		if( isalnum( *p ) )
		{
			if( to_upper )
				*p = toupper( *p );
		}
		else
			*p = '_';
	}

	return str;
}

