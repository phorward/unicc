/* -MODULE----------------------------------------------------------------------
Phorward C/C++ Library
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
https://phorward.info ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	string.c
Author:	Jan Max Meyer
Usage:	Some extended functions for zero-terminated byte- and wide-character
		strings that can be universally used.
----------------------------------------------------------------------------- */

#include "phorward.h"

#define MAX_SIZE		64
#define MALLOC_STEP		32

/** Dynamically appends a character to a string.

//str// is the pointer to a string to be appended. If this is (char*)NULL,
the string will be newly allocated. //chr// is the character to be appended
to str.

Returns a char*-pointer to the (possibly re-)allocated and appended string.
(char*)NULL is returned if no memory could be (re)allocated. This pointer must
be released with pfree() when its existence is no longer required.
*/
char* pstrcatchar( char* str, char chr )
{
	PROC( "pstrcatchar" );
	PARMS( "str", "%p", str );
	PARMS( "chr", "%d", chr );

	if( !chr )
		return str;

	if( !str )
	{
		MSG( "Allocating new string" );
		str = (char*)pmalloc( ( 1 + 1 ) * sizeof( char ) );

		if( str )
			*str = '\0';
	}
	else
	{
		MSG( "Reallocating existing string" );
		str = (char*)prealloc( (char*)str,
				( pstrlen( str ) + 1 + 1 ) * sizeof( char ) );
	}

	VARS( "str", "%p", str );
	if( !str )
	{
		MSG( "Pointer is null, critical error" );
		exit( 1 );
	}

	sprintf( str + pstrlen( str ), "%c", (int)chr );

	RETURN( str );
}

/*TESTCASE:pstrcatchar
#include <phorward.h>

void testcase()
{
	char* str1;

	str1 = pstrdup( "Hello" );
	str1 = pstrcatchar( str1, 'X' );

	printf( "%s", str1 );

	pfree( str1 );
}
---
HelloX
*/


/** Dynamically appends a zero-terminated string to a dynamic string.

//str// is the pointer to a zero-terminated string to be appended.
If this is (char*)NULL, the string is newly allocated.

//append// is the string to be appended at the end of //str//.

//freesrc// frees the pointer provided as //append// automatically by
this function, if set to TRUE.

Returns a char*-pointer to (possibly re-)allocated and appended string.
(char*)NULL is returned if no memory could be (re)allocated, or both strings
were NULL. If //dest// is NULL and //freesrc// is FALSE, the function
automatically returns the pointer //src//. This pointer must be released with
pfree() when its existence is no longer required.
*/
char* pstrcatstr( char* dest, char* src, pboolean freesrc )
{
	PROC( "pstrcatstr" );
	PARMS( "dest", "%p", dest );
	PARMS( "src", "%p", src );
	PARMS( "freesrc", "%s", BOOLEAN_STR( freesrc ) );

	if( src )
	{
		if( !dest )
		{
			if( freesrc )
			{
				dest = src;
				freesrc = FALSE;
			}
			else
				dest = pstrdup( src );
		}
		else
		{
			dest = (char*)prealloc( (char*)dest,
					( pstrlen( dest ) + pstrlen( src ) + 1 )
						* sizeof( char ) );
			strcat( dest, src );
		}

		if( freesrc )
			pfree( src );
	}

	RETURN( dest );
}

/*TESTCASE:pstrcatstr
#include <phorward.h>

void testcase()
{
	char* str1;

	str1 = pstrdup( "Hello" );
	str1 = pstrcatstr( str1, "World", FALSE );

	printf( "%s", str1 );

	pfree( str1 );
}
---
HelloWorld
*/

/** Dynamically appends n-characters from one string to another string.

The function works similar to pstrcatstr(), but allows to copy only a maximum
of //n// characters from //append//.

//str// is the pointer to a string to be appended. If this is (char*)NULL,
the string is newly allocated. //append// is the begin of character sequence to
be appended. //n// is the number of characters to be appended to //str//.

Returns a char*-pointer to (possibly re-)allocated and appended string.
(char*)NULL is returned if no memory could be (re)allocated, or both strings
were NULL. This pointer must be released with pfree() when its existence
is no longer required.
*/
char* pstrncatstr( char* str, char* append, size_t n )
{
	size_t	len		= 0;

	PROC( "pstrncatstr" );
	PARMS( "str", "%p", str );
	PARMS( "append", "%p", append );
	PARMS( "n", "%d", n );

	if( append )
	{
		if( !str )
		{
			if( !( str = (char*)pmalloc( ( n + 1 ) * sizeof( char ) ) ) )
				RETURN( (char*)NULL );
		}
		else
		{
			len = pstrlen( str );

			if( !( str = (char*)prealloc( (char*)str,
					( len + n + 1 ) * sizeof( char ) ) ) )
				RETURN( (char*)NULL );
		}

		strncpy( str + len, append, n );
		str[ len + n ] = '\0';
	}

	RETURN( str );
}

