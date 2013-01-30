#include "xpl.h"

/* Value Objects */

xpl_value* xpl_value_create( void )
{   
    return (xpl_value*)xpl_malloc( (char*)NULL, sizeof( xpl_value ) );
}

xpl_value* xpl_value_create_integer( int i )
{
    xpl_value*  val;
    
    val = xpl_value_create();
    xpl_value_set_integer( val, i );

    return val;
}

xpl_value* xpl_value_create_float( float f )
{
    xpl_value*  val;
    
    val = xpl_value_create();
    xpl_value_set_float( val, f );

    return val;
}

xpl_value* xpl_value_create_string( char* s, short duplicate )
{
    xpl_value*  val;
    
    val = xpl_value_create();
    xpl_value_set_string( val, duplicate ? xpl_strdup( s ) : s );

    return val;
}

void xpl_value_free( xpl_value* val )
{
    if( !val )
        return;

    xpl_value_reset( val );
    free( val );
}

void xpl_value_reset( xpl_value* val )
{
    if( val->type == XPL_STRINGVAL && val->value.s )
        free( val->value.s );
       
    val->strval = xpl_free( val->strval );
        
    memset( val, 0, sizeof( xpl_value ) );
    val->type = XPL_NULLVAL;
}

xpl_value* xpl_value_dup( xpl_value* val )
{
    xpl_value*  dup;
    
    dup = xpl_value_create();
    
    if( !val )
        return dup;

    memcpy( dup, val, sizeof( xpl_value ) );
    
    dup->strval = (char*)NULL;

    if( dup->type == XPL_STRINGVAL )
        dup->value.s = xpl_strdup( dup->value.s );
        
    return dup;
}

void xpl_value_set_integer( xpl_value* val, int i )
{
    xpl_value_reset( val );
    val->type = XPL_INTEGERVAL;
    val->value.i = i;
}

void xpl_value_set_float( xpl_value* val, float f )
{
    xpl_value_reset( val );
    val->type = XPL_FLOATVAL;
    val->value.f = f;
}

void xpl_value_set_string( xpl_value* val, char* s )
{
    xpl_value_reset( val );
    val->type = XPL_STRINGVAL;
    val->value.s = s;
}

int xpl_value_get_integer( xpl_value* val )
{
    switch( val->type )
    {
        case XPL_INTEGERVAL:
            return val->value.i;
        case XPL_FLOATVAL:
            return (int)val->value.f;
        case XPL_STRINGVAL:
            return atoi( val->value.s );

        default:
            break;
    }

    return 0;
}

float xpl_value_get_float( xpl_value* val )
{
    switch( val->type )
    {
        case XPL_INTEGERVAL:
            return (float)val->value.i;
        case XPL_FLOATVAL:
            return val->value.f;
        case XPL_STRINGVAL:
            return (float)atof( val->value.s );

        default:
            break;
    }

    return 0.0;
}

char* xpl_value_get_string( xpl_value* val )
{
    char    buf     [ 128 + 1 ];
    char*   p;
    
    val->strval = xpl_free( val->strval );

    switch( val->type )
    {
        case XPL_INTEGERVAL:
            sprintf( buf, "%d", val->value.i );
            val->strval = xpl_strdup( buf );
            return val->strval;
        case XPL_FLOATVAL:
            sprintf( buf, "%f", val->value.f );
            
            /* Remove trailing zeros to make values look nicer */            
            for( p = buf + strlen( buf ) - 1; p > buf; p-- )
            {
                if( *p == '.' )
                {
                    *p = '\0';
                    break;
                }
                else if( *p != '0' )
                    break;

                *p = '\0';
            }
            
            val->strval = xpl_strdup( buf );
            return val->strval;
        case XPL_STRINGVAL:
            return val->value.s;

        default:
            break;
    }

    return "";
}
