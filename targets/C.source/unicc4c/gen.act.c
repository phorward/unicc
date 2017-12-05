/* -MODULE----------------------------------------------------------------------
UniCC4C
Copyright (C) 2010 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<AT>phorward<DASH>software<DOT>com

File:	gen.act.c
Author:	Jan Max Meyer
Usage:	Action Table related code generator functions
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
	Print action table (normally not used!)
----------------------------------------------------------------------------- */
void print_act_tab( uchar* tab )
{
	XML_T	sym;
	XML_T	states;
	XML_T	state;
	XML_T	entry;
	uchar*	act;
	int		max;
	int		count;
	int		state_cnt;

	states = xml_child( parser, "states" );

	for( state = xml_child( states, "state" ), max = 0; state;
			state = xml_next( state ) )
	{
		count = xml_count( xml_child( state, "shift" ) )
					+ xml_count( xml_child( state, "reduce" ) )
						+ xml_count( xml_child( state, "shift-reduce" ) );
		
		if( count > max )
			max = count;
	}

	state_cnt = xml_count( xml_child(
					xml_child( parser, "states" ), "state" ) );

	printf( "%s %s_%s[ %d ][ %d ] = \n{\n",
				STATIC, prefix, tab, state_cnt, max * 3 + 1 );

	for( state = xml_child( states, "state" ); state;
			state = xml_next( state ) )
	{
		count = xml_count( xml_child( state, "shift" ) )
					+ xml_count( xml_child( state, "reduce" ) )
						+ xml_count( xml_child( state, "shift-reduce" ) );

		printf( "\t{ %d", count );

		for( entry = xml_child( state, "reduce" ); entry;
				entry = xml_next( entry ) )
			printf( ", %s,%d,%s ", xml_attr( entry, "symbol-id" ), REDUCE,
						xml_attr( entry, "by-production" ) );

		for( entry = xml_child( state, "shift" ); entry; 
				entry = xml_next( entry ) )
			printf( ", %s,%d,%s ", xml_attr( entry, "symbol-id" ), SHIFT,
						xml_attr( entry, "to-state" ) );

		for( entry = xml_child( state, "shift-reduce" ); entry;
				entry = xml_next( entry ) )
			printf( ", %s,%d,%s ", xml_attr( entry, "symbol-id" ),
						SHIFT | REDUCE,
							xml_attr( entry, "by-production" ) );

		printf( " }%s\n", xml_next( state ) ? "," : "" );
	}

	printf( "};\n\n" );
}

/* -----------------------------------------------------------------------------
	Print action table as a generated function
----------------------------------------------------------------------------- */
uchar* print_act_fn( uchar* fn )
{
	XML_T		sym;
	XML_T		states;
	XML_T		state;
	XML_T		entry;
	pboolean	first;
	uchar*		out		= (char*)NULL;
	
	states = xml_child( parser, "states" );
	
	L( &out, "%s int %s_%s( _pcb* pcb )", STATIC, prefix, fn );
	L( &out, "{" );
	L( &out, "	/* Select shift, reduce or shift&reduct action */" );
	L( &out, "	switch( *( pcb->tos ) )" );
	L( &out, "	{" );
	
	for( state = xml_child( states, "state" ); state;
			state = xml_next( state ) )
	{
		if( xml_count( xml_child( state, "shift" ) )
				+ xml_count( xml_child( state, "reduce" ) )
					+ xml_count( xml_child( state, "shift-reduce" ) ) == 0 )
			continue;

		L( &out, "		case %s:", xml_attr( state, "id" ) );
		L( &out, "			switch( pcb->sym )" );
		L( &out, "			{" );

		for( entry = xml_child( state, "reduce" ); entry;
				entry = xml_next( entry ) )
		{
			L( &out, "				case %s:", xml_attr( entry, "symbol-id" ) );
			L( &out, "					pcb->act = REDUCE;" );
			L( &out, "					pcb->idx = %s; /* by Production */",
										xml_attr( entry, "by-production" ) );
			L( &out, "					return 1;" );
		}
		
		for( entry = xml_child( state, "shift" ); entry;
				entry = xml_next( entry ) )
		{
			L( &out, "				case %s:", xml_attr( entry, "symbol-id" ) );
			L( &out, "					pcb->act = SHIFT;" );
			L( &out, "					pcb->idx = %s; /* to State */",
										xml_attr( entry, "to-state" ) );
			L( &out, "					return 1;" );
		}
		
		for( entry = xml_child( state, "shift-reduce" ); entry;
				entry = xml_next( entry ) )
		{
			L( &out, "				case %s:", xml_attr( entry, "symbol-id" ) );
			L( &out, "					pcb->act = SHIFT | REDUCE;" );
			L( &out, "					pcb->idx = %s; /* by Production */",
										xml_attr( entry, "by-production" ) );
			L( &out, "					return 1;" );
		}
		
		L( &out, "			}" );
		L( &out, "			break;" );
	}
	
	L( &out, "	}" );

	EL( &out );

	for( state = xml_child( states, "state" ), first = TRUE; state;
			state = xml_next( state ) )
	{
		if( !xml_attr( state, "default-production" ) )
			continue;

		if( first )
		{
			L( &out, "	/* Select default production for reduction */" );
			L( &out, "	switch( *( pcb->tos ) )" );
			L( &out, "	{" );
			first = FALSE;
		}

		L( &out, "		case %s:", xml_attr( state, "id" ) );
		L( &out, "			pcb->act = REDUCE;" );
		L( &out, "			pcb->idx = %s; /* by Production */",
							xml_attr( state, "default-production" ) );
		L( &out, "			return 1;" );

		if( !first && !xml_next( state ) )
			L( &out, "	}" );
	}	
	
	EL( &out );
	
	L( &out, "	return 0;" );
	L( &out, "}" );
	
	return out;
}