/*TESTCASE:pstrncatstr
#include <phorward.h>

void testcase()
{
	char* str1;

	str1 = pstrdup( "Hello" );
	str1 = pstrncatstr( str1, "WorldDinosaurus", 5 );

	printf( "%s", str1 );

	pfree( str1 );
}
---
HelloWorld
*/

/** Replace a substring sequence within a string.

//str// is the string to be replaced in. //find// is the substring to be
matched. //replace// is the string to be inserted for each match of the
substring //find//.

Returns a char* containing the allocated string which is the result of replacing
all occurences of //find// with //replace// in //str//.

This pointer must be released with pfree() when its existence is no longer
required.
*/
char* pstrreplace( char* str, char* find, char* replace )
{
	char*			match;
	char*			str_ptr			= str;
	char*			result			= (char*)NULL;
	char*			result_end		= (char*)NULL;
	unsigned long	len;
	unsigned long	rlen;
	unsigned long	size			= 0L;

	PROC( "pstrreplace" );
	PARMS( "str", "%s", str );
	PARMS( "find", "%s", find );
	PARMS( "replace", "%s", replace );

	len = pstrlen( find );
	rlen = pstrlen( replace );

	while( 1 )
	{
		VARS( "str_ptr", "%s", str_ptr );
		if( !( match = strstr( str_ptr, find ) ) )
		{
			size = 0;
			match = str_ptr + pstrlen( str_ptr );
		}
		else
			size = rlen;

		size += (unsigned long)( match - str_ptr );

		VARS( "size", "%ld", size );
		VARS( "match", "%s", match );

		if( !result )
			result = result_end = (char*)pmalloc(
				( size + 1 ) * sizeof( char ) );
		else
		{
			result = (char*)prealloc( (char*)result,
				( result_end - result + size + 1 ) * sizeof( char ) );
			result_end = result + pstrlen( result );
		}

		if( !result )
		{
			MSG( "Ran out of memory!" );
			exit( 1 );
		}

		strncpy( result_end, str_ptr, match - str_ptr );
		result_end += match - str_ptr;
		*result_end = '\0';

		VARS( "result", "%s", result );

		if( !*match )
			break;

		strcat( result_end, replace );
		VARS( "result", "%s", result );

		result_end += rlen;
		str_ptr = match + len;
	}

	RETURN( result );
}

/*TESTCASE:pstrreplace
#include <phorward.h>

void testcase()
{
	char* str1;
	char* str2;

	str1 = pstrdup( "Hello World" );
	str2 = pstrreplace( str1, "World", "Universe" );

	printf( "%s", str2 );

	pfree( str1 );
}
---
Hello Universe
*/

/** Duplicate a string in memory.

//str// is the string to be copied in memory. If //str// is provided as NULL,
the function will also return NULL.

Returns a char*-pointer to the newly allocated copy of //str//. This pointer
must be released with pfree() when its existence is no longer required.
*/
char* pstrdup( char* str )
{
	if( !str )
		return (char*)NULL;

	return (char*)pmemdup( str, ( pstrlen( str ) + 1 ) * sizeof( char ) );
}

/** Duplicate //n// characters from a string in memory.

The function mixes the functionalities of strdup() and strncpy().
The resulting string will be zero-terminated.

//str// is the parameter string to be duplicated. If this is provided as
(char*)NULL, the function will also return (char*)NULL.
//n// is the number of characters to be copied and duplicated from //str//.
If //n// is greater than the length of //str//, copying will stop at the zero
terminator.

Returns a char*-pointer to the allocated memory holding the zero-terminated
string duplicate. This pointer must be released with pfree() when its existence
is no longer required.
*/
char* pstrndup( char* str, size_t len )
{
	char*	ret;

	if( !str )
		return (char*)NULL;

	if( pstrlen( str ) < len )
		len = pstrlen( str );

	ret = (char*)pmalloc( ( len + 1 ) * sizeof( char ) );
	strncpy( ret, str, len );
	ret[ len ] = '\0';

	return ret;
}

