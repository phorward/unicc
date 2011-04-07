/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator 
Copyright (C) 2006-2009 by Phorward Software Technologies, Jan Max Meyer
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
	Function:		p_mapfile()
	
	Author:			Jan Max Meyer
	
	Usage:			Maps a file into the memory.

	Parameters:		uchar*	filename		Name of the file to be mapped to the
											memory.
					
	Returns:		uchar*					The content of the mapped source file.
											Must be freed manually.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
uchar* p_mapfile( uchar* filename )
{
	FILE*	file	= (FILE*)NULL;
	long	size	= 0;
	long	pos		= 0;
	uchar*	src		= (uchar*)NULL;
	uchar	ch;

	file = fopen( filename, "rb" );
	if( file == (FILE*)NULL )
		return (uchar*)NULL;
	
	fseek( file, 0L, SEEK_END );
	size = ftell( file );
	if( size == 0 )
		return (uchar*)NULL;

	src = (uchar*)p_malloc( (size+1+1) * sizeof( uchar ) );
	if( !src )
	{
		OUT_OF_MEMORY;
		return (uchar*)NULL;
	}
	
	fseek( file, 0L, SEEK_SET );
	while( TRUE )
	{
		ch = (uchar)fgetc( file );
		if( !feof( file ) )
		{
			src[ pos ] = ch;
			pos++;
		}
		else
		{
			break;
		}
	}
	src[ pos ] = '\0';

	return src;
}


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
----------------------------------------------------------------------------- */
int p_unescape_char( uchar* str, uchar** strfix )
{
	uchar*	ptr = str;
	int		ch = 0;
	short	cnt = 0;

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
				printf( "ch = %d\n", ch );
				break;

			default:
				while( *ptr >= '0' && *ptr <= '9' )
				{
					ch *= 10;
					ch += ( *ptr - '0' );
					ptr++;
				}

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
	Function:		p_ccl_to_map()
	
	Author:			Jan Max Meyer
	
	Usage:			Parses a character class definition and maps it in a bitset
					with parser->p_universe size, where enabled characters are
					toggled TRUE.

	Parameters:		PARSER*		parser		Parser information structure
					uchar*		ccl			Character class to be parsed/converted
					
	Returns:		bitset					Bitmap representing the character
											states. This is automatically allocated,
											must be freed by caller.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
bitset p_ccl_to_map( PARSER* parser, uchar* ccl )
{
	uchar*		ptr;
	bitset		map;
	BOOLEAN		neg		= FALSE;
	int			start;
	int			stop;
	int			i;

	if( !parser || !ccl )
		return (bitset)NULL;

	map = bitset_create( parser->p_universe );
	if( !map )
		return (bitset)NULL;

	ptr = ccl;

	/*
	if( *ptr == '^' )
	{
		neg = TRUE;
		for( i = 0; i < parser->p_universe; i++ )
			bitset_set( map, i, 1 );
	}
	*/

	for( ; *ptr; )
	{
		start = p_unescape_char( ptr, &ptr );
		if( *(ptr) == '-' )
		{
			ptr++;
			stop = p_unescape_char( ptr, &ptr );
			
			for( i = ( start > stop ? stop : start );
					i <= ( start > stop ? start : stop );
						i++ )
				bitset_set( map, i, neg ? 0 : 1 );
		}
		else
			bitset_set( map, start, neg ? 0 : 1 );
	}

	return map;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_map_to_ccl()
	
	Author:			Jan Max Meyer
	
	Usage:			Parsers a character class map and maps it into a character
					class definition string.

	Parameters:		PARSER*		parser		Parser information structure
					bitset		map			Character class map to be converted
					
	Returns:		uchar*					The character-class definition string.
											This is automatically allocated, must
											be freed by caller.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
uchar* p_map_to_ccl( PARSER* parser, bitset map )
{
#define DEF_FOR_RANGE( fromchar, tochar ) \
	{ \
		if( (fromchar) >= 32 && (fromchar) <= 126 && (fromchar) != '\\' ) \
			sprintf( tmp, "%c", (uchar)(fromchar) ); \
		else \
			sprintf( tmp, "\\%d", (fromchar) ); \
		\
		sprintf( tmp + strlen( tmp ), "-" ); \
		\
		if( (tochar) >= 32 && (tochar) <= 126 && (tochar) != '\\' ) \
			sprintf( tmp + strlen( tmp ), "%c", (uchar)(tochar) ); \
		else \
			sprintf( tmp + strlen( tmp ), "\\%d", (tochar) ); \
	}

#define DEF_FOR_CHAR( chr ) \
	{ \
		if( (chr) >= 32 && (chr) <= 126 && (chr) != '\\' ) \
			sprintf( tmp, "%c", (uchar)(chr) ); \
		else \
			sprintf( tmp, "\\%d", (chr) ); \
	}


	uchar*		ret		= (uchar*)NULL;
	uchar		tmp		[ 30 + 1 ];
	int			i, begin = -1;

	for( i = 0; i < parser->p_universe; i++ )
	{
		if( begin == -1 && bitset_get( map, i ) )
			begin = i;
		else if( !( bitset_get( map, i ) ) && begin > -1 )
		{
			/* Is this a range? */
			if( begin == i - 1 )
				DEF_FOR_CHAR( begin )
			else				
				DEF_FOR_RANGE( begin, i - 1 )

			ret = p_str_append( ret, tmp, FALSE );

			if( !ret )
				return (uchar*)NULL;

			begin = -1;
		}
	}

	if( begin > -1 )
	{
		if( begin == i - 1 )
			DEF_FOR_CHAR( begin )
		else				
			DEF_FOR_RANGE( begin, i - 1 )

		ret = p_str_append( ret, tmp, FALSE );
	}

	return ret;

#undef DEF_FOR_RANGE
#undef DEF_FOR_CHAR
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_negate_ccl()
	
	Author:			Jan Max Meyer
	
	Usage:			Negates a character class definition and constructs a new
					definition string.

	Parameters:		PARSER*		parser		Parser information structure
					uchar*		ccl			Character class to be negated.
					
	Returns:		uchar*					The character-class definition string.
											This is automatically allocated, must
											be freed by caller.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
uchar* p_negate_ccl( PARSER* parser, uchar* ccl )
{
	bitset	tmp;
	int		i;

	if( !( tmp = p_ccl_to_map( parser, ccl ) ) )
		OUT_OF_MEMORY;

	for( i = 0; i < parser->p_universe; i++ )
	{
		if( bitset_get( tmp, i ) )
			bitset_set( tmp, i, 0 );
		else
			bitset_set( tmp, i, 1 );
	}

	ccl = p_map_to_ccl( parser, tmp );
	p_free( tmp );

	return ccl;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_map_test_char()
	
	Author:			Jan Max Meyer
	
	Usage:			Performs a bitset_get(), but with case-insensitivity, if
					desired.

	Parameters:		bitset		map			Pointer to map to test on
					uchar		chr			Character to be tested
					BOOLEAN		insensitive	TRUE: Test case-insensitive
											FALSE: Test case-sensitive
					
	Returns:		BOOLEAN		TRUE		Bit is set
								FALSE		Bit is unset
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
BOOLEAN p_map_test_char( bitset map, uchar chr, BOOLEAN insensitive )
{
	if( bitset_get( map, (int)chr ) )
		return TRUE;
		
	if( insensitive )
	{
		if( chr >= 'A' && chr <= 'Z' )
			chr += 32;
		else if( chr >= 'a' && chr <= 'z' )
			chr -= 32;
			
		if( bitset_get( map, (int)chr ) )
			return TRUE;
	}
	
	return FALSE;
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
