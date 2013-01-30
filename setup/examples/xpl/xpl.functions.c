#include "xpl.h"

xpl_fn  xpl_buildin_functions[] =
{
    {   "exit",     -1,     1,      XPL_exit    },
    {   "print",    1,      -1,     XPL_print   },
    {   "prompt",   -1,     1,      XPL_prompt  },
    {   "integer",  1,      1,      XPL_integer },
    {   "float",    1,      1,      XPL_float   },
    {   "string",   1,      1,      XPL_string  },
};

int xpl_get_function( char* name )
{
    int     i;
    
    /* Try to find function */  
    for( i = 0; i < sizeof( xpl_buildin_functions ) / sizeof( xpl_fn ); i++ )
        if( strcmp( xpl_buildin_functions[ i ].name, name ) == 0 )
            return i;
            
    return -1;
}

int xpl_check_function_parameters( int function, int parameter_count, int line )
{
    if( xpl_buildin_functions[ function ].min > -1 )
    {
        if( parameter_count < xpl_buildin_functions[ function ].min )
        {
            fprintf( stderr,
                "line %d: Too less parameters in call to %s(), %d parameters "
                "required at minimum",
                    line, xpl_buildin_functions[ function ].name,
                        xpl_buildin_functions[ function ].min );
            return 1;
        }
    }
    else if( xpl_buildin_functions[ function ].max > -1 )
    {
        if( parameter_count > xpl_buildin_functions[ function ].max )
        {
            fprintf( stderr,
                "line %d: Too many parameters in call to %s(), %d parameters "
                "allowed at maximum",
                    line, xpl_buildin_functions[ function ].name,
                        xpl_buildin_functions[ function ].max );
            return 1;
        }
    }
    
    return 0;
}

/* Build-in functions follow */

xpl_value* XPL_print( int argc, xpl_value** argv )
{
    int     i;
    for( i = 0; i < argc; i++ )
        printf( "%s\n", xpl_value_get_string( argv[ i ] ) );
        
    return (xpl_value*)NULL;
}

xpl_value* XPL_prompt( int argc, xpl_value** argv )
{
    char    buf [ 256 + 1 ];
    
    if( argc > 0 )
        printf( "%s: ", xpl_value_get_string( argv[ 0 ] ) );
    
    if( fgets( buf, sizeof( buf ), stdin ) )
    {
        buf[ strlen( buf ) - 1 ] = '\0';
        return xpl_value_create_string( buf, 1 );
    }
    
    return xpl_value_create_string( "", 1 );
}

xpl_value* XPL_exit( int argc, xpl_value** argv )
{
    int     rc      = 0;

    if( argc > 0 )
        rc = xpl_value_get_integer( argv[ 0 ] );
        
    exit( rc );
    return (xpl_value*)NULL;
}

xpl_value* XPL_integer( int argc, xpl_value** argv )
{
    return xpl_value_create_integer( xpl_value_get_integer( *argv ) );
}

xpl_value* XPL_float( int argc, xpl_value** argv )
{
    return xpl_value_create_float( xpl_value_get_float( *argv ) );
}

xpl_value* XPL_string( int argc, xpl_value** argv )
{
    return xpl_value_create_string( xpl_value_get_string( *argv ), 1 );
}
