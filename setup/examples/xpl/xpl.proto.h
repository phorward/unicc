#ifndef XPL_PROTO_H
#define XPL_PROTO_H

/* xpl.debug.c */
void xpl_dump( xpl_program* prog, xpl_runtime* rt );

/* xpl.functions.c */
int xpl_get_function( char* name );
xpl_value* XPL_print( int argc, xpl_value** argv );
xpl_value* XPL_prompt( int argc, xpl_value** argv );
xpl_value* XPL_exit( int argc, xpl_value** argv );
xpl_value* XPL_integer( int argc, xpl_value** argv );
xpl_value* XPL_float( int argc, xpl_value** argv );
xpl_value* XPL_string( int argc, xpl_value** argv );

/* xpl.parser.c */
int xpl_compile( xpl_program* prog, FILE* input );

/* xpl.program.c */
int xpl_get_variable( xpl_program* prog, char* name );
int xpl_get_literal( xpl_program* prog, xpl_value* val );
int xpl_emit( xpl_program* prog, xpl_op op, int param );
void xpl_reset( xpl_program* prog );

/* xpl.run.c */
void xpl_run( xpl_program* prog );

/* xpl.util.c */
char* xpl_malloc( char* oldptr, int size );
char* xpl_strdup( char* str );
char* xpl_free( char* ptr );

/* xpl.value.c */
xpl_value* xpl_value_create( void );
xpl_value* xpl_value_create_integer( int i );
xpl_value* xpl_value_create_float( float f );
xpl_value* xpl_value_create_string( char* s, short duplicate );
void xpl_value_free( xpl_value* val );
void xpl_value_reset( xpl_value* val );
xpl_value* xpl_value_dup( xpl_value* val );
void xpl_value_set_integer( xpl_value* val, int i );
void xpl_value_set_float( xpl_value* val, float f );
void xpl_value_set_string( xpl_value* val, char* s );
int xpl_value_get_integer( xpl_value* val );
float xpl_value_get_float( xpl_value* val );
char* xpl_value_get_string( xpl_value* val );

#endif
