/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2014 by Phorward Software Technologies, Jan Max Meyer
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
	pregex_nfa*	nfa;
	pregex_dfa*	dfa;
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
		nfa = pregex_nfa_create();
		dfa = pregex_dfa_create();

		/* Construct NFAs from keywords */
		LISTFOR( s->actions, m )
		{
			col = (TABCOL*)list_access( m );
			p_symbol_to_nfa( parser, nfa, col->symbol );
		}

		/* Construct DFA, if NFA has been constructed */
		VARS( "plist_count( nfa->states )", "%d", plist_count( nfa->states ) );
		if( plist_count( nfa->states ) )
		{
			MSG( "Constructing DFA from NFA" );
			if( pregex_dfa_from_nfa( dfa, nfa ) < ERR_OK )
				OUTOFMEM;

			VARS( "plist_count( dfa->states )", "%d",
					plist_count( dfa->states ) );

			MSG( "Freeing NFA" );
			nfa = pregex_nfa_free( nfa );

			MSG( "Minimizing DFA" );
			if( pregex_dfa_minimize( dfa ) < ERR_OK )
				OUTOFMEM;

			VARS( "plist_count( dfa->states )", "%d",
					plist_count( dfa->states ) );

			if( ( ex_dfa = p_find_equal_dfa( parser, dfa ) ) )
			{
				MSG( "An equal DFA exists; Freeing temporary one!" );
				dfa = pregex_dfa_free( dfa );
			}
			else
			{
				MSG( "This DFA does not exist in pool yet - integrating!" );
				ex_dfa = dfa;

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
	pregex_nfa*			nfa;
	pregex_dfa*			dfa;
	LIST*				l;
	SYMBOL*				s;

	PROC( "p_single_lexer" );
	PARMS( "parser", "%p", parser );

	MSG( "Constructing NFA" );
	nfa = pregex_nfa_create();
	dfa = pregex_dfa_create();

	LISTFOR( parser->symbols, l )
	{
		s = (SYMBOL*)list_access( l );
		VARS( "s->id", "%d", s->id );

		p_symbol_to_nfa( parser, nfa, s );
	}

	/* Construct DFA, if NFA has been constructed */
	VARS( "plist_count( nfa->states )", "%d", plist_count( nfa->states ) );
	if( plist_count( nfa->states ) )
	{
		MSG( "Constructing DFA from NFA" );
		if( pregex_dfa_from_nfa( dfa, nfa ) < ERR_OK )
			OUTOFMEM;

		VARS( "plist_count( dfa->states )", "%d",
				plist_count( dfa->states ) );

		MSG( "Freeing NFA" );
		nfa = pregex_nfa_free( nfa );

		MSG( "Minimizing DFA" );
		if( pregex_dfa_minimize( dfa ) < ERR_OK )
			OUTOFMEM;

		VARS( "plist_count( dfa->states )", "%d",
				plist_count( dfa->states ) );

		if( !( parser->kw = list_push( parser->kw, (void*)dfa ) ) )
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
	16.01.2014	Jan Max Meyer	Fixed sources to run with libphorward v0.18
								(current development version).
----------------------------------------------------------------------------- */
pregex_dfa* p_find_equal_dfa( PARSER* parser, pregex_dfa* ndfa )
{
	LIST*			l;
	plistel*		e;
	plistel*		f;
	plistel*		g;
	plistel*		h;

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

		VARS( "plist_count( tdfa->states )", "%d", plist_count( tdfa->states ) );
		VARS( "plist_count( ndfa->states )", "%d", plist_count( ndfa->states ) );
		if( plist_count( tdfa->states ) != plist_count( ndfa->states ) )
		{
			MSG( "Number of states does already not match - test next" );
			continue;
		}

		for( e = plist_first( tdfa->states ),
				f = plist_first( ndfa->states ), match = TRUE;
					e && f && match; e = plist_next( e ), f = plist_next( f ) )
		{
			dfa_st[0] = (pregex_dfa_st*)plist_access( e );
			dfa_st[1] = (pregex_dfa_st*)plist_access( f );

			if( !( dfa_st[0]->accept.accept == dfa_st[1]->accept.accept
					&& plist_count( dfa_st[0]->trans )
							== plist_count( dfa_st[1]->trans ) ) )
			{
				MSG( "Number of transitions or accepting ID does not match" );
				match = FALSE;
				break;
			}

			for( g = plist_first( dfa_st[0]->trans ),
					h = plist_first( dfa_st[1]->trans ); g && h;
						g = plist_next( g ), h = plist_next( h ) )
			{
				dfa_ent[0] = (pregex_dfa_tr*)plist_access( g );
				dfa_ent[1] = (pregex_dfa_tr*)plist_access( h );

				if( !( pregex_ccl_compare( dfa_ent[0]->ccl, dfa_ent[1]->ccl )
							== 0
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
		if( !sym->ptn->accept )
			sym->ptn->accept = pmalloc( sizeof( pregex_accept ) );

		pregex_accept_init( sym->ptn->accept );
		sym->ptn->accept->accept = sym->id;

		pregex_ptn_to_nfa( nfa, sym->ptn );
	}

	VOIDRET;
}