/** Return length of a string.

//str// is the parameter string to be evaluated. If (char*)NULL, the function
returns 0. pstrlen() is much safer than strlen() because it returns 0 when
a NULL-pointer is provided.

Returns the length of the string //str//.
*/
size_t pstrlen( char* str )
{
	if( !str )
		return 0;

	return strlen( (char*)str );
}

/** Assign a string to a dynamically allocated pointer.

pstrput() manages the assignment of a dynamically allocated string.

//str// is a pointer receiving the target pointer to be (re)allocated. If
//str// already references a string, this pointer will be freed and reassigned
to a copy of //val//.

//val// is the string to be assigned to //str// (as a independent copy).

Returns a pointer to the allocated heap memory on success, (char*)NULL else.
This is the same pointer as returned when calling ``*str``. The returned pointer
must be released with pfree() or another call of pstrput(). Calling pstrput()
as ``pstrput( &p, (char*)NULL );`` is equivalent to ``p = pfree( &p )``.
*/
char* pstrput( char** str, char* val )
{
	if( *str )
	{
		if( val && strcmp( *str, val ) == 0 )
			return *str;

		pfree( *str );
	}

	*str = pstrdup( val );
	return *str;
}

/** Safely reads a string.

//str// is the string pointer to be safely read. If //str// is NULL, the
function returns a pointer to a static address holding an empty string.
*/
char* pstrget( char* str )
{
	if( !str )
		return "";

	return str;
}

/*TESTCASE:pstrput, pstrget
#include <phorward.h>

void testcase()
{
	char* s = NULL;

	printf( "%s\n", pstrget( s ) );
	pstrput( &s, "Hello World" );
	printf( "%s\n", pstrget( s ) );
	pstrput( &s, "Jean Luc Picard" );
	printf( "%s\n", pstrget( s ) );

	pfree( s );
}
---

Hello World
Jean Luc Picard
*/

