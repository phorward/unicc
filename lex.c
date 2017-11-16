/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2017 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com ++ unicc<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	lex.c
Author:	Jan Max Meyer
Usage:	Turns regular expression definitions into deterministic state
		machines, by using the libphorward regular expression tools.
----------------------------------------------------------------------------- */

#include "unicc.h"

/** Converts the terminal symbols within the states into a DFA, and maybe
re-uses state machines matching the same pool of terminals.

//parser// is the pointer to parser information structure. */
void merge_symbols_to_dfa( PARSER* parser )
{
	pregex_nfa*	nfa;
	pregex_dfa*	dfa;
	pregex_dfa*	ex_dfa;
	LIST*	l;
	LIST*	m;
	STATE*	s;
	TABCOL*	col;

	PROC( "merge_symbols_to_dfa" );
	PARMS( "parser", "%p", parser );

	LISTFOR( parser->lalr_states, l )
	{
		s = (STATE*)list_access( l );

		VARS( "s->state_id", "%d", s->state_id );
		nfa = pregex_nfa_create();

		/* Construct NFAs from symbols */
		LISTFOR( s->actions, m )
		{
			col = (TABCOL*)list_access( m );
			nfa_from_symbol( parser, nfa, col->symbol );
		}

		/* Construct DFA, if NFA has been constructed */
		VARS( "plist_count( nfa->states )", "%d", plist_count( nfa->states ) );
		if( plist_count( nfa->states ) )
		{
			dfa = pregex_dfa_create();

			MSG( "Constructing DFA from NFA" );
			if( !pregex_dfa_from_nfa( dfa, nfa ) )
				OUTOFMEM;

			VARS( "plist_count( dfa->states )", "%d",
					plist_count( dfa->states ) );

			MSG( "Freeing NFA" );
			nfa = pregex_nfa_free( nfa );

			MSG( "Minimizing DFA" );
			if( !pregex_dfa_minimize( dfa ) )
				OUTOFMEM;

			VARS( "plist_count( dfa->states )", "%d",
					plist_count( dfa->states ) );

			if( ( ex_dfa = find_equal_dfa( parser, dfa ) ) )
			{
				MSG( "An equal DFA exists; Freeing temporary one!" );
				dfa = pregex_dfa_free( dfa );
			}
			else
			{
				MSG( "This DFA does not exist in pool yet - integrating!" );
				ex_dfa = dfa;

				if( !( parser->dfas = list_push(
						parser->dfas, (void*)ex_dfa ) ) )
					OUTOFMEM;
			}

			VARS( "ex_dfa", "%p", ex_dfa );
			s->dfa = ex_dfa;
		}
		else
			pregex_nfa_free( nfa );
	}

	VOIDRET;
}

/** Constructs a single DFA for a general token lexer.

//parser// is the pointer to parser information structure. */
void construct_single_lexer( PARSER* parser )
{
	pregex_nfa*			nfa;
	pregex_dfa*			dfa;
	plistel*			e;
	SYMBOL*				s;

	PROC( "construct_single_lexer" );
	PARMS( "parser", "%p", parser );

	MSG( "Constructing NFA" );
	nfa = pregex_nfa_create();
	dfa = pregex_dfa_create();

	plist_for( parser->symbols, e )
	{
		s = (SYMBOL*)plist_access( e );
		VARS( "s->id", "%d", s->id );

		nfa_from_symbol( parser, nfa, s );
	}

	/* Construct DFA, if NFA has been constructed */
	VARS( "plist_count( nfa->states )", "%d", plist_count( nfa->states ) );
	if( plist_count( nfa->states ) )
	{
		MSG( "Constructing DFA from NFA" );
		if( !pregex_dfa_from_nfa( dfa, nfa ) )
			OUTOFMEM;

		VARS( "plist_count( dfa->states )", "%d",
				plist_count( dfa->states ) );

		MSG( "Freeing NFA" );
		nfa = pregex_nfa_free( nfa );

		MSG( "Minimizing DFA" );
		if( !pregex_dfa_minimize( dfa ) )
			OUTOFMEM;

		VARS( "plist_count( dfa->states )", "%d",
				plist_count( dfa->states ) );

		if( !( parser->dfas = list_push( parser->dfas, (void*)dfa ) ) )
			OUTOFMEM;
	}

	VOIDRET;
}

/** Walks trough the DFA machines of the current parser definition and tests if
the temporary generated DFA contains the same states than a one already defined
in the parser.

//parser// is the parser information structure.
//ndfa// is the pointer to DFA that is compared with the other machine already
integrated into the parser structure.

Returns the pointer to a matching DFA, else (pregex_dfa*)NULL.
*/
pregex_dfa* find_equal_dfa( PARSER* parser, pregex_dfa* ndfa )
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

	/*
	19.11.2009	Jan Max Meyer
	Revision of entire function, to work with structures of the new
	regex-library.

	16.01.2014	Jan Max Meyer
	Fixed sources to run with libphorward v0.18 (current development version).
	*/

	PROC( "find_equal_dfa" );
	PARMS( "parser", "%p", parser );
	PARMS( "ndfa", "%p", ndfa );

	LISTFOR( parser->dfas, l )
	{
		tdfa = (pregex_dfa*)list_access( l );

		VARS( "plist_count( tdfa->states )", "%d",
				plist_count( tdfa->states ) );
		VARS( "plist_count( ndfa->states )", "%d",
				plist_count( ndfa->states ) );

		if( plist_count( tdfa->states ) != plist_count( ndfa->states ) )
		{
			MSG( "Number of states does already not match - test next" );
			continue;
		}

		for( e = plist_first( tdfa->states ),
				f = plist_first( ndfa->states );
					e && f; e = plist_next( e ), f = plist_next( f ) )
		{
			match = TRUE;

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

				if( !( p_ccl_compare( dfa_ent[0]->ccl, dfa_ent[1]->ccl )
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

/** Converts a symbols regular expression pattern defininition into a
NFA state machine.

//parser// is the pointer to parser information structure.
//nfa// is the pointer to NFA structure, that is extended by this function.
//sym// is the symbol which is associated with this NFA.
*/
void nfa_from_symbol( PARSER* parser, pregex_nfa* nfa, SYMBOL* sym )
{
	PROC( "nfa_from_symbol" );
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

		sym->ptn->accept->accept = sym->id + 1;
		pregex_ptn_to_nfa( nfa, sym->ptn );
	}

	VOIDRET;
}

