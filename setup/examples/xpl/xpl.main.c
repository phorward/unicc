#include "xpl.h"

int main( int argc, char** argv )
{
    xpl_program     program;
    FILE*           src;
    
    /* Initialize program structure to all zero */
    memset( &program, 0, sizeof( xpl_program ) );
    
    /* Open input file, if provided. */
    if( argc > 1 )
    {
        if( !( src = fopen( argv[1], "rb" ) ) )
        {
            fprintf( stderr, "Can't open file '%s'\n", argv[1] );
            return 1;
        }
    }
    else
    {
        printf( "Usage: %s FILENAME\n", *argv );
        return 1;
    }
    
    /* Call the compiler */
    if( xpl_compile( &program, src ) > 0 )
        return 1;
        
    /* Dump program before execution */     
    xpl_dump( &program, (xpl_runtime*)NULL );
    
    /* Run program */
    xpl_run( &program );
    
    /* Clean-up used memory */
    xpl_reset( &program );

    if( src != stdin )
        fclose( src );

    return 0;
}
