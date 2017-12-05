/* -MODULE----------------------------------------------------------------------
UniCC4C
Copyright (C) 2010 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<AT>phorward<DASH>software<DOT>com

File:	gen.c
Author:	Jan Max Meyer
Usage:	Code generator functions
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
	General Parser Print function
----------------------------------------------------------------------------- */
uchar* gen( void )
{
	uchar*	out		= (uchar*)NULL;
	uchar*	final	= (uchar*)NULL;
	int	error_sym	= get_error_sym();

#ifndef MAKE_PROTOTYPES
	#include "PARSER.c"
#endif

	if( !( final = pstr_render( out,
			VARPREF	"prefix", prefix, FALSE,
			VARPREF "name", xml_attr( parser, "name" ), FALSE,

			VARPREF UNICC4C "_print_act_fn()",
				print_act_fn( "get_act" ), TRUE,
			VARPREF UNICC4C "_print_go_fn()",
				print_go_fn( "get_go" ), TRUE,
			VARPREF UNICC4C "_print_lex_fn()",
				print_lex_fn( "lex" ), TRUE,
			/*
			VARPREF "actions",
				print_actions(), TRUE,
			*/

			(uchar*)NULL

			) ) )
		OUTOFMEM;

	pfree( out );

	return final;
}

