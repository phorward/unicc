/* -MODULE----------------------------------------------------------------------
Phorward C/C++ Library
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
https://phorward.info ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	dbg.c
Author:	Jan Max Meyer
Usage:	Macros and functions for trace output.
----------------------------------------------------------------------------- */

#include "phorward.h"

/*
	Trace is activated in any program if the DEBUG-macro is defined.
	A function which uses trace looks like the following:

	int func( int a, char* b )
	{
		int i;

		PROC( "func" );
		PARMS( "a", "%d", a );
		PARMS( "b", "%s", b );

		MSG( "Performing loop..." );
		for( i = 0; i < a; i++ )
		{
			VARS( "i", "%d", i );
			printf( "%c\n", b[ i ] );
		}

		MSG( "Ok, everything is fine! :)" );
		RETURN( i );
	}
*/

/** Write function entry to trace.

The PROC-macro introduces a new function level, if compiled with trace.

The PROC-macro must be put behind the last local variable declaration and the
first code line, else it won't compile. A PROC-macro must exists within a
function to allow for other trace-macro usages. If PROC() is used within a
function, the macros RETURN() or VOIDRET, according to the function return
value, must be used. If PROC is used without RETURN, the trace output will
output a wrong call level depth.

The parameter //func_name// is a static string for the function name.
*/
/*MACRO:PROC( char* func_name )*/

/** Write function return to trace.

RETURN() can only be used if PROC() is used at the beginning of the function.
For void-functions, use the macro VOIDRET.

//return_value// is return-value of the function.
*/
/*MACRO:RETURN( function_type return_value )*/

/** Write void function return to trace.

VOIDRET can only be used if PROC() is used at the beginning of the function.
For typed functions, use the macro RETURN().
*/
/*MACRO:VOIDRET*/

/** Write parameter content to trace.

The PARMS-macro is used to write parameter names and values to the program
trace. PARMS() should - by definition - only be used right behind PROC().
If the logging of variable values is wanted during a function execution to
trace, the VARS()-macro shall be used.

//param_name// is the name of the parameter
//format// is a printf-styled format placeholder.
//parameter// is the parameter itself.
*/
/*MACRO:PARMS( char* param_name, char* format, param_type parameter )*/

/** Write variable content to trace.

The VARS-macro is used to write variable names and values to the program trace.
For parameters taken to functions, the PARMS()-macro shall be used.

//var_name// is the name of the variable
//format// is a printf-styled format placeholder.
//variable// is the parameter itself.
*/
/*MACRO:VARS( char* var_name, char* format, var_type variable )*/

/** Write a message to trace.

//message// is your message!
*/
/*MACRO:MSG( char* message )*/

/** Write any logging output to trace.

This function is newer than the previous ones, and allows for a printf-like
format string with variable amount of parameters.

//format// is a printf()-like format-string.
//...// parameters in the way they occur in the format-string.
*/
/*MACRO:LOG( char* format, ... )*/

/*NO_DOC*/

static int 		_dbg_level;
static clock_t	_dbg_clock;

/** Print //indent// levels to //f//. */
static void _dbg_indent( void )
{
	int		i;
	char*	traceindent;

	if( ( traceindent = getenv( "TRACEINDENT" ) ) )
	{
		if( strcasecmp( traceindent, "ON" ) != 0 && !atoi( traceindent ) )
			return;
	}

	for( i = 0; i < _dbg_level; i++ )
		fprintf( stderr, "." );
}

/** Check if debug output is wanted. */
pboolean _dbg_trace_enabled( char* file, char* function, char* type )
{
	char*		modules;
	char*		functions;
	char*		types;
	char*		basename;
	int			maxdepth;

	if( !( basename = strrchr( file, PPATHSEP ) ) )
		basename = file;
	else
		basename++;

	/* Find out if this module should be traced */
	if( ( modules = getenv( "TRACEMODULE" ) ) )
	{
		if( strcmp( modules, "*" ) != 0
			&& !strstr( modules, basename ) )
			return FALSE;
	}

	/* Find out if this function should be traced */
	if( ( functions = getenv( "TRACEFUNCTION" ) ) )
	{
		if( strcmp( functions, "*" ) != 0
			&& !strstr( functions, function ) )
			return FALSE;
	}

	/* Find out if this type should be traced */
	if( ( types = getenv( "TRACETYPE" ) ) )
	{
		if( strcmp( types, "*" ) != 0
			&& !strstr( types, type ) )
			return FALSE;
	}

	/* One of both configs must be present! */
	if( !modules && !functions )
		return FALSE;

	if( ( maxdepth = atoi( pstrget( getenv( "TRACEDEPTH" ) ) ) ) > 0
			&& _dbg_level > maxdepth )
		return FALSE;

	return TRUE;
}

/** Output trace message to the error log.

//file// is the filename (__FILE__).
//line// is the line number in file (__LINE__).
//type// is the type of the trace message ("PROC", "RETURN", "VARS" ...).
//function// is the function name that is currently executed
//format// is an optional printf()-like format string.
//...// values according to the format string follow.
*/
void _dbg_trace( char* file, int line, char* type,
					char* function, char* format, ... )
{
	va_list		arg;
	time_t		now;

	/* Check if trace is enabled */
	if( !_dbg_trace_enabled( file, function, type ) )
		return;

	if( type && strcmp( type, "ENTRY" ) == 0 )
		_dbg_level++;

	now = clock();
	if( !_dbg_clock )
		_dbg_clock = now;

	fprintf( stderr, "(%-20s:%5d %lf) ", file, line,
		( (double)( now - _dbg_clock ) / CLOCKS_PER_SEC ) );

	_dbg_clock = now;

	_dbg_indent();
	fprintf( stderr, "%-8s", type ? type : "" );

	if( format && *format )
	{
		va_start( arg, format );
		fprintf( stderr, ": " );
		vfprintf( stderr, format, arg );
		va_end( arg );
	}
	else
		fprintf( stderr, ": %s", function );

	fprintf( stderr, "\n" );

	if( type && strcmp( type, "RETURN" ) == 0 )
		_dbg_level--;

	fflush( stderr );
}

/*COD_ON*/
