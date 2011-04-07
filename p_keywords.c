/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator 
Copyright (C) 2006-2009 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com/ ++ unicc<<AT>>phorward-software<<DOT>>com

File:	p_keywords.c
Author:	Jan Max Meyer
Usage:	Turns keyword definitions into deterministic state machines, by
		using the regular expression library.
		
You may use, modify and distribute this software under the terms and conditions
of the Artistic License, version 2. Please see LICENSE for more information.
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
	Function:		p_keywords_to_dfa()
	
	Author:			Jan Max Meyer
	
	Usage:			Converts the keywords within the states into a DFA, and maybe
					re-uses state machines matching the same pool of keywords.
					
	Parameters:		<type>		<identifier>		<description>
	
	Returns:		<type>							<description>
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_keywords_to_dfa( PARSER* parser )
{
	LIST*	nfa;
	LIST*	dfa;
	LIST*	dfa_tmp;
	LIST*	l;
	LIST*	m;
	STATE*	s;
	TABCOL*	col;

	for( l = parser->lalr_states; l; l = l->next )
	{
		s = (STATE*)(l->pptr);

		nfa = (LIST*)NULL;

		/* Construct NFAs from keywords */
		for( m = s->actions; m; m = m->next )
		{
			col = (TABCOL*)(m->pptr);
			nfa = p_symbol_to_nfa( parser, nfa, col->symbol );
		}

		/* re_dbg_print_nfa( nfa, parser->p_universe ); */

		/* Construct DFA, if NFA has been constructed */
		if( nfa )
		{
			if( !( dfa_tmp = re_build_dfa( nfa, parser->p_universe ) ) )
			{
				re_free_nfa_table( nfa );
				OUT_OF_MEMORY;
			}

			if( !( dfa_tmp = re_dfa_minimize( dfa_tmp, parser->p_universe ) ) )
			{
				re_free_dfa( &dfa_tmp );
				OUT_OF_MEMORY;
			}

			if( ( dfa = p_find_equal_dfa( parser, dfa_tmp ) ) )
			{
				re_free_dfa( &dfa_tmp );
				/* printf( "Re-using DFA...\n" ); */
			}
			else
			{
				dfa = dfa_tmp;
				parser->kw = list_push( parser->kw, dfa );				
				/* printf( "Creating new DFA...\n" ); */
			}

			s->dfa = dfa;			
		}
	}
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_single_lexer()
	
	Author:			Jan Max Meyer
	
	Usage:			Constructs a single DFA for a general token lexer.
					
	Parameters:		<type>		<identifier>		<description>
	
	Returns:		<type>							<description>
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_single_lexer( PARSER* parser )
{
	LIST*	nfa			= (LIST*)NULL;
	LIST*	dfa_tmp;
	LIST*	l;
	SYMBOL*	s;

	for( l = parser->symbols; l; l = l->next )
	{
		s = (SYMBOL*)(l->pptr);

		if( s->type == SYM_NON_TERMINAL )
			continue;

		nfa = p_symbol_to_nfa( parser, nfa, s );
	}

	/* Construct DFA, if NFA has been constructed */
	if( nfa )
	{
		if( !( dfa_tmp = re_build_dfa( nfa, parser->p_universe ) ) )
		{
			re_free_nfa_table( nfa );
			OUT_OF_MEMORY;
		}

		if( !( dfa_tmp = re_dfa_minimize( dfa_tmp, parser->p_universe ) ) )
		{
			re_free_dfa( &dfa_tmp );
			OUT_OF_MEMORY;
		}

		parser->kw = list_push( parser->kw, dfa_tmp ) ;
	}
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_find_equal_dfa()
	
	Author:			Jan Max Meyer
	
	Usage:			Walks trough the DFA machines of the current parser definition
					and tests if the temporary generated DFA contains the same
					states than a one already defined in the parser.
					
	Parameters:		<type>		<identifier>		<description>
	
	Returns:		<type>							<description>
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
LIST* p_find_equal_dfa( PARSER* parser, LIST* ndfa )
{
	LIST*	l;
	LIST*	m;
	LIST*	n;
	LIST*	tdfa		= (LIST*)NULL;
	DFA*	dfa[2];
	BOOLEAN	match;

	for( l = parser->kw; l; l = l->next )
	{
		tdfa = (LIST*)(l->pptr);

		if( list_count( tdfa ) != list_count( ndfa ) )
		{
			tdfa = (LIST*)NULL;
			continue;
		}

		match = TRUE;
		for( m = tdfa, n = ndfa; m && n; m = m->next, n = n->next )
		{
			dfa[0] = (DFA*)(m->pptr);
			dfa[1] = (DFA*)(n->pptr);

			if( !( dfa[0]->accept == dfa[1]->accept &&
					memcmp( dfa[0]->line, dfa[1]->line,
						parser->p_universe * sizeof( int ) ) == 0 ) )
			{
				match = FALSE;
				break;
			}
		}

		if( match )
			break;

		tdfa = (LIST*)NULL;
	}

	return tdfa;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_keyword_to_nfa()
	
	Author:			Jan Max Meyer
	
	Usage:			Builds a nondeterminisitic finite automation structure from
					a keyword string. This will later be turned into a DFA.
					
	Parameters:		<type>		<identifier>		<description>
	
	Returns:		<type>							<description>
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	21.08.2008	Jan Max Meyer	Small bugfix on empty keywords
----------------------------------------------------------------------------- */
void p_keyword_to_nfa( PARSER* parser, LIST** nfa, uchar* keyword, int accepting_id )
{
	NFA*	start;
	NFA*	end;
	NFA*	tmp			= (NFA*)NULL;
	NFA*	tmp_start;
	LIST*	tmp_nfa		= (LIST*)NULL;
	uchar*	kp;
	uchar	ch;

	if( !( start = re_create_nfa( &tmp_nfa ) ) )
		OUT_OF_MEMORY;

	for( kp = keyword; *kp; )
	{
		if( kp == keyword )
		{
			if( !( start->next = tmp = re_create_nfa( &tmp_nfa ) ) )
				OUT_OF_MEMORY;
		}
		else
		{
			if( !( tmp->next = re_create_nfa( &tmp_nfa ) ) )
				OUT_OF_MEMORY;

			tmp = tmp->next;
		}

		ch = p_unescape_char( kp, &kp );
		if( parser->p_cis_keywords )
		{
			tmp->edge = CCL;
			tmp->cclass = bitset_create( parser->p_universe );

			bitset_set( tmp->cclass, p_tolower( ch ), TRUE );
			bitset_set( tmp->cclass, p_toupper( ch ), TRUE );

			//p_dump_map( stdout, tmp->cclass, parser->p_universe );
		}
		else
			tmp->edge = ch;
	}

	if( !( end = re_create_nfa( &tmp_nfa ) ) )
		OUT_OF_MEMORY;

	if( tmp )
		tmp->next = end;
	else
		start->next = end;

	end->accept = accepting_id;

	if( !(*nfa) )
		*nfa = tmp_nfa;
	else
	{
		*nfa = list_union( *nfa, tmp_nfa );
		list_free( tmp_nfa );

		tmp_start = (NFA*)((*nfa)->pptr);
		while( tmp_start->next2 )
			tmp_start = tmp_start->next2;

		tmp_start->next2 = start;
	}
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_symbol_to_nfa()
	
	Author:			Jan Max Meyer
	
	Usage:			Converts a symbol - based on its type - to a nfa.
					This can only be done for keywords and for pre-compiled
					regular expression terminals.
					This function is used in several places, so it was
					established here.
					
	Parameters:		<type>		<identifier>		<description>
	
	Returns:		<type>							<description>
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
LIST* p_symbol_to_nfa( PARSER* parser, LIST* nfa, SYMBOL* sym )
{
	LIST*	tmp_nfa = (LIST*)NULL;
	NFA*	nfa_ptr;
	NFA*	nfa_copy;
	LIST*	l;

	if( sym->type == SYM_KW_TERMINAL )
		p_keyword_to_nfa( parser, &nfa, sym->name, sym->id );
	else if( sym->type == SYM_REGEX_TERMINAL && sym->nfa_def )
	{
		/* Copy the whole NFA - this does not work nice with the
			regex lib if we do it using another way :( */
		for( l = sym->nfa_def; l; l = l->next )
		{
			nfa_ptr = (NFA*)( l->pptr );
			if( !( nfa_copy = re_create_nfa( &tmp_nfa ) ) )
				OUT_OF_MEMORY;

			memcpy( nfa_copy, nfa_ptr, sizeof( NFA ) );
			if( nfa_copy->edge == CCL )
				nfa_copy->cclass = bitset_copy(
					parser->p_universe, nfa_ptr->cclass );

			/*
				Just for the protocol:
				I LIKE HACKS, BUT I WOULD OMIT... ;(
			*/

			/* Temporary replace pointers... */
			if( nfa_copy->next )
				nfa_copy->next = (NFA*)list_find( 
					sym->nfa_def, nfa_ptr->next );

			if( nfa_copy->next2 )
				nfa_copy->next2 = (NFA*)list_find(
					sym->nfa_def, nfa_ptr->next2 );
		}

		/* ...and then restore them... this product is more than just freaky...
			but its the most quick solution ... maybe to be reworked anywhere
				in the future... oh shit... */
		for( l = tmp_nfa; l; l = l->next )
		{
			nfa_ptr = (NFA*)( l->pptr );
			
			if( nfa_ptr->next )
				nfa_ptr->next = list_getptr( tmp_nfa, (int)( nfa_ptr->next ) );

			if( nfa_ptr->next2 )
				nfa_ptr->next2 = list_getptr( tmp_nfa, (int)( nfa_ptr->next2 ) );
		}
		
		if( tmp_nfa )
		{
			if( !nfa )
				nfa = tmp_nfa;
			else
			{
				nfa = list_union( nfa, tmp_nfa );
	
				nfa_ptr = (NFA*)( nfa->pptr );
				while( nfa_ptr->next2 )
					nfa_ptr = nfa_ptr->next2;
	
				nfa_ptr->next2 = (NFA*)( tmp_nfa->pptr );
				list_free( tmp_nfa );
			}
		}
	}

	return nfa;
}

#if 0
/* -FUNCTION--------------------------------------------------------------------
	Function:		p_create_dfa_from_keywords()
	
	Author:			Jan Max Meyer
	
	Usage:			Creates a determisitic finite automation from the keywords
					defined within the grammar.
					
	Parameters:		<type>		<identifier>		<description>
	
	Returns:		<type>							<description>
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_create_dfa_from_nfa( PARSER* parser )
{
	LIST*	l;
	LIST*	nfa		= (LIST*)NULL;
	SYMBOL*	sym;

	for( l = parser->symbols; l; l = l->next )
	{
		sym = (SYMBOL*)l->pptr;
		if( sym->type == SYM_TERMINAL && sym->keyword )
			p_keyword_to_nfa( &nfa, sym->name, sym->id, parser->p_cis_keywords );
	}

	/* nfa is NULL, if there are no keywords ;) */
	if( nfa )
	{
		/*
		printf( "\n" );
		re_dbg_print_nfa( nfa, parser->p_universe );
		*/
		if( !( parser->dfa = re_build_dfa( nfa, parser->p_universe ) ) )
		{
			re_free_nfa_table( nfa );
			OUT_OF_MEMORY;
		}
		
		if( !( parser->dfa = re_dfa_minimize( parser->dfa, parser->p_universe ) ) )
		{
			re_free_dfa( &( parser->dfa ) );
			OUT_OF_MEMORY;
		}
	}
}
#endif

