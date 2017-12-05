/* -MODULE----------------------------------------------------------------------
UniCC4C
Copyright (C) 2010 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<AT>phorward<DASH>software<DOT>com

File:	gen.lex.c
Author:	Jan Max Meyer
Usage:	Utility functions for generating the lexer function
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

static CCL ccl_from_xml( XML_T cclx )
{
	CCL		ccl = (CCL)NULL;
	XML_T	range;

	if( pstrcmp( xml_name( cclx ), "character-class" ) != 0 )
		return (CCL)NULL;

	for( range = xml_child( cclx, "range" ); range;
			range = xml_next( range ) )
	{
		ccl = ccl_addrange( ccl, (pchar)xml_int_attr( range, "from" ),
									(pchar)xml_int_attr( range, "to" ) );

	}

	return ccl;
}

static void ccl_to_condition( uchar** out, int t, uchar* ifstr,
								uchar* var, XML_T cclx )
{
	CCL		origin;
	CCL		complement;
	CCL		c;
	int		i;

	if( ifstr )
		T( out, t, "%s", ifstr );

	origin = ccl_from_xml( cclx );
	complement = ccl_negate( ccl_dup( origin ) );

	if( ccl_count( origin ) < ccl_count( complement ) )
		ccl_free( complement );
	else
	{
		ccl_free( origin );
		origin = complement;
		T( out, 0, " !( " );
	}

	for( c = origin, i = 0; !ccl_end( c ); c++, i++ )
	{
		if( i )
			EL( out );

		if( c->begin == c->end )
			T( out, !i ? 0 : t, "%s next == %d",
				( !i ? "" : "\t||" ), c->begin );
		else
			T( out, !i ? 0 : t, "%s ( next >= %d && next <= %d )",
				( !i ? "" : "\t||" ), c->begin, c->end );
	}

	if( origin == complement )
		T( out, 0, " )" );

	if( ifstr )
		T( out, 0, " )" );

	EL( out );

	ccl_free( origin );
}

/* -----------------------------------------------------------------------------
	Print one lexer
----------------------------------------------------------------------------- */
static void print_lexer( uchar** out, int t, XML_T lexer )
{
	XML_T		state;
	XML_T		trans;
	XML_T		cclass;
	XML_T		range;
	BOOLEAN		first;
	uchar*		ifstr;
	int			i;
	int			tr;

	TL( out, t, 		"while( dfa_st != -1 )" );
	TL( out, t, 		"{" );
	TL( out, ++t, 			"next = _get_input( pcb, ++len );" );
	EL( out );
	TL( out, t,		"switch( dfa_st )" );
	TL( out, t,			"{" );
	
	for( state = xml_child( lexer, "state" ); state;
			state = xml_next( state ) )
	{
		TL( out, ++t,			"case %s:", xml_attr( state, "id" ) );

		t++;

		if( xml_attr( state, "accept" ) )
			TL( out, t,				"pcb->sym = %s;",
										xml_attr( state, "accept" ) );

		for( trans = xml_child( state, "transition" ), ifstr = (uchar*)NULL;
				trans; trans = xml_next( trans ) )
		{
			if( pstrcmp( xml_attr( trans, "goto" ), 
							xml_attr( state, "default-transition" ) ) == 0 )
				continue;

			if( !ifstr )
				ifstr = "if(";
			else
				ifstr = "else if(";

			ccl_to_condition( out, t, ifstr, "next",
				xml_child( trans, "character-class" ) );

			TL( out, t+1,			"dfa_st = %s;", xml_attr( trans, "goto" ) );
		}
		
		
		if( xml_attr( state, "default-transition" ) )
			tr = xml_int_attr( state, "default-transition" );
		else
			tr = -1;

		if( ifstr )
		{
			TL( out, t, 				"else" );
			TL( out, t + 1,				"dfa_st = %d;", tr );
		}
		else
			TL( out, t,				"dfa_st = %d;", tr );

		TL( out, t--,				"break;" );
		t--;
	}

	TL( out, t--,		"}" );
	TL( out, t--,	"}" );
}

/* -----------------------------------------------------------------------------
	Print lexer function
----------------------------------------------------------------------------- */
uchar* print_lex_fn( uchar* fn )
{
	XML_T	states;
	XML_T	lexers;
	XML_T	lexer;
	XML_T	state;
	XML_T	symbols;
	XML_T	symbol;
	int		count;
	int		white		= 0;
	int		t			= 1;
	int		i;
	uchar*	out			= (uchar*)NULL;
	
	/* Are lexers defined? */
	if( !( lexers = xml_child( parser, "lexers" ) ) )
		return;
	
	if( !( count = xml_count( xml_child( lexers, "lexer" ) ) ) )
		/* Anyway, no lexers defined! */
		return;

	/* Get number of whitespace tokens */
	symbols = xml_child( parser, "symbols" );

	for( symbol = xml_child( symbols, "symbol" ); symbol;
			symbol = xml_next( symbol ) )
		if( xml_attr( symbol, "is-whitespace" ) )
			white++;

	L( &out, "%s int %s_%s( _pcb* pcb )", STATIC, prefix, fn );
	L( &out, "{" );
	
	L( &out, "	int		dfa_st	= 0;" );
	L( &out, "	int		chr;" );
	L( &out, "	int		len		= 0;" );
	L( &out, "	int		next;" );
	EL( &out );
	L( &out, "	pcb->sym = -1;" );
	EL( &out );

	if( white )
	{
		TL( &out, t, 	"do" );
		TL( &out, t, 	"{" );
		t++;
		TL( &out, t,	"if( pcb->sym > -1 )" );
		TL( &out, t, 	"	%s_clear_input( pcb );", prefix );
		TL( &out, t, 	"pcb->sym = -1;" );
		EL( &out );
	}
		
	if( count == 1 )
		print_lexer( &out, t, xml_child( lexers, "lexer" ) );
	else
	{
		states = xml_child( parser, "states" );
		
		t++;
		TL( &out, t, "/* Select lexer by state */" );
		TL( &out, t, "switch( *( pcb->tos ) )" );
		TL( &out, t, "{" );
		
		for( lexer = xml_child( lexers, "lexer" ); lexer;
				lexer = xml_next( lexer ) )
		{
			for( state = xml_child( states, "state" ); state;
					state = xml_next( state ) )
				if( pstrcmp( xml_attr( state, "lexer" ),
						xml_attr( lexer, "id" ) ) == 0 )
					TL( &out, t + 1, "case %s:", xml_attr( state, "id" ) );

			print_lexer( &out, t + 2, lexer );

			TL( &out, t + 2, "break;" );
		}

		TL( &out, t, "}" );
	}

	if( white )
	{
		t--;
		TL( &out, t,	"}" );
		T( &out, t,	"while( " );
		for( symbol = xml_child( symbols, "symbol" ), i = 0; symbol;
				symbol = xml_next( symbol ) )
		{
			if( xml_attr( symbol, "is-whitespace" ) )
			{
				T( &out, 0, "pcb->sym == %s %s", xml_attr( symbol, "id" ),
					++i < white ? "||" : "" );
			}
		}

		TL( &out, 0, ");" );
	}

	EL( &out );
	L( &out, "	return ( pcb->sym >= 0 ) ? 1 : 0;" );
	
	L( &out, "}" );
	
	return out;
}

