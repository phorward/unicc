/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2013 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com/ ++ unicc<<AT>>phorward-software<<DOT>>com

File:	p_keywords.c
Author:	Jan Max Meyer
Usage:	Turns regular expression definitions into deterministic state
		machines, by using the Phorward regular expression library.

You may use, modify and distribute this software under the terms and conditions
of the Artistic License, version 2. Please see LICENSE for more information.
----------------------------------------------------------------------------- */

/*
	A note on history:
	Until UniCC v0.24, the keyword-only feature has been extended to
	entrie regular expressions. Later on in UniCC 0.27, the term
	"keyword" was renamed to "string", and the classification of the
	various terminals was not that strong anymore than before. So this
	is the reason why everything in here is still called "keyword",
	altought it means any terminal in general. All terminals are now
	put into one lexical analysis part, since UniCC 0.27.
*/

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
	Function:		p_keywords_to_dfa()

	Author:			Jan Max Meyer

	Usage:			Converts the keywords within the states into a DFA, and
					maybe re-uses state machines matching the same pool of
					keywords.

	Parameters:		PARSER*		parser				Pointer to parser
													information structure.

	Returns:		void

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_keywords_to_dfa( PARSER* parser )
{
	pregex_nfa	nfa;
	pregex_dfa	dfa;
	pregex_dfa*	ex_dfa;
	LIST*	l;
	LIST*	m;
	STATE*	s;
	TABCOL*	col;

	PROC( "p_keywords_to_dfa" );
	PARMS( "parser", "%p", parser );

	LISTFOR( parser->lalr_states, l )
	{
		s = (STATE*)list_access( l );

		VARS( "s->state_id", "%d", s->state_id );
		memset( &nfa, 0, sizeof( pregex_nfa ) );

		/* Construct NFAs from keywords */
		LISTFOR( s->actions, m )
		{
			col = (TABCOL*)list_access( m );
			p_symbol_to_nfa( parser, &nfa, col->symbol );
		}

		/* Construct DFA, if NFA has been constructed */
		VARS( "list_count( nfa.states )", "%d", list_count( nfa.states ) );
		if( list_count( nfa.states ) )
		{
			MSG( "Constructing DFA from NFA" );
			if( pregex_dfa_from_nfa( &dfa, &nfa ) < ERR_OK )
				OUTOFMEM;

			VARS( "list_count( dfa.states )", "%d",
					list_count( dfa.states ) );

			MSG( "Freeing NFA" );
			pregex_nfa_free( &nfa );

			MSG( "Minimizing DFA" );
			if( pregex_dfa_minimize( &dfa ) < ERR_OK )
				OUTOFMEM;

			VARS( "list_count( dfa.states )", "%d",
					list_count( dfa.states ) );

			if( ( ex_dfa = p_find_equal_dfa( parser, &dfa ) ) )
			{
				MSG( "An equal DFA exists; Freeing temporary one!" );
				pregex_dfa_free( &dfa );
			}
			else
			{
				MSG( "This DFA does not exist in pool yet - integrating!" );
				if( !( ex_dfa = (pregex_dfa*)pmemdup(
									&dfa, sizeof( pregex_dfa ) ) ) )
					OUTOFMEM;

				if( !( parser->kw = list_push( parser->kw, (void*)ex_dfa ) ) )
					OUTOFMEM;
			}

			VARS( "ex_dfa", "%p", ex_dfa );
			s->dfa = ex_dfa;
		}
	}

	VOIDRET;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_single_lexer()

	Author:			Jan Max Meyer

	Usage:			Constructs a single DFA for a general token lexer.

	Parameters:		PARSER*		parser				Pointer to parser
													information structure.

	Returns:		void

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_single_lexer( PARSER* parser )
{
	pregex_nfa			nfa;
	pregex_dfa			dfa;
	pregex_dfa*			pdfa;
	LIST*				l;
	SYMBOL*				s;

	PROC( "p_single_lexer" );
	PARMS( "parser", "%p", parser );

	MSG( "Constructing NFA" );
	memset( &nfa, 0, sizeof( pregex_nfa ) );
	LISTFOR( parser->symbols, l )
	{
		s = (SYMBOL*)list_access( l );
		VARS( "s->id", "%d", s->id );

		p_symbol_to_nfa( parser, &nfa, s );
	}

	/* Construct DFA, if NFA has been constructed */
	VARS( "list_count( nfa.states )", "%d", list_count( nfa.states ) );
	if( list_count( nfa.states ) )
	{
		MSG( "Constructing DFA from NFA" );
		if( pregex_dfa_from_nfa( &dfa, &nfa ) < ERR_OK )
			OUTOFMEM;

		VARS( "list_count( dfa.states )", "%d",
				list_count( dfa.states ) );

		MSG( "Freeing NFA" );
		pregex_nfa_free( &nfa );

		MSG( "Minimizing DFA" );
		if( pregex_dfa_minimize( &dfa ) < ERR_OK )
			OUTOFMEM;

		VARS( "list_count( dfa.states )", "%d",
				list_count( dfa.states ) );

		if( !( pdfa = (pregex_dfa*)pmemdup( &dfa, sizeof( pregex_dfa ) ) ) )
			OUTOFMEM;

		if( !( parser->kw = list_push( parser->kw, (void*)pdfa ) ) )
			OUTOFMEM;
	}

	VOIDRET;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_find_equal_dfa()

	Author:			Jan Max Meyer

	Usage:			Walks trough the DFA machines of the current parser
					definition and tests if the temporary generated DFA contains
					the same states than a one already defined in the parser.

	Parameters:		PARSER*		parser				The parser information
													structure.
					pregex_dfa*	ndfa				Pointer to DFA that is
													compared with the other
													machine already integrated
													into the parser structure.

	Returns:		pregex_dfa*						Returns the pointer to a
													matching DFA, else
													(pregex_dfa*)NULL.

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	19.11.2009	Jan Max Meyer	Revision of entire function, to work with
								structures of the new regex-library.
----------------------------------------------------------------------------- */
pregex_dfa* p_find_equal_dfa( PARSER* parser, pregex_dfa* ndfa )
{
	LIST*			l;
	LIST*			m;
	LIST*			n;
	LIST*			o;
	LIST*			p;
	pregex_dfa*		tdfa;
	pregex_dfa_st*	dfa_st		[2];
	pregex_dfa_tr*	dfa_ent		[2];
	BOOLEAN			match;

	PROC( "p_find_equal_dfa" );
	PARMS( "parser", "%p", parser );
	PARMS( "ndfa", "%p", ndfa );

	LISTFOR( parser->kw, l )
	{
		tdfa = (pregex_dfa*)list_access( l );

		VARS( "list_count( tdfa->states )", "%d", list_count( tdfa->states ) );
		VARS( "list_count( ndfa->states )", "%d", list_count( ndfa->states ) );
		if( list_count( tdfa->states ) != list_count( ndfa->states ) )
		{
			MSG( "Number of states does already not match - test next" );
			continue;
		}

		for( m = tdfa->states, n = ndfa->states, match = TRUE;
				m && n && match; m = list_next( m ), n = list_next( n ) )
		{
			dfa_st[0] = (pregex_dfa_st*)list_access( m );
			dfa_st[1] = (pregex_dfa_st*)list_access( n );

			if( !( dfa_st[0]->accept.accept == dfa_st[1]->accept.accept
					&& list_count( dfa_st[0]->trans )
							== list_count( dfa_st[1]->trans ) ) )
			{
				MSG( "Number of transitions or accepting ID does not match" );
				match = FALSE;
				break;
			}

			for( o = dfa_st[0]->trans, p = dfa_st[1]->trans; o && p;
					o = list_next( o ), p = list_next( p ) )
			{
				dfa_ent[0] = list_access( o );
				dfa_ent[1] = list_access( p );

				if( !( ccl_compare( dfa_ent[0]->ccl, dfa_ent[1]->ccl ) == 0
						&& dfa_ent[0]->go_to == dfa_ent[1]->go_to ) )
				{
					MSG( "Deep scan of transitions not equal" );
					match = FALSE;
					break;
				}
			}
		}

		VARS( "match", "%d", match );
		if( match )
		{
			MSG( "DFA matches!" );
			VARS( "tdfa", "%p", tdfa );
			RETURN( tdfa );
		}
	}

	MSG( "No DFA matches!" );
	RETURN( (pregex_dfa*)NULL );
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_symbol_to_nfa()

	Author:			Jan Max Meyer

	Usage:			Convers a symbol's regular expression pattern defininition
					into a NFA state machine.

	Parameters:		PARSER*		parser				Pointer to parser
													information structure.
					pregex_nfa*	nfa					Pointer to NFA structure,
													that is extended by this
													function.
					SYMBOL*		sym					Symbol which is associated
													with this NFA.

	Returns:		void

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_symbol_to_nfa( PARSER* parser, pregex_nfa* nfa, SYMBOL* sym )
{
	pregex_accept	acc;
	pregex_nfa		tmp_nfa;
	pregex_nfa_st*	nfa_ptr;
	pregex_nfa_st*	nfa_cpy;
	LIST*			l;

	PROC( "p_symbol_to_nfa" );
	PARMS( "parser", "%p", parser );
	PARMS( "nfa", "%p", nfa );
	PARMS( "sym", "%p", sym );

	/*
	if( !( sym->type == SYM_REGEX_TERMINAL ) )
	{
		MSG( "Symbol is not a regular expression" );
		VOIDRET;
	}

	TODO: Maybe later, check terminal types here according to config
	*/

	if( sym->ptn )
	{
		pregex_accept_init( &acc );
		acc.accept = sym->id;

		pregex_ptn_to_nfa( nfa, sym->ptn, &acc );
	}

	VOIDRET;
}

