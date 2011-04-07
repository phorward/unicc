/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator 
Copyright (C) 2006-2009 by Phorward Software Technologies, Jan Max Meyer
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
					
	Parameters:		PARSER*		parser			Pointer to the parser information
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
			p_error( (sym->type == SYM_NON_TERMINAL ) ?
				ERR_UNDEFINED_NONTERM : ERR_UNDEFINED_TERM, ERRSTYLE_FATAL,
					sym->name );
		}
		
		if( sym->generated == FALSE && sym->used == FALSE )
		{			
			p_error( (sym->type == SYM_NON_TERMINAL ) ?
				ERR_UNUSED_NONTERM : ERR_UNUSED_TERM, ERRSTYLE_WARNING,
					sym->name );
		}
	}
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_try_to_parse()
	
	Author:			Jan Max Meyer
	
	Usage:			Tries to parse a string from a given state; This function
					is internally used by p_keyword_anomalies() to find out
					if a keyword can even be recognized by a grammar construct
					of the language.
					
	Parameters:		PARSER*		parser			Pointer to the parser information
												structure.
					uchar*		str				String to be recognized (the key-
												word)
					int			start			The start state; This is the state
												where parsing is immediatelly stated.
	
	Returns:		TRUE if the parse succeeded (when str was completely absorbed),
					FALSE else.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	06.03.2008	Jan Max Meyer	Changes on the algorithm because of new
								SHIFT_REDUCE-transitions. They are either
								reduces.
