#include "xpl.h"

/* Function for memory allocation */
char* xpl_malloc( char* oldptr, int size )
{
    char*   retptr;

    if( oldptr )
        retptr = (char*)realloc( oldptr, size );
    else
        retptr = (char*)malloc( size );

    if( !retptr )
    {
        fprintf( stderr, "%s, %d: Fatal error, XPL ran out of memory\n",
                    __FILE__, __LINE__ );
        exit( 1 );
    }

    if( !oldptr )
        memset( retptr, 0, size );

    return retptr;
}

char* xpl_strdup( char* str )
{
    char*   nstr;

    nstr = xpl_malloc( (char*)NULL, ( strlen( str ) + 1 ) * sizeof( char ) );
    strcpy( nstr, str );

    return nstr;
}

char* xpl_free( char* ptr )
{
    if( !ptr )
        return (char*)NULL;
        
    free( ptr );
    return (char*)NULL;
}
