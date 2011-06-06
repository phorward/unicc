/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator 
Copyright (C) 2006-2011 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com/ ++ unicc<<AT>>phorward-software<<DOT>>com

File:	p_integrity.c
Author:	Jan Max Meyer
Usage:	Grammar integrity checking functions

You may use, modify and distribute this software under the terms and conditions
of the Artistic License, version 2. Please see LICENSE for more information.
----------------------------------------------------------------------------- */

/*
 * Includes
 */
#include "p_global.h"
#include "p_proto.h"
#include "p_error.h"

/*
 * Global variables
 */

/*
 * Functions
 */

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_undef_or_unused()
	
	Author:			Jan Max Meyer
	
	Usage:			Checks for undefined but called, and/or defined but
					uncalled symbols.
					
	Parameters:		PARSER*		parser		Pointer to the parser information
											structure.
	
	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_undef_or_unused( PARSER* parser )
{
	LIST*	l;
	SYMBOL*	sym;

	for( l = parser->symbols; l; l = l->next )
	{
		sym = l->pptr;
		if( sym->generated == FALSE && sym->defined == FALSE )
		{
			p_error( parser, (sym->type == SYM_NON_TERMINAL ) ?
				ERR_UNDEFINED_NONTERM : ERR_UNDEFINED_TERM,
					ERRSTYLE_FATAL | ERRSTYLE_FILEINFO,
						parser->filename, sym->line, sym->name );
		}
		
		if( sym->generated == FALSE && sym->used == FALSE )
		{			
			p_error( parser, (sym->type == SYM_NON_TERMINAL ) ?
				ERR_UNUSED_NONTERM : ERR_UNUSED_TERM,
					ERRSTYLE_WARNING | ERRSTYLE_FILEINFO,
						parser->filename, sym->line, sym->name );
		}
	}
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_nfa_transition_on_ccl()
	
	Author:			Jan Max Meyer
	
	Usage:			This function is required to test if a given character
					class will be consumed by a NFA machine state.
					
	Parameters:		pregex_nfa*		nfa				The NFA to work on.
					LIST*			res				Defines the closure set
													of NFA states required 
													for the next transition.
													(LIST*)NULL causes the
													NFA to be startet from
													initial state.
					int*			accept			Return pointer for the
													accepting ID of the
													NFA.
					CCL				check_with		Character-class to test
													transitions on.
	
	Returns:		LIST*							The result of the
													transition on the given
													character class, (LIST*)NULL
													if there is no transition.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
static LIST* p_nfa_transition_on_ccl(
	pregex_nfa* nfa, LIST* res, int* accept, CCL check_with )
{
	CCL		i;
	pchar	ch;
	LIST*	tr;
	LIST*	ret_res	= (LIST*)NULL;

	if( !res )
		res = list_push( (LIST*)NULL, list_access( nfa->states ) );
	
	res = pregex_nfa_epsilon_closure( nfa, res, accept, (int*)NULL );

	for( i = check_with; !ccl_end( i ); i++ )
	{
		/*
			TODO:
			This may be the source for large run time latency.
			The way chosen by pregex/dfa.c should be used if necessary
			somewhere in future.
		*/
		for( ch = i->begin; ch <= i->end; ch++ )
		{
			tr = list_dup( res );
			if( ( tr = pregex_nfa_move( nfa, tr, ch, ch ) ) )
				ret_res = list_union( ret_res, tr );

			list_free( tr );
		}
	}

	list_free( res );
	return ret_res;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_nfa_matches_parser()
	
	Author:			Jan Max Meyer
	
	Usage:			Tries to parse along an NFA state machine using character
					class terminals from a given LALR(1) state. It should prove
					if the grammar is capable to match the string the regular
					expression matches. It is used for the regex anomaly
					detection.
	
	Parameters:		PARSER*		parser		Pointer to the parser information
											structure.
					uchar*		str			String to be recognized (the key-
											word)
					int			start		The start state; This is the state
											where parsing is immediatelly
											stated.
	
	Returns:		TRUE if the parse succeeded (when str was completely
					absorbed), FALSE else.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	06.03.2008	Jan Max Meyer	Changes on the algorithm because of new
								SHIFT_REDUCE-transitions. They are either
								reduces.
	30.01.2011	Jan Max Meyer	Renamed to p_nfa_matches_parser(), the
								function now tries to parse along a NFA
								state machine which must not have its
								origin in a keyword terminal.
----------------------------------------------------------------------------- */
static BOOLEAN p_nfa_matches_parser(
	PARSER* parser, pregex_nfa* nfa, LIST* start_res, int start )
{
	int		stack[ 1024 ];
	int		act;
	int		accept;
	int		idx;
	int		error;
	int		tos 			= 0;
	PROD*	rprod;
	STATE*	st;
	TABCOL*	col;
	LIST*	l;
	LIST*	res;

	/*
		TODO:
		This part of UniCC is programmed very rude, should be
		changed somewhere in the future. But for now, it does
		its job.
	*/
	error = list_count( parser->productions ) * -1;
	
	if( ( st = list_getptr( parser->lalr_states, start ) ) )
		stack[ tos++ ] = st->derived_from->state_id;
	else
		stack[ tos++ ] = start;
	
	stack[ tos ] = start;

	do
	{
		act = 0;
		idx = 0;
		st = list_getptr( parser->lalr_states, stack[ tos ] );

		for( l = st->actions; l; l = l->next )
		{
			col = (TABCOL*)l->pptr;
			if( col->symbol->type == SYM_CCL_TERMINAL )
			{
				res = list_dup( start_res );

				if( ( res = p_nfa_transition_on_ccl( nfa, res,
						&accept, col->symbol->ccl ) ) )
				{
					act = col->action;
					idx = col->index;
				}

				if( act || accept != REGEX_ACCEPT_NONE )
					break;

				list_free( res );
			}
		}
		/*
		fprintf( stderr, "state = %d, act = %d idx = %d accept = %d res = %p\n",
			st->state_id, act, idx, accept, res );
		*/

		list_free( start_res );
		start_res = res;

		if( accept != REGEX_ACCEPT_NONE )
			break;

		/* Error */
		if( act == ERROR )
			return FALSE;

		/* Shift */		
		if( act & SHIFT )
			stack[ ++tos ] = idx;

		/* Reduce */
		while( act & REDUCE )
		{
			rprod = (PROD*)list_getptr( parser->productions, idx );
			/*
			fprintf( stderr, "tos = %d, reducing production %d, %d\n",
				tos, idx, list_count( rprod->rhs ) );
			p_dump_production( stderr, rprod, FALSE, FALSE );
			*/

			tos -= list_count( rprod->rhs );

			/*
			fprintf( stderr, "tos %d\n", tos );
			*/
			tos++;
			
			st = list_getptr( parser->lalr_states, stack[ tos - 1 ] );

			for( l = st->gotos; l; l = l->next )
			{
				col = (TABCOL*)l->pptr;

				if( col->symbol == rprod->lhs )
				{
					act = col->action;
					stack[ tos ] = idx = col->index;
					break;
				}
			}

			if( stack[ tos ] == -1 )
				return FALSE;
		}
	}
	while( accept == REGEX_ACCEPT_NONE );

	list_free( res );
	return TRUE;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_regex_anomalies()
	
	Author:			Jan Max Meyer
	
	Usage:			Checks for regex anomalies in the resulting parse tables.
					Such regex anomalies occur, when the parser can reduce both
					by a regular expression based terminal and by a character,
					so it decides for the regular expression.
					
					Under some circumstances, the token to be shifted is
					NOT the regex it was reduced for, and the parse fails.

					A demonstation grammar for a keyword anomaly is the
					following:

					start$ -> a;
					a -> b "PRINT";
					b -> c | '[' b ']';
					c -> 'A-Z' | c 'A-Z';

					At the input "[HALLOPRINT]", which is valid, the parser
					will fail after successfully parsing "[HALLO", expecting a 
					"]".
					
	Parameters:		PARSER*		parser		Pointer to the parser information
											structure.
	
	Returns:		BOOLEAN					TRUE if regex anomalies where
											found, FALSE if not.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	06.03.2008	Jan Max Meyer	Changes on the algorithm because of new
								SHIFT_REDUCE-transitions. They are either
								reduces.
	12.06.2010	Jan Max Meyer	Changed to new regular expression library
								functions, which can handle entire sets of
								characters from 0x0 - 0xFFFF.
	31.01.2011	Jan Max Meyer	Renamed the function to p_regex_anomalies()
								because not only keywords are tested now, even
								entire regular expressions.
----------------------------------------------------------------------------- */
BOOLEAN p_regex_anomalies( PARSER* parser )
{
	STATE*		st;
	LIST*		l;
	LIST*		m;
	LIST*		n;
	LIST*		o;
	LIST*		q;
	PROD*		p;
	SYMBOL*		lhs;
	SYMBOL*		sym;
	TABCOL*		col;
	TABCOL*		ccol;
	int			cnt;
	BOOLEAN		found;

	LIST*		res			= (LIST*)NULL;
	pregex_nfa*	nfa;
	int			accept;

	/*
		For every keyword, try to find a character class beginning with the
		same character as the keyword. Then, try to recognize the keyword
		beginning from the state that forms the keyword, by running the
		parser on its existing tables.
	*/
	for( l = parser->lalr_states; l; l = l->next )
	{
		st = (STATE*)l->pptr;

		/* First of all, count all possible reduces in the current state. */
		for( m = st->actions, cnt = 0; m; m = m->next )
		{
			col = (TABCOL*)m->pptr;
			if( col->action & REDUCE )
				cnt++;
		}

		for( m = st->actions; m; m = m->next )
		{
			col = (TABCOL*)m->pptr;

			/* Regular expression to be reduced? */
			if( col->symbol->type == SYM_REGEX_TERMINAL
					&& col->action & REDUCE )
			{
				/*
					Table columns not derived from the kernel set
					of the state are ignored
				*/
				if( col->derived_from && list_find( st->epsilon,
						col->derived_from ) >= 0 )
					continue;

				nfa = &( col->symbol->nfa );

				/*
					p_nfa_matches_parser() can either be called here;
					But to be sure, we have a try if there are shifts on
					the same character, and then we try to parse the using
					the existing parse tables. This will even be more
					faster, I think.
				*/
				for( n = st->actions; n; n = n->next )
				{
					ccol = (TABCOL*)n->pptr;

					/* Character class to be shifted? */
					if( ccol->symbol->type == SYM_CCL_TERMINAL
							&& ccol->action & SHIFT )
					{
						/*
							If this is a match with the grammar and the keyword,
							a keyword anomaly exists between the shift by a 
							character and the reduce by a keyword which can be
							derived from the build-up of the characters of the
							keyword. This is not the problem if there is only
							one reduce, but if there are more, output a warning!
						*/
						if( ( res = p_nfa_transition_on_ccl(
									nfa, (LIST*)NULL, &accept,
										ccol->symbol->ccl ) ) )
						{
							/*
							printf( "state %d\n", st->state_id );
							p_dump_item_set( stderr, (char*)NULL, st->kernel );
							p_dump_item_set( stderr, (char*)NULL, st->epsilon );
							getchar();
							*/
							if( p_nfa_matches_parser( parser, nfa, res,
									st->state_id ) && cnt > 1 )
							{
								/*
									At this point, we have a candidate for a
									regex anomaly.. now we check out all
									positions where the left-hand side of the
									reduced production appears in...
									if there is no keyword to shift in the
									FIRST-sets of following symbols, report
									this anomaly!
								*/								
								p = (PROD*)list_getptr(
										parser->productions, col->index );
								lhs = p->lhs;
								
								/* Go trough all productions */
								for( o = parser->productions, found = FALSE;
										o && !found; o = list_next( o ) )
								{
									p = (PROD*)list_access( o );
									for( q = p->rhs; q && !found;
										q = list_next( q ) )
									{
										sym = (SYMBOL*)list_access( q );
										if( sym == lhs && list_next( q ) )
										{
											do
											{
												q = list_next( q );
												if( !( sym = (SYMBOL*)
														list_access( q ) ) )
													break;
												/*
												fprintf( stderr, "sym = " );
												p_print_symbol( stderr, sym );
												fprintf( stderr, " %d %d\n",
													list_find( sym->first,
														col->symbol ),
															sym->nullable );
												*/
												if( list_find( sym->first,
														col->symbol ) == -1 
													&& !sym->nullable )
												{
													p_error( parser,
														ERR_KEYWORD_ANOMALY,
														ERRSTYLE_WARNING |
														ERRSTYLE_STATEINFO,
														st, ccol->symbol->name,
															col->symbol->name );
													
													found = TRUE;
													break;
												}
											}
											while( sym && sym->nullable );
										}
									}									
								}
							}
						}
					}
				}
			}
		}
	} /* This is stupid... */

	return FALSE;
}


/* -FUNCTION--------------------------------------------------------------------
	Function:		p_stupid_productions()
	
	Author:			Jan Max Meyer
	
	Usage:			Checks for productions resulting in circular definitions,
					empty recursions or with wrong FIRST-sets.
					
	Parameters:		PARSER*		parser			Pointer to the parser
												information structure.
	
	Returns:		BOOLEAN		TRUE			In case if stupid productions
												where detected,
								FALSE			if all is fine :D
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
BOOLEAN p_stupid_productions( PARSER* parser )
{
	LIST*	l;
	LIST*	m;
	PROD*	p;
	SYMBOL*	sym;
	BOOLEAN	stupid		= FALSE;
	BOOLEAN	possible	= FALSE;
	LIST*	first_check	= (LIST*)NULL;

	for( l = parser->productions; l; l = l->next )
	{
		p = (PROD*)( l->pptr );

		if( list_count( p->rhs ) == 1 &&
				(SYMBOL*)( p->rhs->pptr ) == p->lhs )
		{
			p_error( parser, ERR_CIRCULAR_DEFINITION,
				ERRSTYLE_WARNING | ERRSTYLE_PRODUCTION | ERRSTYLE_FILEINFO,
					parser->filename, p->line, p );
			stupid = TRUE;
		}
		else if( p->lhs->nullable )
		{
			for( m = p->rhs, possible = FALSE; m; m = m->next )
			{
				sym = (SYMBOL*)( m->pptr );

				if( !( sym->nullable ) )
				{
					possible = FALSE;
					break;
				}
				else if( sym == p->lhs )
				{
					possible = TRUE;
				}
			}

			if( possible )
			{
				p_error( parser, ERR_EMPTY_RECURSION,
					ERRSTYLE_WARNING | ERRSTYLE_FILEINFO | ERRSTYLE_PRODUCTION,
						parser->filename, p->line, p );
				stupid = TRUE;
			}
		}

		/* Get all FIRST-sets of the right-hand side; If there are none,
			this can't be possible */
		if( p->rhs )
		{
			p_rhs_first( &first_check, p->rhs );
			if( list_count( first_check ) == 0 )
				p_error( parser, ERR_USELESS_RULE,
					ERRSTYLE_WARNING | ERRSTYLE_PRODUCTION | ERRSTYLE_FILEINFO,
						parser->filename, p->line, p );

			first_check = list_free( first_check );
		}
	}

	return stupid;
}