----------------------------------------------------------------------------- */
BOOLEAN p_try_to_parse( PARSER* parser, uchar* str, int start )
{
	int stack[ 1024 ];
	uchar sym;
	int act;
	int idx;
	int error;
	int tos = 0;
	PROD* rprod;
	STATE* st;
	TABCOL* col;
	LIST* l;
	bitset map;

	error = list_count( parser->productions ) * -1;
	
	stack[ tos ] = start;
	sym = *(str++);
	
	/*
	fprintf( stderr, "STATE %d\n", start );
	*/
	do
	{
		act = 0;
		idx = 0;
		st = list_getptr( parser->lalr_states, stack[ tos ] );

		for( l = st->actions; l; l = l->next )
		{
			col = (TABCOL*)l->pptr;
			if( !( col->symbol->keyword ) )
			{
				map = p_ccl_to_map( parser, col->symbol->name );

				/* p_dump_map( stderr, map, parser->p_universe ); */
				if( bitset_get( map, (int)sym ) )
				{
					act = col->action;
					idx = col->index;
				}

				p_free( map );

				if( act != 0 )
					break;
			}
		}
		/*
		fprintf( stderr, "state = %d, sym = >%c< act = %d idx = %d\n",
			st->state_id, sym, act, idx );
		*/

		/* Error */
		if( act == 0 )
			return FALSE;

		/* Shift */		
		if( act & SHIFT )
		{
			tos++;
			stack[ tos ] = idx;

			sym = *(str++);
		}

		/* Reduce */
		while( act & REDUCE )
		{
			if( tos == 0 )
				return FALSE;

			rprod = (PROD*)list_getptr( parser->productions, idx );
			
			/*
			fprintf( stderr, "tos = %d, reducing production %d, %d\n", tos, idx, list_count( rprod->rhs ) );
			*/
			tos -= list_count( rprod->rhs );
			tos++;
			
			/*
			fprintf( stderr, "tos = %d\n", tos );
			*/

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
	while( sym );

	return TRUE;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_keyword_anomalies()
	
	Author:			Jan Max Meyer
	
	Usage:			Checks for keyword anomalies in the resulting parse tables.
					Keyword anomalies occur, when the parser can reduce both
					by a keyword and by a character, so it decides for the key-
					word. Under some circumstances, the token to be shifted is
					NOT the keyword it was reduced for, and the parse fails.

					A demonstation grammar for a keyword anomaly is the following:

					start -> a;
					a -> b "PRINT";
					b -> c | '[' b ']';
					c -> 'A-Z' | c 'A-Z';


					and even this one

					language$ 	->	p;
					p ->	ident "PRINT" ident;
					ident -> 'A-Z' ident
							| 'A-Z';


					At the input "[HALLOPRINT]", which is valid, the parser
					will fail after successfully parsing "[HALLO", expecting a 
					"]".
					
	Parameters:		PARSER*		parser			Pointer to the parser information
												structure.
	
	Returns:		BOOLEAN						TRUE if keyword anomalies where
												found, FALSE if not.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	06.03.2008	Jan Max Meyer	Changes on the algorithm because of new
								SHIFT_REDUCE-transitions. They are either
								reduces.
----------------------------------------------------------------------------- */
BOOLEAN p_keyword_anomalies( PARSER* parser )
{
	STATE*		st;
	LIST*		l;
	LIST*		m;
	LIST*		n;
	TABCOL*		col;
	TABCOL*		ccol;
	bitset		test;
	int			cnt;

	/*
		For every keyword, try to find a character class beginning with the same
		character as the keyword. Then, try to recognize the keyword beginning from
		the state that forms the keyword, by running the parser on its existing
		tables.
	*/
	for( l = parser->lalr_states; l; l = l->next )
	{
		st = (STATE*)l->pptr;

		/* First of all, count all possible reduces in the current state. */
		for( m = st->actions, cnt = 0; m; m = m->next )
		{
			col = (TABCOL*)m->pptr;
			if( col->action == REDUCE )
				cnt++;
		}

		for( m = st->actions; m; m = m->next )
		{
			col = (TABCOL*)m->pptr;

			/* Keyword to be reduced? */
			if( col->symbol->type == SYM_KW_TERMINAL && col->action == REDUCE )
			{
				/*
					p_try_to_parse() can either be called here; But to be
					sure, we have a try if there are shifts on the same
					character, and then we try to parse the keyword using
					the existing parse tables. This will even be more
					faster (I think ;)).
				*/
				for( n = st->actions; n; n = n->next )
				{
					ccol = (TABCOL*)n->pptr;

					/* Character class to be shifted? */
					if( ccol->symbol->type == SYM_CCL_TERMINAL
							&& ccol->action & SHIFT )
					{
						test = p_ccl_to_map( parser, ccol->symbol->name );

						/*
							If this is a match with the grammar and the keyword,
							a keyword anomaly exists between the shift by a 
							character and the reduce by a keyword which can be
							derived from the build-up of the characters of the
							keyword. This is not the problem if there is only
							one reduce, but if there are more, output a warning!
						*/
						if( bitset_get( test, *( col->symbol->name ) ) )
						{
							if( p_try_to_parse( parser, col->symbol->name,
									st->state_id ) && cnt > 1 )
							{
									p_error( ERR_KEYWORD_ANOMALY,
										ERRSTYLE_WARNING | ERRSTYLE_STATEINFO,
											st, ccol->symbol->name, col->symbol->name );
							}
						}

						p_free( test );
					}
				}
			}
		}
	}

	return FALSE;
}


/* -FUNCTION--------------------------------------------------------------------
	Function:		p_stupid_productions()
	
	Author:			Jan Max Meyer
	
	Usage:			Checks for productions resulting in circular definitions,
					empty recursions or with wrong FIRST-sets.
					
	Parameters:		PARSER*		parser			Pointer to the parser information
												structure.
	
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
			p_error( ERR_CIRCULAR_DEFINITION, ERRSTYLE_WARNING | ERRSTYLE_PRODUCTION, p );
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
				p_error( ERR_EMPTY_RECURSION, ERRSTYLE_WARNING | ERRSTYLE_PRODUCTION, p );
				stupid = TRUE;
			}
		}

		/* Get all FIRST-sets of the right-hand side; If there are none,
			this can't be possible */
		if( p->rhs )
		{
			p_rhs_first( &first_check, p->rhs );
			if( list_count( first_check ) == 0 )
				p_error( ERR_USELESS_RULE, ERRSTYLE_WARNING | ERRSTYLE_PRODUCTION, p );

			first_check = list_free( first_check );
		}
	}

	return stupid;
}

