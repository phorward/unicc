/* String-related management functions */

#include "unicc.h"

/** Returns an allocated string which contains the string-representation of an
int value, for code generation purposes.

//val// is the value to be converted.

Returns a char*-pointer to allocated string. This must bee freed later on.
*/
char* int_to_str( int val )
{
    char*	ret;

    ret = (char*)pmalloc( 64 * sizeof( char ) );
    sprintf( ret, "%d", val );

    return ret;
}

/** Returns an allocated string which contains the string-representation of a
long value, for code generation purposes.

//val// is the value to be converted.

Returns a char*-pointer to allocated string. This must bee freed later on.
*/
char* long_to_str( long val )
{
    char*	ret;

    ret = (char*)pmalloc( 128 * sizeof( char ) );
    sprintf( ret, "%ld", val );

    return ret;
}

/** Removes all whitespaces from a string (including inline ones!) and returns
the resulting string.

//str// is the acts both as input and output-string.

Returns a pointer to the input string.
*/
char* str_no_whitespace( char* str )
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
