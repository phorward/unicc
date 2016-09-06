/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2016 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com ++ unicc<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	p_util.c
Author:	Jan Max Meyer
Usage:	Utility functions
----------------------------------------------------------------------------- */

/*
 * Includes
 */
#include "p_global.h"
#include "p_proto.h"
#include "p_error.h"

/*
 * Global variables
 */


/*
 * Functions
 */

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_derivation_name()

	Author:			Jan Max Meyer

	Usage:			Creates a name derivation.
					The derivation of a name is just a string, where an
					character is appended to, and which is unique.

	Parameters:		char*		name		Original name to be derived.
					char		append_char	Character to be appended.

	Returns:		char*					The derived name; New allocated memory,
											must be freed!

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
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

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_unescape_char()

	Author:			Jan Max Meyer

	Usage:			Parses a single character, even escaped ones.

	Parameters:		char*		str			Pointer where the character parse
											starts at.
					char**		strfix		Optional return pointer for the
											new position next to parsed character
											definition.

	Returns:		int						The character value

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	18.07.2009	Jan Max Meyer	Negative escaped characters
----------------------------------------------------------------------------- */
int p_unescape_char( char* str, char** strfix )
{
	char*	ptr = str;
	int		ch = 0;
	short	cnt = 0;
	BOOLEAN	neg = FALSE;

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

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_find_base_symbol()

	Author:			Jan Max Meyer

	Usage:			Finds out the base symbol for a possibly derived symbol,
					and returns it.

	Parameters:		<type>		<identifier>		<description>

	Returns:		<type>							<description>

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
SYMBOL* p_find_base_symbol( SYMBOL* sym )
{
	while( sym->derived_from )
		sym = sym->derived_from;

	return sym;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_gen_c_identifier()

	Author:			Jan Max Meyer

	Usage:			Construct a C-identifier from a file-name.

	Parameters:		<type>		<identifier>		<description>

	Returns:		<type>							<description>

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
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

