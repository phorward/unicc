/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator 
Copyright (C) 2006-2009 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ mail@phorward-software.com

File:	p_virtual.c
Author:	Jan Max Meyer
Usage:	Virtual production generation functions
----------------------------------------------------------------------------- */

/*
 * Includes
 */
#include "p_global.h"
#include "p_error.h"
#include "p_proto.h"

/*
 * Global variables
 */


/*
 * Functions
 */

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_positive_closure()
	
	Author:			Jan Max Meyer
	
	Usage:			Creates a positive closure for a symbol.
					
	Parameters:		PARSER*		parser				Parser information structure
					SYMBOL*		base				Base symbol
	
	Returns:		SYMBOL*							Pointer to symbol representing
													the closure nonterminal.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
SYMBOL* p_positive_closure( PARSER* parser, SYMBOL* base )
{
	uchar*	deriv_str;
	PROD*	p;
	SYMBOL*	s			= (SYMBOL*)NULL;

	if( base )
	{
		deriv_str = p_derivation_name( base->name, P_POSITIVE_CLOSURE );

		if( !( s = p_get_symbol( parser, deriv_str, SYM_NON_TERMINAL, FALSE ) ) )
		{
			s = p_get_symbol( parser, deriv_str, SYM_NON_TERMINAL, TRUE );
			s->generated = TRUE;
			s->used = TRUE;
			s->defined = TRUE;
			s->vtype = base->vtype;

			p = p_create_production( parser, s );
			p_append_to_production( p, s, (uchar*)NULL );
			p_append_to_production( p, base, (uchar*)NULL );
			
			p = p_create_production( parser, s );
			p_append_to_production( p, base, (uchar*)NULL );
		}

		p_free( deriv_str );
	}

	return s;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_kleene_closure()
	
	Author:			Jan Max Meyer
	
	Usage:			Creates a kleene closure for a symbol.
					
	Parameters:		PARSER*		parser				Parser information structure
					SYMBOL*		base				Base symbol
	
	Returns:		SYMBOL*							Pointer to symbol representing
													the closure nonterminal.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	14.05.2008	Jan Max Meyer	Modified rework. Instead of

								s* -> s* base | ;

								this will now create

								s+ -> s+ base | base;
								s* -> s+ | ;
----------------------------------------------------------------------------- */
SYMBOL* p_kleene_closure( PARSER* parser, SYMBOL* base )
{
	uchar*	deriv_str;
	PROD*	p;
	SYMBOL*	s			= (SYMBOL*)NULL;
	SYMBOL*	pos_s		= (SYMBOL*)NULL;

	if( base )
	{
		pos_s = p_positive_closure( parser, base );
		if( !pos_s )
			return s;

		deriv_str = p_derivation_name( base->name, P_KLEENE_CLOSURE );

		if( !( s = p_get_symbol( parser, deriv_str, SYM_NON_TERMINAL, FALSE ) ) )
		{
			s = p_get_symbol( parser, deriv_str, SYM_NON_TERMINAL, TRUE );
			s->generated = TRUE;
			s->used = TRUE;
			s->defined = TRUE;
			s->vtype = base->vtype;

			p = p_create_production( parser, s );
			/*p_append_to_production( p, s, (uchar*)NULL );
			p_append_to_production( p, base, (uchar*)NULL );*/
			p_append_to_production( p, pos_s, (uchar*)NULL );
		
			p = p_create_production( parser, s );
		}

		p_free( deriv_str );
	}

	return s;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_optional_closure()
	
	Author:			Jan Max Meyer
	
	Usage:			Creates an optional closure for a symbol.
					
	Parameters:		PARSER*		parser				Parser information structure
					SYMBOL*		base				Base symbol
	
	Returns:		SYMBOL*							Pointer to symbol representing
													the closure nonterminal.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
SYMBOL* p_optional_closure( PARSER* parser, SYMBOL* base )
{
	uchar*	deriv_str;
	PROD*	p;
	SYMBOL*	s			= (SYMBOL*)NULL;

	if( base )
	{
		deriv_str = p_derivation_name( base->name, P_OPTIONAL_CLOSURE );

		if( !(s = p_get_symbol( parser, deriv_str, SYM_NON_TERMINAL, FALSE ) ) )
		{
			s = p_get_symbol( parser, deriv_str, SYM_NON_TERMINAL, TRUE );
			s->generated = TRUE;
			s->used = TRUE;
			s->defined = TRUE;
			s->vtype = base->vtype;

			p = p_create_production( parser, s );
			p_append_to_production( p, base, (uchar*)NULL );
			
			p = p_create_production( parser, s );
		}

		p_free( deriv_str );
	}

	return s;
}

