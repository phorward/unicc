/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2017 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com ++ unicc<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	p_string.c
Author:	Jan Max Meyer
Usage:	String-related management functions
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

	Returns:		char*						Pointer to allocated string

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
char* p_int_to_str( int val )
{
	char*	ret;

	ret = (char*)pmalloc( 64 * sizeof( char ) );
	sprintf( ret, "%d", val );

	return ret;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_long_to_str()

	Author:			Jan Max Meyer

	Usage:			Returns an allocated string which contains the string-repre-
					sentation of a long value, for code generation purposes.

	Parameters:		long	val					Value to be converted

	Returns:		char*						Pointer to allocated string

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
char* p_long_to_str( long val )
{
	char*	ret;

	ret = (char*)pmalloc( 128 * sizeof( char ) );
	sprintf( ret, "%ld", val );

	return ret;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_str_no_whitespace()

	Author:			Jan Max Meyer

	Usage:			Removes all whitespaces from a string (including inline
					ones!) and returns the resulting string.

	Parameters:		char*	str			Acts both as input and output-string.

	Returns:		Pointer to the input string.

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
char* p_str_no_whitespace( char* str )
{
	char*	ptr		= str;
	char*	start	= str;

	while( *str != '\0' )
		if( *str == ' ' || *str == '\t' )
			str++;
		else
			*ptr++ = *str++;

	*ptr = '\0';

	return start;
}

