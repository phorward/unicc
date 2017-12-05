/* -MODULE----------------------------------------------------------------------
UniCC4C
Copyright (C) 2010 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<AT>phorward<DASH>software<DOT>com

File:	gen.misc.c
Author:	Jan Max Meyer
Usage:	Miscelleanous code generator functions
----------------------------------------------------------------------------- */

/*
 * Includes
 */
#include "unicc4c.h"

/*
 * Global variables
 */
extern	char*		prefix;
extern	XML_T		parser;

/*
 * Defines
 */

/*
 * Functions
 */

/* -----------------------------------------------------------------------------
	Print symbol table
----------------------------------------------------------------------------- */
void print_symbol_name_tab( uchar* tab )
{
	XML_T	symbols;
	XML_T	symbol;
	
	symbols = xml_child( parser, "symbols" );
	
	printf( "%s %s_%s[ %d ] = \n{\n	", STATIC, prefix, tab,
		xml_count( xml_child( symbols, "symbol" ) ) );
	
	for( symbol = xml_child( symbols, "symbol" ); symbol;
			symbol = xml_next( symbol ) )
	{
		/* TODO: Escape sequences */
		printf( "\"%s\"", xml_attr( symbol, "name" ) );
		if( xml_next( symbol ) )
			printf( ", " );
	}
	
	printf( "\n};\n\n" );
}

/* -----------------------------------------------------------------------------
	Print whitespace identification table
----------------------------------------------------------------------------- */
void print_whitespace_tab( uchar* tab )
{
	XML_T	symbols;
	XML_T	symbol;
	
	symbols = xml_child( parser, "symbols" );
	
	printf( "%s %s_%s[ %d ] = \n{\n	", STATIC, prefix, tab,
		xml_count( xml_child( symbols, "symbol" ) ) );
	
	for( symbol = xml_child( symbols, "symbol" ); symbol;
			symbol = xml_next( symbol ) )
	{
		/* TODO: Escape sequences */
		printf( "\"%s\"", xml_attr( symbol, "name" ) );
		if( xml_next( symbol ) )
			printf( ", " );
	}
	
	printf( "\n};\n\n" );
}

