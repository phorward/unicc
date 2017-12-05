/* -MODULE----------------------------------------------------------------------
UniCC4C
Copyright (C) 2010 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<AT>phorward<DASH>software<DOT>com

File:	util.c
Author:	Jan Max Meyer
Usage:	UniCC4C utility functions
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

/* Has the Parser Error Resynchronization? */
int get_error_sym( void )
{
	XML_T	sym;
	uchar*	type;

	for( sym = xml_child( xml_child( parser, "symbols" ), "symbol" );
			sym; sym = xml_next( sym ) )
	{
		if( !( type = xml_attr( sym, "terminal-type" ) ) )
			continue;

		if( pstrstr( type, "error-resync" ) == type )
			return (int)xml_int_attr( sym, "id" );
	}

	return -1;
}

/* Get value type */
int get_value_type( uchar* type_name )
{
	XML_T	val_type;

	if( !( val_type = xml_child( xml_child( parser, "value-types" ),
						"value-type" ) ) )
		return -1;

	for( val_type = xml_child( val_type, "value-type" ); val_type;
			val_type = xml_next( val_type ) )
	{
		if( pstrstr( xml_attr( val_type, "cname" ), type_name ) )
			return (int)xml_int_attr( val_type, "id" );
	}

	return -1;
}

