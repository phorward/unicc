/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2013 by Phorward Software Technologies, Jan Max Meyer
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

	Returns:		uchar*						Pointer to allocated string

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
	Function:		p_str_no_whitespace()

	Author:			Jan Max Meyer

	Usage:			Removes all whitespaces from a string (including inline
					ones!) and returns the resulting string.

	Parameters:		uchar*	str			Acts both as input and output-string.

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

