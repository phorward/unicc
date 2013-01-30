#include "xpl.h"

/* Program structure handling */
int xpl_get_variable( xpl_program* prog, char* name )
{
    int     i;
    
    /* A function name with the same identifier may not exist! */
    if( xpl_get_function( name ) > -1 )
        return -1;
    
    /* Try to find variable index */    
    for( i = 0; i < prog->variables_cnt; i++ )
        if( strcmp( prog->variables[ i ], name ) == 0 )
            return i;

    /* Else, eventually allocate memory for new variables */
    if( ( i % XPL_MALLOCSTEP ) == 0 )
    {
        prog->variables = (char**)xpl_malloc(
                            (char*)prog->variables, ( i + XPL_MALLOCSTEP )
                                                        * sizeof( char** ) );
    }

    /* Introduce new variable */
    prog->variables[ prog->variables_cnt ] = xpl_strdup( name );

    return prog->variables_cnt++;;
}

int xpl_get_literal( xpl_program* prog, xpl_value* val )
{
    /* Else, eventually allocate memory for new variables */
    if( ( prog->literals_cnt % XPL_MALLOCSTEP ) == 0 )
    {
        prog->literals = (xpl_value**)xpl_malloc(
                                (char*)prog->literals,
                                    ( prog->literals_cnt + XPL_MALLOCSTEP )
                                        * sizeof( xpl_value* ) );
    }
    
    prog->literals[ prog->literals_cnt ] = val;
    return prog->literals_cnt++;
}

int xpl_emit( xpl_program* prog, xpl_op op, int param )
{
    if( ( prog->program_cnt % XPL_MALLOCSTEP ) == 0 )
    {
        prog->program = (xpl_cmd*)xpl_malloc(
                            (char*)prog->program,
                                ( prog->program_cnt + XPL_MALLOCSTEP )
                                    * sizeof( xpl_cmd ) );
    }
    
    prog->program[ prog->program_cnt ].op = op;
    prog->program[ prog->program_cnt ].param = param;
    return prog->program_cnt++;
}

void xpl_reset( xpl_program* prog )
{
    int     i;

    /* Variables */
    for( i = 0; i < prog->variables_cnt; i++ )
        xpl_free( prog->variables[ i ] );
    
    xpl_free( (char*)prog->variables );
    
    /* Literals */
    for( i = 0; i < prog->literals_cnt; i++ )
        xpl_value_free( prog->literals[ i ] );
    
    xpl_free( (char*)prog->literals );
    
    /* Program */
    xpl_free( (char*)prog->program );

    memset( prog, 0, sizeof( xpl_program ) );
}
