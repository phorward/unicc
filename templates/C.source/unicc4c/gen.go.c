/* -MODULE----------------------------------------------------------------------
UniCC4C
Copyright (C) 2010 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<AT>phorward<DASH>software<DOT>com

File:	gen.go.c
Author:	Jan Max Meyer
Usage:	Goto-Table related code generator functions
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
	Print goto table (this is not the normal way)
----------------------------------------------------------------------------- */
void print_go_tab( uchar* tab )
{
	XML_T	sym;
	XML_T	states;
	XML_T	state;
	XML_T	entry;
	uchar*	act;
	int		max;
	int		state_cnt;
	int		count;

	states = xml_child( parser, "states" );

	for( state = xml_child( states, "state" ), max = 0; state;
			state = xml_next( state ) )
		if( ( count = xml_count( xml_child( state, "goto" ) ) ) > max )
			max = count;

	state_cnt = xml_count( xml_child(
					xml_child( parser, "states" ), "state" ) );

	printf( "%s %s_%s[ %d ][ %d ] = \n{\n",
				STATIC, prefix, tab, state_cnt, max * 3 + 1 );

	for( state = xml_child( states, "state" ); state;
			state = xml_next( state ) )
	{
		printf( "\t{ %d", xml_count( xml_child( state, "goto" ) ) );

		for( entry = xml_child( state, "goto" ); entry;
				entry = xml_next( entry ) )
		{
			if( xml_attr( entry, "by-production" ) )
				printf( ", %s,%d,%s ", xml_attr( entry, "symbol-id" ),
					SHIFT | REDUCE, xml_attr( entry, "by-production" ) );
			else
				printf( ", %s,%d,%s ", xml_attr( entry, "symbol-id" ), SHIFT,
							xml_attr( entry, "to-state" ) );
		}

		printf( " }%s\n", xml_next( state ) ? "," : "" );
	}

	printf( "};\n\n" );
}

/* -----------------------------------------------------------------------------
	Print goto table as a function
----------------------------------------------------------------------------- */
uchar* print_go_fn( uchar* fn )
{
	XML_T	sym;
	XML_T	states;
	XML_T	state;
	XML_T	entry;
	uchar*	out			= (char*)NULL;
	
	states = xml_child( parser, "states" );
	
	L( &out, "%s int %s_%s( _pcb* pcb )", STATIC, prefix, fn );
	L( &out, "{" );
	L( &out, "	/* Select goto action */" );
	L( &out, "	switch( *( pcb->tos ) )" );
	L( &out, "	{" );
	
	for( state = xml_child( states, "state" ); state;
			state = xml_next( state ) )
	{
		if( xml_count( xml_child( state, "goto" ) ) == 0 )
			continue;

		L( &out, "		case %s:", xml_attr( state, "id" ) );
		L( &out, "			switch( pcb->sym )" );
		L( &out, "			{" );

		for( entry = xml_child( state, "goto" ); entry;
				entry = xml_next( entry ) )
		{
			L( &out, "				case %s:", xml_attr( entry, "symbol-id" ) );
			
			if( xml_attr( entry, "by-production" ) )
			{
				L( &out, "					pcb->act = SHIFT | REDUCE;" );
				L( &out, "					pcb->idx = %s; /* by Production */",
											xml_attr( entry,
												"by-production" ) );

			}
			else
			{
				L( &out, "					pcb->act = SHIFT;" );
				L( &out, "					pcb->idx = %s; /* to State */",
											xml_attr( entry, "to-state" ) );
			}
			L( &out, "					return 1;" );
		}
		
		L( &out, "			}" );
		L( &out, "			break;" );
	}
	
	L( &out, "	}" );
	
	EL( &out );
	
	L( &out, "	return 0;" );
	L( &out, "}" );
	
	return out;
}

