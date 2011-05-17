/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator 
Copyright (C) 2006-2011 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com/ ++ unicc<<AT>>phorward-software<<DOT>>com

File:	p_util.c
Author:	Jan Max Meyer
Usage:	Utility functions

You may use, modify and distribute this software under the terms and conditions
of the Artistic License, version 2. Please see LICENSE for more information.
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

	Parameters:		uchar*		name		Original name to be derived.
					uchar		append_char	Character to be appended.
					
	Returns:		uchar*					The derived name; New allocated memory,
											must be freed!
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
uchar* p_derivation_name( uchar* name, uchar append_char )
{
	uchar*		ret;
	size_t		len;

	ret = (uchar*)p_malloc( ( strlen( name ) + 1 + 1 ) * sizeof( uchar ) );
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

	Parameters:		uchar*		str			Pointer where the character parse
											starts at.
					uchar**		strfix		Optional return pointer for the
											new position next to parsed character
											definition.
					
	Returns:		int						The character value
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	18.07.2009	Jan Max Meyer	Negative escaped characters
----------------------------------------------------------------------------- */
int p_unescape_char( uchar* str, uchar** strfix )
{
	uchar*	ptr = str;
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
uchar* p_gen_c_identifier( uchar* str, BOOLEAN to_upper )
{
	uchar*	p;

	if( !( str = pstrdup( str ) ) )
	 	OUTOFMEM;

	for( p = str; *p; p++ )
	{
		if( pisalnum( *p ) )
		{
			if( to_upper )
				*p = ptoupper( *p );
		}
		else
			*p = '_';
	}

	return str;
}

