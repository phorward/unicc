/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2017 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com ++ unicc<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	p_virtual.c
Author:	Jan Max Meyer
Usage:	Virtual production generation functions
----------------------------------------------------------------------------- */

#include "unicc.h"

/** Creates a positive closure for a symbol.

//parser// is the Parser information structure.
//base// is the base symbol.

Returns a SYMBOL* sointer to the symbol representing the closure nonterminal. */
SYMBOL* p_positive_closure( PARSER* parser, SYMBOL* base )
{
	char*	deriv_str;
	PROD*	p;
	SYMBOL*	s			= (SYMBOL*)NULL;

	/*
	30.09.2010	Jan Max Meyer:
	Inherit defined_at information
	*/

	if( base )
	{
		deriv_str = p_derivation_name( base->name, P_POSITIVE_CLOSURE );

		if( !( s = p_get_symbol( parser, deriv_str,
						SYM_NON_TERMINAL, FALSE ) ) )
		{
			s = p_get_symbol( parser, deriv_str,
					SYM_NON_TERMINAL, TRUE );
			s->generated = TRUE;
			s->used = TRUE;
			s->defined = TRUE;
			s->vtype = base->vtype;
			s->derived_from = base;
			s->line = base->line;

			p = p_create_production( parser, s );
			p_append_to_production( p, s, (char*)NULL );
			p_append_to_production( p, base, (char*)NULL );

			p = p_create_production( parser, s );
			p_append_to_production( p, base, (char*)NULL );
		}

		pfree( deriv_str );
	}

	return s;
}

/** Creates a kleene closure for a symbol.

//parser// is the parser information structure.
//base// is the base symbol.

Returns a SYMBOL* Pointer to symbol representing the closure nonterminal. */
SYMBOL* p_kleene_closure( PARSER* parser, SYMBOL* base )
{
	char*	deriv_str;
	PROD*	p;
	SYMBOL*	s			= (SYMBOL*)NULL;
	SYMBOL*	pos_s		= (SYMBOL*)NULL;

	/*
	14.05.2008	Jan Max Meyer
	Modified rework. Instead of

		s* -> s* base | ;

	this will now create

		s+ -> s+ base | base;
		s* -> s+ | ;


	30.09.2010	Jan Max Meyer
	Inherit defined_at information
	*/

	if( base )
	{
		pos_s = p_positive_closure( parser, base );
		if( !pos_s )
			return s;

		deriv_str = p_derivation_name( base->name, P_KLEENE_CLOSURE );

		if( !( s = p_get_symbol( parser, deriv_str,
					SYM_NON_TERMINAL, FALSE ) ) )
		{
			s = p_get_symbol( parser, deriv_str,
					SYM_NON_TERMINAL, TRUE );
			s->generated = TRUE;
			s->used = TRUE;
			s->defined = TRUE;
			s->vtype = base->vtype;
			s->derived_from = base;
			s->line = base->line;

			p = p_create_production( parser, s );
			/*p_append_to_production( p, s, (char*)NULL );
			p_append_to_production( p, base, (char*)NULL );*/
			p_append_to_production( p, pos_s, (char*)NULL );

			p = p_create_production( parser, s );
		}

		pfree( deriv_str );
	}

	return s;
}

/** Creates an optional closure for a symbol.

//parser// is the parser information structure.
//base// is the base symbol.

Returns a SYMBOL* Pointer to symbol representing the closure nonterminal.
*/
SYMBOL* p_optional_closure( PARSER* parser, SYMBOL* base )
{
	char*	deriv_str;
	PROD*	p;
	SYMBOL*	s			= (SYMBOL*)NULL;

	/*
	30.09.2010	Jan Max Meyer:
	Inherit defined_at information
	*/

	if( base )
	{
		deriv_str = p_derivation_name( base->name, P_OPTIONAL_CLOSURE );

		if( !(s = p_get_symbol( parser, deriv_str,
					SYM_NON_TERMINAL, FALSE ) ) )
		{
			s = p_get_symbol( parser, deriv_str,
					SYM_NON_TERMINAL, TRUE );
			s->generated = TRUE;
			s->used = TRUE;
			s->defined = TRUE;
			s->vtype = base->vtype;
			s->derived_from = base;
			s->line = base->line;

			p = p_create_production( parser, s );
			p_append_to_production( p, base, (char*)NULL );

			p = p_create_production( parser, s );
		}

		pfree( deriv_str );
	}

	return s;
}