/** String rendering function.

Inserts multiple values dynamically into the according wildcards positions of
a template string. The function can be compared to the function of
pstrreplace(), but allows to replace multiple substrings by multiple replacement
strings.

//tpl// is the template string to be rendered with values.
//...// are the set of values to be inserted into the desired position;

These consist of three values each:

- //char* name// as a wildcard-name
- //char* value// as the replacement value for the wildcard
- //pboolean freeflag// defines if //value// shall be freed after processing
-

Returns an allocated string which is the result of rendering. This string must
be released by pfree() or another function releasing heap memory when its
existence is no longer required.
*/
char* pstrrender( char* tpl, ... )
{
	struct
	{
		char*	wildcard;
		char*	value;
		BOOLEAN	clear;
		char*	_match;
	} values[MAX_SIZE];

	va_list	args;
	int		i;
	int		vcount;
	int		match;
	char*	tpl_ptr			= tpl;
	char*	result			= (char*)NULL;
	long	copy_size;
	long	prev_size;
	long	size			= 0L;

	if( !tpl )
		return (char*)NULL;

	va_start( args, tpl );

	for( vcount = 0; vcount < MAX_SIZE; vcount++ )
	{
		if( !( values[vcount].wildcard = va_arg( args, char* ) ) )
			break;

		values[vcount].value = va_arg( args, char* );
		values[vcount].clear = (pboolean)va_arg( args, int );

		if( !values[vcount].value )
		{
			values[vcount].value = pstrdup( "" );
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
			if( values[i]._match != (char*)NULL )
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
			result = (char*)prealloc( (char*)result,
				( size + 1 ) * sizeof( char ) );
		else
			result = (char*)pmalloc( ( size + 1 ) * sizeof( char ) );

		memcpy( result + prev_size, tpl_ptr, copy_size * sizeof( char ) );

		if( match < vcount )
			memcpy( result + prev_size + copy_size, values[match].value,
				strlen( values[match].value ) * sizeof( char ) );

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

/*TESTCASE:String rendering from templates
#include <phorward.h>

void testcase()
{
	char* str1;
	char* str2 = "Hello World";

	str1 = pstrrender( "<a href=\"$link\" alt=\"$title\">$title</a>",
			"$link", "https://phorward.info", FALSE,
			"$title", str2, FALSE,
			(char*)NULL );

	printf( "str1 = >%s<\n", str1 );
	pfree( str1 );
}
---
str1 = ><a href="https://phorward.info" alt="Hello World">Hello World</a><
*/


/** Removes whitespace on the left of a string.

//s// is the string to be left-trimmed.

Returns //s//.
*/
char* pstrltrim( char* s )
{
	char*	c;

	if( !( s && *s ) )
		return pstrget( s );

	for( c = s; *c; c++ )
		if( !( *c == ' ' || *c == '\t' || *c == '\r' || *c == '\n' ) )
			break;

	memmove( s, c, ( pstrlen( c ) + 1 ) * sizeof( char ) );

	return s;
}

/** Removes trailing whitespace on the right of a string.

//s// is the string to be right-trimmed.

Returns //s//.
*/
char* pstrrtrim( char* s )
{
	char*	c;

	if( !( s && *s ) )
		return pstrget( s );

	for( c = s + pstrlen( s ) - 1; c > s; c-- )
		if( !( *c == ' ' || *c == '\t' || *c == '\r' || *c == '\n' ) )
			break;

	*( c + 1 ) = '\0';

	return s;
}

/** Removes beginning and trailing whitespace from a string.

//s// is the string to be trimmed.

Returns //s//.
*/
char* pstrtrim( char* s )
{
	if( !( s && *s ) )
		return s;

	return pstrltrim( pstrrtrim( s ) );
}

/** Splits a string at a delimiting token and returns an allocated array of
token reference pointers.

//tokens// is an allocated array of tokenized array values.
Requires a pointer to char**.
//str// is the input string to be tokenized.
//sep// is the token separation substring.
//limit// is the token limit; If set to 0, there is no token limit available,
in which case as many as possible tokens are read.

Returns the number of separated tokens, or -1 on error.
*/
int pstrsplit( char*** tokens, char* str, char* sep, int limit )
{
	char*	next;
	char*	tok		= str;
	int		cnt		= 0;

	PROC( "pstrsplit" );
	PARMS( "tokens", "%p", tokens );
	PARMS( "str", "%s", str );
	PARMS( "sep", "%p", sep );
	PARMS( "limit", "%d", limit );

	if( !( tokens && str && sep && *sep ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	if( !( *tokens = (char**)pmalloc( MALLOC_STEP * sizeof( char* ) ) ) )
		RETURN( -1 );

	VARS( "cnt", "%d", cnt );
	VARS( "tok", "%s", tok );
	(*tokens)[ cnt++ ] = tok;

	while( ( next = strstr( tok, sep ) )
			&& ( ( limit > 0 ) ? cnt < limit : 1 ) )
	{
		tok = next + pstrlen( sep );
		VARS( "next", "%s", next );
		*next = '\0';

		if( ( cnt % MAX_SIZE ) == 0 )
		{
			MSG( "realloc required!" );
			if( !( *tokens = (char**)prealloc( (char**)*tokens,
					( cnt + MALLOC_STEP ) * sizeof( char* ) ) ) )
				RETURN( -1 );
		}

		VARS( "cnt", "%d", cnt );
		VARS( "tok", "%s", tok );
		(*tokens)[ cnt++ ] = tok;
	}

	if( limit > 0 )
		*next = '\0';

	RETURN( cnt );
}

/*TESTCASE:String Tokenizer
#include <phorward.h>

void testcase()
{
	char 	str[1024];
	int 	i, all;
	char**	tokens;

	strcpy( str, "Hello World, this is a simple test" );
	all = pstrsplit( &tokens, str, " ", 3 );
	printf( "%d\n", all );

	for( i = 0; i < all; i++ )
		printf( "%d: >%s<\n", i, tokens[i] );

	pfree( tokens );
}
---
3
0: >Hello<
1: >World,<
2: >this<
*/

/** Convert a string to upper-case.

//s// acts both as input- and output string.

Returns //s//.
*/
char* pstrupr( char* s )
{
	char*	ptr;

	if( !s )
		return (char*)NULL;

	for( ptr = s; *ptr; ptr++ )
		if( islower( *ptr ) )
			*ptr = toupper( *ptr );

	return s;
}

/** Convert a string to lower-case.

//s// is the acts both as input and output-string.

Returns //s//.
*/
char* pstrlwr( char* s )
{
	char*	ptr;

	if( !s )
		return (char*)NULL;

	for( ptr = s; *ptr; ptr++ )
		if( isupper( *ptr ) )
			*ptr = tolower( *ptr );

	return s;
}

/** Compare a string ignoring case-order.

//s1// is the string to compare with //s2//.
//s2// is the string to compare with //s1//.

Returns 0 if both strings are equal. Returns a value <0 if //s1// is lower than
//s2// or a value >0 if //s1// is greater than //s2//.
*/
int	pstrcasecmp( char* s1, char* s2 )
{
	if( !( s1 && s2 ) )
		return -1;

	for( ; *s1 && *s2 && toupper( *s1 ) == toupper( *s2 ); s1++, s2++ )
		;

	return (int)( toupper( *s1 ) - toupper( *s2 ) );
}

/** Compare two strings ignoring case-order up to a maximum of //n// bytes.

//s1// is the string to compare with //s2//.
//s2// is the string to compare with //s1//.
//n// is the number of bytes to compare.

Returns 0 if both strings are equal. Returns a value <0 if //s1// is less than
//s2// or a value >0 if //s1// is greater than //s2//.
*/
int	pstrncasecmp( char* s1, char* s2, size_t n )
{
	if( !( s1 && s2 && n ) )
	{
		WRONGPARAM;
		return -1;
	}

	for( ; n > 0 && *s1 && *s2 && toupper( *s1 ) == toupper( *s2 );
			s1++, s2++, n-- )
		;

	return (int)( ( !n ) ? 0 : ( toupper( *s1 ) - toupper( *s2 ) ) );
}

/** Converts a string with included escape-sequences back into its natural form.

The following table shows escape sequences which are converted.

|| Sequence | is replaced by |
| \n | newline |
| \t | tabulator |
| \r | carriage-return |
| \b | backspace |
| \f | form feed |
| \a | bell / alert |
| \' | single-quote |
| \" | double-quote |


The replacement is done within the memory bounds of //str// itself, because the
unescaped version of the character requires less space than its previous escape
sequence.

The function always returns its input pointer.

**Example:**
```
char* s = (char*)NULL;

psetstr( &s, "\\tHello\\nWorld!" );
printf( ">%s<\n", pstrunescape( s ) );

s = pfree( s );
```
*/
char* pstrunescape( char* str )
{
	char*	ch;
	char*	esc;
	char*	ptr;

	for( ptr = ch = str; *ch; ch++, ptr++ )
	{
		if( *ch == '\\' && *( ch + 1 ) )
		{
			for( esc = "n\nt\tr\rb\bf\fv\va\a'\'\"\""; *esc; esc += 2 )
				if( *( ch + 1 ) == *esc )
				{
					*ptr = *( esc + 1 );
					ch++;
					break;
				}

			/* TODO: hex-seqs (UTF-8). */
		}
		else
			*ptr = *ch;
	}

	*ptr = '\0';
	return str;
}

/** Implementation and replacement for vasprintf.

//str// is the pointer receiving the result, allocated string pointer.
//fmt// is the format string.
//...// are the parameters according to the placeholders set in //fmt//.

Returns the number of characters written, or -1 in case of an error.
*/
int pvasprintf( char** str, char* fmt, va_list ap )
{
	char*		istr;
	int			ilen;
	int			len;
	va_list		w_ap;

	PROC( "pvasprintf" );
	PARMS( "str", "%p", str );
	PARMS( "fmt", "%s", fmt );
	PARMS( "ap", "%p", ap );

	if( !( istr = (char*)pmalloc( MALLOC_STEP * sizeof( char ) ) ) )
		RETURN( -1 );

	va_copy( w_ap, ap );

	MSG( "Invoking vsnprintf() for the first time" );
	len = vsnprintf( istr, MALLOC_STEP, fmt, w_ap );
	VARS( "len", "%d", len );

	if( len >= 0 && len < MALLOC_STEP )
		*str = istr;
	else if( len == INT_MAX || len < 0 )
	{
		MSG( "ret is negative or too big - can't handle this!" );
		va_end( w_ap );
		RETURN( -1 );
	}
	else
	{
		if( !( istr = prealloc( istr, ( ilen = len + 1 ) * sizeof( char ) ) ) )
		{
			va_end( w_ap );
			RETURN( -1 );
		}

		va_end( w_ap );
		va_copy( w_ap, ap );

		MSG( "Invoking vsnprintf() for the second time" );
		len = vsnprintf( istr, ilen, fmt, w_ap );
		VARS( "len", "%d", len );

		if( len >= 0 && len < ilen )
			*str = istr;
		else
		{
			pfree( istr );
			RETURN( -1 );
		}
	}

	va_end( w_ap );
	RETURN( len );
}

/** Implementation and replacement for asprintf. pasprintf() takes only the
format-string and various arguments. It outputs an allocated string to be freed
later on.

//fmt// is the format string.
//...// are the parameters according to the placeholders set in //fmt//.

Returns a char* Returns the allocated string which contains the format string
with inserted values.
*/
char* pasprintf( char* fmt, ... )
{
	char*	str		= (char*)NULL;
	va_list	args;

	PROC( "pasprintf" );
	PARMS( "fmt", "%s", fmt );

	if( !( fmt ) )
		RETURN( (char*)NULL );

	va_start( args, fmt );
	pvasprintf( &str, fmt, args );
	va_end( args );

	VARS( "str", "%s", str );
	RETURN( str );
}

/*TESTCASE:Self-allocating sprintf extension
#include <phorward.h>

void testcase()
{
	char* str = "Hello World";
	char* str1;

	str1 = pasprintf( "current content of str is >%s<, and len is %d",
						str, strlen( str ) );
	printf( "str1 = >%s<\n", str1 );
	pfree( str1 );
}
---
str1 = >current content of str is >Hello World<, and len is 11<
*/


/******************************************************************************
 * FUNCTIONS FOR UNICODE PROCESSING (wchar_t)                                 *
 ******************************************************************************/

#ifdef UNICODE

/** Duplicate a wide-character string in memory.

//str// is the string to be copied in memory. If //str// is provided as NULL,
the function will also return NULL.

Returns a wchar_t*-pointer to the newly allocated copy of //str//. This pointer
must be released with pfree() when its existence is no longer required.

//This function is only available when compiled with -DUNICODE.//
*/
wchar_t* pwcsdup( wchar_t* str )
{
	if( !str )
		return (wchar_t*)NULL;

	return (wchar_t*)pmemdup( str, ( pwcslen( str ) + 1 ) * sizeof( wchar_t ) );
}

/** Appends a character to a dynamic wide-character string.

//str// is the pointer to a wchar_t-string to be appended. If this is
(wchar_t*)NULL, the string is newly allocated. //chr// is the the character
to be appended to str.

Returns a wchar_t* Pointer to (possibly re-)allo- cated and appended string.
(wchar_t*)NULL is returned if no memory could be (re)allocated.

//This function is only available when compiled with -DUNICODE.//
*/
wchar_t* pwcscatchar( wchar_t* str, wchar_t chr )
{
	PROC( "pwcscatchar" );
	PARMS( "str", "%p", str );
	PARMS( "chr", "%d", chr );

	if( !str )
	{
		MSG( "Allocating new string" );
		str = (wchar_t*)pmalloc( ( 1 + 1 ) * sizeof( wchar_t ) );

		if( str )
			*str = L'\0';
	}
	else
	{
		MSG( "Reallocating existing string" );
		str = (wchar_t*)prealloc( (wchar_t*)str,
				( pwcslen( str ) + 1 + 1) * sizeof( wchar_t ) );
	}

	VARS( "str", "%p", str );
	if( !str )
	{
		MSG( "Pointer is null, critical error" );
		exit( 1 );
	}

	#if _WIN32
	_snwprintf( str + pwcslen( str ), 1, L"%lc", chr );
	#else
	swprintf( str + pwcslen( str ), 1, L"%lc", chr );
	#endif

	RETURN( str );
}

/** Appends a (possibly dynamic) wide-character string to a dynamic
wide-character string.

//str// is the pointer to a wchar_t-string to be appended. If this is
(wchar_t*)NULL, the string is newly allocated.
//append// is the string to be appended.
//freesrc// if true, //append// is free'd automatically by this function.

Returns a wchar_t* Pointer to (possibly re-)allo- cated and appended string.
(wchar_t*)NULL is returned if no memory could be (re)allocated, or both strings
were NULL.

//This function is only available when compiled with -DUNICODE.//
*/
wchar_t* pwcscatstr( wchar_t* dest, wchar_t* src, pboolean freesrc )
{
	PROC( "pwcscatstr" );
	PARMS( "dest", "%p", dest );
	PARMS( "src", "%p", src );
	PARMS( "freesrc", "%d", freesrc );

	if( src )
	{
		if( !dest )
		{
			if( freesrc )
			{
				dest = src;
				freesrc = FALSE;
			}
			else
				dest = pwcsdup( src );
		}
		else
		{
			dest = (wchar_t*)prealloc( (wchar_t*)dest,
					( pwcslen( dest ) + pwcslen( src ) + 1 )
						* sizeof( wchar_t ) );
			wcscat( dest, src );
		}

		if( freesrc )
			pfree( src );
	}

	RETURN( dest );
}

/** Appends //n// characters from one wide-character string to a dynamic string.

//str// is the pointer to a wchar_t-string to be appended. If this is
(wchar_t*)NULL, the string is newly allocated.
//append// is the begin of character sequence to be appended.
//n// is the number of characters to be appended to str.

Returns a wchar_t* Pointer to (possibly re-)allo- cated and appended string.
(wchar_t*)NULL is returned if no memory could be (re)allocated, or both strings
were NULL.

//This function is only available when compiled with -DUNICODE.//
*/
wchar_t* pwcsncatstr( wchar_t* str, wchar_t* append, size_t n )
{
	size_t	len		= 0;

	PROC( "pwcsncatstr" );
	PARMS( "str", "%p", str );
	PARMS( "str", "%ls", str );
	PARMS( "append", "%p", append );
	PARMS( "append", "%ls", append );
	PARMS( "n", "%d", n );

	if( append )
	{
		if( !str )
		{
			if( !( str = (wchar_t*)pmalloc( ( n + 1 ) * sizeof( wchar_t ) ) ) )
				RETURN( (wchar_t*)NULL );
		}
		else
		{
			len = pwcslen( str );

			VARS( "len", "%d", len );
			VARS( "( len + n + 1 ) * sizeof( wchar_t )",
						"%d", ( len + n + 1 ) * sizeof( wchar_t ) );

			if( !( str = (wchar_t*)prealloc( (wchar_t*)str,
					( len + n + 1 ) * sizeof( wchar_t ) ) ) )
				RETURN( (wchar_t*)NULL );
		}

		wcsncpy( str + len, append, n );
		str[ len + n ] = 0;
	}

	RETURN( str );
}

/** Safe strlen replacement for wide-character.

//str// is the parameter string to be evaluated. If (wchar_t*)NULL,
the function returns 0.

//This function is only available when compiled with -DUNICODE.//
*/
size_t pwcslen( wchar_t* str )
{
	if( !str )
		return 0;

	return wcslen( str );
}

/** Assign a wide-character string to a dynamically allocated pointer.

pwcsput() manages the assignment of an dynamically allocated  wide-chararacter
string.

//str// is a pointer receiving the target pointer to be (re)allocated. If
//str// already references a wide-character string, this pointer will be freed
and reassigned to a copy of //val//.

//val// is the the wide-character string to be assigned to //str//
(as an independent copy).

Returns a pointer to the allocated heap memory on success, (char_t*)NULL else.
This is the same pointer as returned when calling ``*str``. The returned pointer
must be released with pfree() or another call of pwcsput(). Calling pwcsput()
as ``pwcsput( &p, (char*)NULL );`` is equivalent to ``p = pfree( &p )``.

//This function is only available when compiled with -DUNICODE.//
*/
wchar_t* pwcsput( wchar_t** str, wchar_t* val )
{
	if( *str )
	{
		if( val && wcscmp( *str, val ) == 0 )
			return *str;

		pfree( *str );
	}

	*str = pwcsdup( val );
	return *str;
}

/** Safely reads a wide-character string.

//str// is the string pointer to be safely read. If //str// is NULL, the
function returns a pointer to a static address holding an empty string.

//This function is only available when compiled with -DUNICODE.//
*/
wchar_t* pwcsget( wchar_t* str )
{
	if( !str )
		return L"";

	return str;
}

/** Duplicate //n// characters from a wide-character string in memory.

The function mixes the functionalities of wcsdup() and wcsncpy().
The resulting wide-character string will be zero-terminated.

//str// is the parameter wide-character string to be duplicated.
If this is provided as (wchar_t*)NULL, the function will also return
(wchar_t*)NULL.

//n// is the the number of characters to be copied and duplicated from //str//.
If //n// is greater than the length of //str//, copying will stop at the zero
terminator.

Returns a wchar_t*-pointer to the allocated memory holding the zero-terminated
wide-character string duplicate. This pointer must be released with pfree()
when its existence is no longer required.

//This function is only available when compiled with -DUNICODE.//
*/
wchar_t* pwcsndup( wchar_t* str, size_t len )
{
	wchar_t*	ret;

	if( !str )
		return (wchar_t*)NULL;

	if( pwcslen( str ) < len )
		len = pwcslen( str );

	ret = (wchar_t*)pmalloc( ( len + 1 ) * sizeof( wchar_t ) );
	wcsncpy( ret, str, len );
	ret[ len ] = '\0';

	return ret;
}


/** Wide-character implementation of pasprintf().

//str// is the a pointer receiving the resultung, allocated string pointer.
//fmt// is the the format string.
//...// is the parameters according to the placeholders set in //fmt//.

Returns the number of characters written.

//This function is only available when compiled with -DUNICODE.//
*/
int pvawcsprintf( wchar_t** str, wchar_t* fmt, va_list ap )
{
	wchar_t*	istr;
	int			ilen;
	int			len;
	va_list		w_ap;

	PROC( "pvawcsprintf" );
	PARMS( "str", "%p", str );
	PARMS( "fmt", "%ls", fmt );
	PARMS( "ap", "%p", ap );

	if( !( istr = (wchar_t*)pmalloc( ( ilen = MALLOC_STEP + 1 )
				* sizeof( wchar_t ) ) ) )
		RETURN( -1 );

	do
	{
		va_copy( w_ap, ap );

		len =
#ifdef __MINGW32__
		_vsnwprintf
#else
		vswprintf
#endif
		( istr, (size_t)ilen, fmt, w_ap );

		va_end( w_ap );
		VARS( "len", "%d", len );

		if( len < 0 )
		{
			if( !( istr = prealloc( istr, ( ilen = ilen + MALLOC_STEP + 1 )
					* sizeof( wchar_t ) ) ) )
				RETURN( -1 );
		}
	}
	while( len < 0 || len >= ilen );

	*str = istr;

	RETURN( len );
}

/** An implementation of pasprintf() for wide-character wchar_t. pasprintf()
takes only the format-string and various arguments. It outputs an allocated
string to be released with pfree() later on.

//fmt// is the format string.
//...// are the parameters according to the placeholders set in //fmt//.

Returns a wchar_t* Returns the allocated string which cointains the format
string with inserted values.

//This function is only available when compiled with -DUNICODE.//
*/
wchar_t* pawcsprintf( wchar_t* fmt, ... )
{
	wchar_t*	str;
	va_list	args;

	PROC( "pasprintf" );
	PARMS( "fmt", "%ls", fmt );

	if( !( fmt ) )
		RETURN( (wchar_t*)NULL );

	va_start( args, fmt );
	pvawcsprintf( &str, fmt, args );
	va_end( args );

	VARS( "str", "%ls", str );
	RETURN( str );
}

#endif /* UNICODE */

/*TESTCASE:UNICODE functions
#include <phorward.h>
#include <locale.h>

void unicode_demo()
{
	wchar_t		mystr		[ 255 ];
	wchar_t*	mydynamicstr;

	setlocale( LC_ALL, "" );

	wcscpy( mystr, L"Yes, w€ cän üse standard C function "
			L"names for Unicode-strings!" );

	printf( "mystr = >%ls<\n", mystr );
	swprintf( mystr, sizeof( mystr ),
			L"This string was %d characters long!",
			pwcslen( mystr ) );
	printf( "mystr = >%ls<\n", mystr );

	mydynamicstr = pwcsdup( mystr );
	mydynamicstr = pwcscatstr( mydynamicstr,
			L" You can see: The functions are"
			L" used the same way as the standard"
			L" char-functions!", FALSE );

	printf( "mydynamicstr = >%ls<\n", mydynamicstr );
	pfree( mydynamicstr );

	mydynamicstr = pawcsprintf( L"This is €uro symbol %ls of %d",
						mystr, sizeof( mystr ) );
	printf( "mydynamicstr = >%ls<\n", mydynamicstr );
	pfree( mydynamicstr );
}
---
mystr = >Yes, w€ cän üse standard C function names for Unicode-strings!<
mystr = >This string was 62 characters long!<
mydynamicstr = >This string was 62 characters long! You can see: The functions are used the same way as the standard char-functions!<
mydynamicstr = >This is €uro symbol This string was 62 characters long! of 1020<
*/
