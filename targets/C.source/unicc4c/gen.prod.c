/* -MODULE----------------------------------------------------------------------
UniCC4C
Copyright (C) 2010 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<AT>phorward<DASH>software<DOT>com

File:	gen.prod.c
Author:	Jan Max Meyer
Usage:	Production-related code generating functions
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
	Print production length table
----------------------------------------------------------------------------- */
void print_prod_len_tab( uchar* tab )
{
	XML_T	productions;
	XML_T	production;
	int		prod_cnt;
	
	productions = xml_child( parser, "productions" );
	
	prod_cnt = xml_count( xml_child(
					xml_child( parser, "productions" ), "production" ) );
	printf( "%s %s_%s[ %d ] = \n{\n	", STATIC, prefix, tab, prod_cnt );
	
	for( production = xml_child( productions, "production" ); production;
			production = xml_next( production ) )
	{
		printf( "%s", xml_attr( production, "length" ) );
		if( xml_next( production ) )
			printf( ", " );
	}
	
	printf( "\n};\n\n" );
}

/* -----------------------------------------------------------------------------
	Print production left-hand side table
----------------------------------------------------------------------------- */
void print_prod_lhs_tab( uchar* tab )
{
	XML_T	productions;
	XML_T	production;
	XML_T	lhs;
	int		max;
	int		count;
	int		prod_cnt;
	
	productions = xml_child( parser, "productions" );
	
	for( production = xml_child( productions, "production" ), max = 0;
			production; production = xml_next( production ) )
	{
		if( ( count = xml_count( xml_child( production, "left-hand-side" ) ) )
				> max )
			max = count;
	}
	
	prod_cnt = xml_count( xml_child(
					xml_child( parser, "productions" ), "production" ) );

	printf( "%s %s_%s[ %d ][ %d ] = \n{\n",
				STATIC, prefix, tab, prod_cnt, max + 1 );
	
	for( production = xml_child( productions, "production" ); production;
			production = xml_next( production ) )
	{
		printf( "\t{ %d, ",
				xml_count( xml_child( production, "left-hand-side" ) ) );
		
		for( lhs = xml_child( production, "left-hand-side" ); lhs;
				lhs = xml_next( lhs ) )
		{
			printf( "%s", xml_attr( lhs, "symbol-id" ) );
			if( xml_next( lhs ) )
				printf( ", " );
		}
		
		printf( " }%s\n", xml_next( production ) ? "," : "" );
	}
	
	printf( "};\n\n" );
}

/* -----------------------------------------------------------------------------
	Print default production table
----------------------------------------------------------------------------- */
void print_def_prod_tab( uchar* tab )
{
	XML_T	states;
	XML_T	state;
	
	states = xml_child( parser, "states" );
	
	printf( "%s %s_%s[ %d ] = \n{\n	", STATIC, prefix, tab,
		xml_count( xml_child( states, "state" ) ) );
	
	for( state = xml_child( states, "state" ); state;
			state = xml_next( state ) )
	{
		printf( "%s", xml_attr( state, "default-production" ) ?
						xml_attr( state, "default-production" ) : "-1" );
		if( xml_next( state ) )
			printf( ", " );
	}
	
	printf( "\n};\n\n" );
}

static void print_action_variable( uchar** out, XML_T var )
{
	XML_T	val_type;
	uchar*	var_valtype;
	int		valtype		= -1;

	if(	( val_type = xml_child( parser, "value-types" ) ) )
	{
		val_type = xml_child( val_type, "value-type" );
		if( ( var_valtype = xml_attr( var, "value-type" ) ) )
		{
			for( val_type = xml_child( val_type, "value-type" );
					val_type; val_type = xml_next( val_type ) )
			{
			}
		}
		else
			valtype = 0;
	}

	if( !pstrcmp( xml_attr( var, "type" ), "left-hand-side" ) )
	{
	}

}

/* -----------------------------------------------------------------------------
	Print actions
----------------------------------------------------------------------------- */
uchar* print_actions( void )
{
	XML_T	productions;
	XML_T	production;
	XML_T	code;
	XML_T	part;
	uchar*	out				= (uchar*)NULL;

	productions = xml_child( parser, "productions" );
	for( production = xml_child( productions, "production" );
			production; production = xml_next( production ) )
	{
		if( !( code = xml_child( production, "code" ) ) )
			continue;

		L( &out, "		case %d:", xml_int_attr( production, "id" ) );
		L( &out, "		{" );

		for( part = xml_child( code, (char*)NULL );
				part; part = xml_next_inorder( part ) )
		{
			if( !pstrcmp( xml_name( part ), "raw" ) )
				T( &out, 0, "%s", xml_txt( part ) );
			else if( !pstrcmp( xml_name( part ), "variable" ) )
			{
			}
		}

		L( &out, "		}" );
	}
}

