/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator 
Copyright (C) 2006-2009 by Phorward Software Technologies, Jan Max Meyer
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
	Function:		p_tpl_insert()
	
	Author:			Jan Max Meyer
	
	Usage:			Inserts values into the according wildcards positions of a
					source-code template.
					
	Parameters:		uchar*		tpl					The code template to be
													filled with values.
					...								Set of values to be inserted
													into the desired position;
													These must be always consist
													of three values:

													- Wildcard-Name (const uchar*)
													- Value to be inserted (uchar*)
													- free-flag (boolean int)
													  if the value is freed by
													  this function (for recur-
													  sive code generation!)

												    Put a (uchar*)NULL as end of
													parameter signal.

	Returns:		uchar*							An allocated string which
													is the resulting source.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
uchar* p_tpl_insert( uchar* tpl, ... )
{
	struct
	{
		uchar*	wildcard;
		uchar*	value;
		BOOLEAN	clear;
		uchar*	_match;
	} values[64];

	va_list	args;
	int		i;
	int		vcount;
	int		match;
	uchar*	tpl_ptr			= tpl;
	uchar*	result			= (uchar*)NULL;
	long	copy_size;
	long	prev_size;
	long	size			= 0L;

	if( !tpl )
		return (uchar*)NULL;

	va_start( args, tpl );

	for( vcount = 0; vcount < 64; vcount++ )
	{
		if( !( values[vcount].wildcard = va_arg( args, uchar* ) ) )
			break;

		values[vcount].value = va_arg( args, uchar* );
		values[vcount].clear = va_arg( args, BOOLEAN );

		if( !values[vcount].value )
		{
			values[vcount].value = p_strdup( "" );
			values[vcount].clear = TRUE;
		}
	}

	do
	{
		for( i = 0; i < vcount; i++ )
			values[i]._match = strstr( tpl_ptr, values[i].wildcard );

		match = vcount;
		for( i = 0; i < vcount; i++ )
		{
			if( values[i]._match != (uchar*)NULL )
			{
				if( match == vcount || values[match]._match > values[i]._match )
					match = i;
			}
		}

		prev_size = size;
		if( match < vcount )
		{
			copy_size = (long)( values[match]._match - tpl_ptr );
			size += (long)strlen( values[match].value );
		}
		else
			copy_size = (long)strlen( tpl_ptr );

		size += copy_size;

		if( result )
			result = (uchar*)p_realloc( (uchar*)result, 
				( size + 1 ) * sizeof( uchar ) );
		else
			result = (uchar*)p_malloc( ( size + 1 ) * sizeof( uchar ) );

		memcpy( result + prev_size, tpl_ptr, copy_size * sizeof( uchar ) );

		if( match < vcount )
			memcpy( result + prev_size + copy_size, values[match].value,
				strlen( values[match].value ) * sizeof( uchar ) );

		result[size] = '\0';

		if( match < vcount )
			tpl_ptr += copy_size + strlen( values[match].wildcard );
	}
	while( match < vcount );


	for( i = 0; i < vcount; i++ )
		if( values[i].clear )
			free( values[i].value );

	va_end( args );

	return result;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_str_append()

	Author:			Jan Max Meyer
	
	Usage:			Appends a dynamic string to another one and reallocates
					memory. The function also frees the second parameter string
					that is appended to the first one automatically.

	Parameters:		uchar*	dest		String destination to be extended
					uchar*	src			Source to be appended to dest.
					BOOLEAN	freesrc		Defines if src will be freed or not.

	Returns:		uchar*				(must be freed via free())
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	18.10.2007	Jan Max Meyer	freesrc parameter added for better performance!
----------------------------------------------------------------------------- */
uchar* p_str_append( uchar* dest, uchar* src, BOOLEAN freesrc )
{
	if( src )
	{
		if( dest == (uchar*)NULL )
		{
			if( freesrc )
			{
				dest = src;
				freesrc = FALSE;
			}
			else
				dest = p_strdup( src );
		}
		else
		{
			dest = (uchar*)p_realloc( (uchar*)dest, ( strlen( dest ) + strlen( src ) + 1 )
					* sizeof( uchar ) );
			strcat( dest, src );
		}
	
		if( freesrc )
			p_free( src );
	}

	return dest;
}

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

	ret = (uchar*)p_malloc( 64 * sizeof( uchar ) );
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

	ret = (uchar*)p_malloc( (1+1) * sizeof( uchar ) );
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

	ret = (uchar*)p_malloc( 128 * sizeof( uchar ) );
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
		ret = p_strdup( val );
	else
		ret = p_strdup( "" );

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
		return p_strdup( "" );

	ret = p_tpl_insert( str, 7,
				"<", "&lt;", FALSE,
				">", "&gt;", FALSE,
				"&", "&amp;", FALSE,
				"\"", "&quot;", FALSE,
				"\n", "&#x0A;", FALSE,
				"\r", "&#x0D;", FALSE,
				"\t", "&#x09;", FALSE
			);

	p_free( str );

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
		str1 = p_strdup( str1 );
		str2 = p_strdup( str2 );

		p_strupr( str1 );
		p_strupr( str2 );
	}

	cmp_ret = strcmp( str1, str2 );

	if( insensitive )
	{
		p_free( str1 );
		p_free( str2 );
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

