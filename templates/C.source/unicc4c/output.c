/* -MODULE----------------------------------------------------------------------
UniCC4C
Copyright (C) 2010 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<AT>phorward<DASH>software<DOT>com

File:	output.c
Author:	Jan Max Meyer
Usage:	UniCC4C output functions
----------------------------------------------------------------------------- */

/*
 * Includes
 */
#include "unicc4c.h"

/*
 * Global variables
 */

/*
 * Defines
 */

/*
 * Functions
 */

/* Empty Line */
void EL( char** output )
{
	if( !( *output = pstr_append_str( *output, "\n", FALSE ) ) )
		OUTOFMEM;
}

/* Generate tab stops */
void TABS( char** output, int i )
{
	while( i-- > 0 )
	{
		if( !( *output = pstr_append_str( *output, "\t", FALSE ) ) )
			OUTOFMEM;
	}
}

/* Universal PRINT-function used by subsequent functions, internally operated */
void PRINT( uchar** output, uchar* fmt, va_list args )
{
	uchar*	txt;

	if( pvasprintf( &txt, fmt, args ) < 0 )
		OUTOFMEM;

	if( !( *output = pstr_append_str( *output, txt, TRUE ) ) )
		OUTOFMEM;
}

/* Formattet Line */
void L( uchar** output, uchar* txt, ... )
{
	va_list	args;
	
	va_start( args, txt );

	PRINT( output, txt, args );
	EL( output );
	
	va_end( args );
}

/* Formattet Line with Tabstops */
void TL( uchar** output, int i, uchar* txt, ... )
{
	va_list	args;
	
	va_start( args, txt );
	
	TABS( output, i );
	PRINT( output, txt, args );
	EL( output );
	
	va_end( args );
}

/* Formattet Output with tabstops, no linebreak */
void T( uchar** output, int i, uchar* txt, ... )
{
	va_list	args;
	
	va_start( args, txt );
	
	TABS( output, i );
	PRINT( output, txt, args );
	
	va_end( args );
}

