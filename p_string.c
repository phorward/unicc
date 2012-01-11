/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator 
Copyright (C) 2006-2012 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com/ ++ unicc<<AT>>phorward-software<<DOT>>com

File:	p_string.c
Author:	Jan Max Meyer
Usage:	String-related management functions

You may use, modify and distribute this software under the terms and conditions
of the Artistic License, version 2. Please see LICENSE for more information.
----------------------------------------------------------------------------- */

/*
 * Includes
 */
#include "p_global.h"
#include "p_proto.h"

/*
 * Global variables
 */


/*
 * Functions
 */

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_int_to_str()
	
	Author:			Jan Max Meyer
	
	Usage:			Returns an allocated string which contains the string-repre-
					sentation of an int value, for code generation purposes.

	Parameters:		int		val					Value to be converted
					
	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
uchar* p_int_to_str( int val )
{
	uchar*	ret;

	ret = (uchar*)pmalloc( 64 * sizeof( uchar ) );
	sprintf( ret, "%d", val );

	return ret;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_chr_to_str()
	
	Author:			Jan Max Meyer
	
	Usage:			Returns an allocated string which contains the character-
					representation of an int value, for code generation purposes.

	Parameters:		int		val					Value to be converted
					
	Returns:		uchar*						Pointer to allocated string
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
uchar* p_chr_to_str( int val )
{
	uchar*	ret;

	ret = (uchar*)pmalloc( (1+1) * sizeof( uchar ) );
	sprintf( ret, "%c", val );

	return ret;
}


/* -FUNCTION--------------------------------------------------------------------
	Function:		p_long_to_str()
	
	Author:			Jan Max Meyer
	
	Usage:			Returns an allocated string which contains the string-repre-
					sentation of a long value, for code generation purposes.

	Parameters:		long	val					Value to be converted
					
	Returns:		uchar*						Pointer to allocated string
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
uchar* p_long_to_str( long val )
{
	uchar*	ret;

	ret = (uchar*)pmalloc( 128 * sizeof( uchar ) );
	sprintf( ret, "%ld", val );

	return ret;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_str_to_str()
	
	Author:			Jan Max Meyer
	
	Usage:			Returns an allocated string which contains the string-repre-
					sentation of a string or empty string if it is (uchar*)NULL.

	Parameters:		uchar*		val				The string that should be
												validated.
					
	Returns:		uchar*						Pointer to allocated string
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
uchar* p_str_to_str( uchar* val )
{
	uchar*	ret;

	if( val )
		ret = pstrdup( val );
	else
		ret = pstrdup( "" );

	return ret;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_str_to_xml()
	
	Author:			Jan Max Meyer
	
	Usage:			Modifies a string for XML-compliance. Frees the string
					parameter it gets and returns a allocated pointer that
					has to be freed manually later.

	Parameters:		uchar*		str				The string to be made
												XML compliant. Will be freed
												by p_str_to_xml().
					
	Returns:		uchar*						Pointer to allocated string
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
uchar* p_str_to_xml( uchar* str )
{
	uchar*	ret;

	if( !str )
		return pstrdup( "" );

	ret = pstr_render( str, 7,
				"<", "&lt;", FALSE,
				">", "&gt;", FALSE,
				"&", "&amp;", FALSE,
				"\"", "&quot;", FALSE,
				"\n", "&#x0A;", FALSE,
				"\r", "&#x0D;", FALSE,
				"\t", "&#x09;", FALSE
			);

	pfree( str );

	return ret;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_strcmp()
	
	Author:			Jan Max Meyer
	
	Usage:			Compares two strings.

	Parameters:		uchar*		str1			First string to be compared.
					uchar*		str2			Second string to be compared.
					int			insensitive		TRUE: Check case insensitive
												FALSE: Check case sensitive
					
	Returns:		int							Same values as strcmp()
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
int p_strcmp( uchar* str1, uchar* str2, int insensitive )
{
	int		cmp_ret;
	if( insensitive )
	{
		str1 = pstrdup( str1 );
		str2 = pstrdup( str2 );

		p_strupr( str1 );
		p_strupr( str2 );
	}

	cmp_ret = strcmp( str1, str2 );

	if( insensitive )
	{
		pfree( str1 );
		pfree( str2 );
	}

	return cmp_ret;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_strupr()
	
	Author:			Jan Max Meyer
	
	Usage:			Serves a platform-independent strupr-function.

	Parameters:		uchar*	str				Acts both as input and output-string.
					
	Returns:		Pointer to the input string.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
uchar* p_strupr( uchar* str )
{
	uchar*	ptr;

	if( !str )
		return (uchar*)NULL;

	for( ptr = str; *ptr; ptr++ )
		if( *ptr >= 'a' && *ptr <= 'z' )
			*ptr -= 32;

	return str;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_unescape_str()
	
	Author:			Jan Max Meyer
	
	Usage:			Unescapes a string.

	Parameters:		uchar*	str				Acts both as input and output-string.
					
	Returns:		Pointer to the input string.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
uchar* p_unescape_str( uchar* str )
{
	uchar*	ptr		= str;
	uchar*	start	= str;

	while( *str != '\0' )
	{
		if( *str == '\\' )
		{
			switch( *(str+1) )
			{
				case 'n':
					*ptr++ = '\n';
					break;
				case 'r':					
					*ptr++ = '\r';
					break;
				case 't':
					*ptr++ = '\t';
					break;
				case 'b':
					*ptr++ = '\b';
					break;
				case '0':
					*ptr++ = '\0';
					break;

				default:
					break;
			}
			str += 2;
		}
		else
			*ptr++ = *str++;
	}

	*ptr = '\0';

	return start;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_str_no_whitespace()
	
	Author:			Jan Max Meyer
	
	Usage:			Removes all whitespaces from a string (including inline ones!)
					and returns the resulting string.

	Parameters:		uchar*	str				Acts both as input and output-string.
					
	Returns:		Pointer to the input string.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
uchar* p_str_no_whitespace( uchar* str )
{
	uchar*	ptr		= str;
	uchar*	start	= str;

	while( *str != '\0' )
		if( *str == ' ' || *str == '\t' )
			str++;
		else
			*ptr++ = *str++;

	*ptr = '\0';

	return start;
}

