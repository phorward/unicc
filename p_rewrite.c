/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator 
Copyright (C) 2006-2011 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com/ ++ unicc<<AT>>phorward-software<<DOT>>com

File:	p_rewrite.c
Author:	Jan Max Meyer
Usage:	Grammar revision functions

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
	Function:		p_rewrite_grammar()
	
	Author:			Jan Max Meyer
	
	Usage:			Rewrites the grammar. The revision is done to simulate
					tokens which are separated by whitespaces.
					
	Parameters:		PARSER*		parser				Pointer to parser informa-
													tion structure
	
	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	26.03.2008	Jan Max Meyer	Use keyname with special type prefix for hash
								table access
----------------------------------------------------------------------------- */
void p_rewrite_grammar( PARSER* parser )
{
	LIST	*	l,
			*	m,
			*	stack		= (LIST*)NULL,
			*	done		= (LIST*)NULL;
	SYMBOL	*	ws_all		= (SYMBOL*)NULL,
			*	ws_list,
			*	ws_optlist,
			*	sym,
			*	nsym;
	PROD	*	p;
	uchar	*	deriv;

	/* Create productions for all whitespaces */
	for( l = parser->symbols; l; l = l->next )
	{
		sym = l->pptr;

		if( sym->whitespace )
		{
			if( !ws_all )
			{
				ws_all = p_get_symbol( parser, P_WHITESPACE,
						SYM_NON_TERMINAL, TRUE );
				ws_all->lexem = TRUE;
				ws_all->generated = TRUE;
			}

			p = p_create_production( parser, ws_all );
			p_append_to_production( p, sym, (uchar*)NULL );
		}
	}

	if( !ws_all )
		return;

	ws_all->whitespace = TRUE;

	ws_list = p_positive_closure( parser, ws_all );
	ws_list->lexem = TRUE;	
	ws_list->whitespace = TRUE;

	ws_optlist = p_optional_closure( parser, ws_list );
	ws_optlist->lexem = TRUE;
	ws_optlist->whitespace = TRUE;

	/*
		Find out all lexeme non-terminals and those
		which belong to them.
	*/
	for( l = parser->symbols; l; l = l->next )
	{
		sym = l->pptr;

		if( sym->lexem && sym->type == SYM_NON_TERMINAL )
		{
			done = list_push( done, sym );
			stack = list_push( stack, sym );
		}
	}

	while( list_count( stack ) )
	{
		stack = list_pop( stack, (void**)&sym );

		for( l = sym->productions; l; l = l->next )
		{
			p = l->pptr;
			for( m = p->rhs; m; m = m->next )
			{
				sym = m->pptr;

				if( sym->type == SYM_NON_TERMINAL )
				{
					if( list_find( done, sym ) == -1 )
					{
						done = list_push( done, sym );
						stack = list_push( stack, sym );
						sym->lexem = TRUE;
					}
				}
			}
		}
	}

	/*
		Find all non-terminals from goal; If there is a call to a
		lexem non-terminal or a terminal, rewrite their rules and
		replace them.
	*/
	if( !( parser->goal->lexem ) )
	{
		done = list_free( done );
		stack = (LIST*)NULL;

		done = list_push( done, parser->goal );
		stack = list_push( stack, parser->goal );

		while( list_count( stack ) )
		{
			stack = list_pop( stack, (void**)&sym );
			for( l = sym->productions; l; l = l->next )
			{
				p = l->pptr;
				for( m = p->rhs; m; m = m->next )
				{
					sym = m->pptr;

					if( sym->type == SYM_NON_TERMINAL
						&& !( sym->lexem ) )
					{
						if( list_find( done, sym ) == -1 )
						{
							done = list_push( done, sym );
							stack = list_push( stack, sym );
						}
					}
					else if( ( sym->type == SYM_NON_TERMINAL && sym->lexem )
							|| IS_TERMINAL( sym ) )
					{
						/* Do not rewrite the eof and error resync symbol! */
						if( sym == parser->end_of_input
							|| sym->type == SYM_ERROR_RESYNC )
							continue;

						/* Construct derivative symbol name */
						deriv = p_strdup( sym->name );
						
						/* Create unique symbol name */
						do
						{
							deriv = p_str_append( deriv,
										P_REWRITTEN_TOKEN, FALSE );
							nsym = p_get_symbol( parser, deriv,
										SYM_NON_TERMINAL, FALSE );
						}
						while( nsym && nsym->derived_from != sym );
						
						/* If you already found a symbol, don't do anything! */
						if( !nsym )
						{
							nsym = p_get_symbol( parser, deriv,
										SYM_NON_TERMINAL, TRUE );

							p = p_create_production( parser, nsym );
							p_append_to_production( p, sym, (uchar*)NULL );
							p_append_to_production( p, ws_optlist,
															(uchar*)NULL );
							
							/* p_dump_production( stdout, p, TRUE, FALSE ); */
	
							nsym->prec = sym->prec;
							nsym->assoc = sym->assoc;
							nsym->nullable = sym->nullable;
							nsym->generated = TRUE;
							nsym->keyword = sym->keyword;
							nsym->vtype = sym->vtype;
	
							nsym->derived_from = sym;
						}
						
						/* Replace the rewritten symbol with the
						  		production's symbol! */						
						m->pptr = nsym;

						p_free( deriv );
					}
				}
			}
		}
	}

	done = list_free( done );
	stack = (LIST*)NULL;

	/* Build a new goal symbol */
	deriv = p_strdup( parser->goal->name );

	do
	{
		deriv = p_str_append( deriv, P_REWRITTEN_TOKEN, FALSE );
		sym = p_get_symbol( parser, deriv, SYM_NON_TERMINAL, FALSE );							
	}
	while( sym && sym->derived_from != parser->goal );

	sym = p_get_symbol( parser, deriv, SYM_NON_TERMINAL, TRUE );
	if( !sym )
	{
		OUTOFMEM;
		return;
	}

	sym->generated = TRUE;
	p_free( deriv );

	p = p_create_production( parser, sym );
	if( !p )
	{
		OUTOFMEM;
		return;
	}

	p_append_to_production( p, ws_optlist, (uchar*)NULL );
	p_append_to_production( p, parser->goal, (uchar*)NULL );
	parser->goal = sym;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_unique_charsets()
	
	Author:			Jan Max Meyer
	
	Usage:			Rewrites the grammar to work with uniquely identifyable
					character sets instead of overlapping ones. This function
					was completely rewritten in Nov 2009.
					
	Parameters:		PARSER*		parser			Pointer to parser to be
												rewritten
	
	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	11.11.2009	Jan Max Meyer	Redesign of function, to work with full
								Unicode-range character classes.
----------------------------------------------------------------------------- */
void p_unique_charsets( PARSER* parser )
{
	LIST*		l;
	LIST*		m;
	SYMBOL*		sym;
	SYMBOL*		tsym;
	SYMBOL*		nsym;
	SYMBOL*		rsym;
	PROD*		p;
	CCL			inter;
	CCL			diff;
	int			old_prod_cnt;
	uchar*		tmpstr;

	PROC( "p_unique_charsets" );

	do
	{
		old_prod_cnt = list_count( parser->productions );

		LISTFOR( parser->symbols, l )
		{
			/* Get symbol pointer */
			sym = (SYMBOL*)list_access( l );
			if( sym->type != SYM_CCL_TERMINAL )
				continue;
				
			MSG( "NEXT SYMBOL FOR REVISION" );
			VARS( "sym->name", "%s", sym->name );
			
			/*
			fprintf( stderr, "sym->ccl: %d\n", ccl_size( sym->ccl ) );
			ccl_print( stderr, sym->ccl, 1 );
			*/

			/* Find overlapping character classes */
			MSG( "Searching for overlapping character classes" );
			LISTFOR( parser->symbols, m )
			{
				/* Get valid symbol pointer */
				/* if( l == m )
					continue; */

				tsym = (SYMBOL*)list_access( m );

				if( tsym->type != SYM_CCL_TERMINAL )
					continue;

				VARS( "tsym->name", "%s", tsym->name );

				inter = ccl_intersect( sym->ccl, tsym->ccl );
				
				/*
				fprintf( stderr, "inter: %d\n", ccl_size( inter ) );
				ccl_print( stderr, inter, 1 );
				fprintf( stderr, "tsym->ccl: %d\n", ccl_size( tsym->ccl ) );
				ccl_print( stderr, tsym->ccl, 1 );
				*/
				
				VARS( "ccl_size( inter )", "%d", ccl_size( inter ) );
				if( ccl_size( inter ) )
				{
					MSG( "Intersections found with tsym" );

					/* Create charclass-symbol for remaining symbols */					
					diff = ccl_diff( tsym->ccl, inter );
					if( !ccl_size( diff ) )
					{
						ccl_free( diff );
						ccl_free( inter );
						continue;
					}

					/* Disallow intersections in context-sensitive model */
					if( parser->p_mode == MODE_INSENSITIVE )
					{
						if( !( tmpstr = ccl_to_str( inter, TRUE ) ) )
							OUTOFMEM;

						p_error( parser, ERR_CHARCLASS_OVERLAP,
									ERRSTYLE_FATAL, tmpstr );

						pfree( tmpstr );
						continue;
					}
					
					/* Create charclass-symbol for intersecting symbols */										
					if( !( nsym = p_get_symbol( parser, (void*)inter,
									SYM_CCL_TERMINAL, FALSE ) ) )
					{
						nsym = p_get_symbol( parser, (void*)inter,
										SYM_CCL_TERMINAL, TRUE );					
						nsym->used = TRUE;
						nsym->defined = TRUE;
					}
					else
						ccl_free( inter );

					rsym = p_get_symbol( parser, (void*)diff,
									SYM_CCL_TERMINAL, TRUE );
					rsym->used = TRUE;
					rsym->defined = TRUE;

					/* Re-configure symbol */	
					ccl_free( tsym->ccl );
					tsym->name = p_str_append( tsym->name,
									P_REWRITTEN_CCL, FALSE );
					tsym->type = SYM_NON_TERMINAL;
					tsym->first = list_free( tsym->first );
					
					/* Create & append productions */
					p = p_create_production( parser, tsym );
					p_append_to_production( p, nsym, (uchar*)NULL );

					p = p_create_production( parser, tsym );
					p_append_to_production( p, rsym, (uchar*)NULL );
				}
				else
				{
					MSG( "Has no intersections, next" );
					ccl_free( inter );
				}
			}
		}
		
		/*
		fprintf( stderr, "-----\nCURRENT GRAMMAR:\n" );
		p_dump_grammar( stderr, parser );
		getchar();
		*/
	}
	while( old_prod_cnt != list_count( parser->productions ) );
	
	
#if 0			
			MSG( "Revision of base" );
			VARS( "ccl_size( inter )", "%d", ccl_size( inter ) );
			
			/* Rewrite base */
			if( ccl_size( inter ) )
			{
				MSG( "Rewriting character class" );
				VARS( "sym->name", "%s", sym->name );

				base = ccl_diff( sym->ccl, inter );

				fprintf( stderr, "base: %d\n", ccl_size( base ) );
				ccl_print( stderr, base, 1 );
				fprintf( stderr, "inter: %d\n", ccl_size( inter ) );
				ccl_print( stderr, inter, 1 );
				fprintf( stderr, "sym->ccl: %d\n", ccl_size( sym->ccl ) );
				ccl_print( stderr, sym->ccl, 1 );

				/* Create first production from 'base' */
				VARS( "ccl_size( base )", "%d", ccl_size( base ) );
				if( ccl_size( base ) )
				{
					/* Re-configure symbol to be a non-terminal, append
						P_REWRITTEN_CCL to its name */
					ccl_free( sym->ccl );
					sym->name = p_str_append( sym->name,
									P_REWRITTEN_CCL, FALSE );
					sym->type = SYM_NON_TERMINAL;
					sym->first = list_free( sym->first );
				
					VARS( "sym->name", "%s", sym->name );

					tsym = p_get_symbol( parser, (void*)base,
							SYM_CCL_TERMINAL, TRUE );
					tsym->used = TRUE;
					tsym->defined = TRUE;
				
					VARS( "sym", "%p", sym );
					VARS( "tsym", "%p", tsym );
				
					MSG( "Appending tsym to sym" );
					p = p_create_production( parser, sym );
					p_append_to_production( p, tsym, (uchar*)NULL );
					
				
					/* Create second production from 'intersect' */
					tsym = p_get_symbol( parser, (void*)intersect,
							SYM_CCL_TERMINAL, TRUE );
					tsym->used = TRUE;
					tsym->defined = TRUE;
		
					VARS( "sym", "%p", sym );
					VARS( "tsym", "%p", tsym );
		
					MSG( "Appending tsym to sym" );
					p = p_create_production( parser, sym );
					p_append_to_production( p, tsym, (uchar*)NULL );
					
					tsym = sym;
				}
				
				ccl_free( base );

				
				/* Now, create productions to intersecting symbols */
				LISTFOR( parser->symbols, m )
				{
					tsym = (SYMBOL*)list_access( m );
					if( tsym->type != SYM_CCL_TERMINAL )
						continue;

					tinter = ccl_intersect( tsym->ccl, inter );
					VARS( "ccl_size( tinter )", "%d", ccl_size( tinter ) );
					if( ccl_size( tinter ) )
					{
						MSG( "Having intersection with tsym - revision!" );
						VARS( "tsym->name", "%s", tsym->name );

						csym = p_get_symbol( parser, (void*)tinter,
									SYM_CCL_TERMINAL, TRUE );

						csym->used = TRUE;
						csym->defined = TRUE;
						
						
						p = p_create_production( parser, sym );
						p_append_to_production( p, tsym, (uchar*)NULL );

						tmp = ccl_diff( inter, tinter );
						ccl_free( inter );
						inter = tmp;
					}
					else
						ccl_free( tinter );
				}

				if( ccl_size( inter ) )
				{
					MSG( "Obtaining symbol" );
					tsym = p_get_symbol( parser, (void*)inter,
							SYM_CCL_TERMINAL, TRUE );
					tsym->used = TRUE;
					tsym->defined = TRUE;

					p = p_create_production( parser, sym );
					p_append_to_production( p, tsym, (uchar*)NULL );
				}
			}
			else
			{
				MSG( "No revision required - no intersections!" );
			}
			
			ccl_free( inter );
		}
#endif

	VOIDRET;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_fix_precedences()
	
	Author:			Jan Max Meyer
	
	Usage:			Fixes the precedence and associativity information of the
					current grammar to be prepared for LALR(1) table generation.
					
	Parameters:		<type>		<identifier>		<description>
	
	Returns:		<type>							<description>
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_fix_precedences( PARSER* parser )
{
	PROD*	p;
	int		i;
	LIST*	l;
	LIST*	m;
	BOOLEAN	found;
	SYMBOL*	sym;

	/* If nonterminal symbol has a precedence, attach it to all its productions! */
	for( l = parser->symbols; l; l = l->next )
	{
		sym = (SYMBOL*)l->pptr;

		if( sym->type == SYM_NON_TERMINAL
			&& sym->prec > 0
				&& !( sym->generated ) )
		{
			for( m = sym->productions; m; m = m->next )
			{
				p = (PROD*)m->pptr;

				if( p->prec <= sym->prec )
					p->prec = sym->prec;
			}
		}
	}

	/*
		Set production's precedence level to the
		one of the rightmost terminal!
	*/
	for( l = parser->productions; l; l = l->next )
	{
		p = (PROD*)l->pptr;

		if( p->prec > 0 )
			continue;

		/* First try to find leftmost terminal */
		found = FALSE;
		for( i = list_count( p->rhs ) - 1; i >= 0; i-- )
		{
			sym = (SYMBOL*)list_getptr( p->rhs, i );

			if( sym->lexem )
			{
				p->prec = sym->prec;
				found = TRUE;
				break;
			}
		}
		
		/*
			If there is no terminal, use rightmost
			non-terminal with a precedence
		*/
		if( !found )
			for( i = list_count( p->rhs ) - 1; i >= 0; i-- )
			{
				sym = (SYMBOL*)list_getptr( p->rhs, i );

				if( sym->prec > p->prec )
				{
					p->prec = sym->prec;
					break;
				}
			}
	}
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_inherit_fixiations()
	
	Author:			Jan Max Meyer
	
	Usage:			Inherits the fixiation definitions once done with "fixate"
					parser directive.
					
	Parameters:		PARSER*		parser				Pointer to parser information
													structure; This huge shit of
													data, holding everything :)
	
	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_inherit_fixiations( PARSER* parser )
{
	LIST*		l;
	LIST*		m;
	SYMBOL*		sym;
	PROD*		p;
	LIST*		done	= (LIST*)NULL;
	LIST*		stack	= (LIST*)NULL;

	for( l = parser->symbols; l; l = l->next )
	{
		sym = l->pptr;

		if( sym->fixated && sym->type == SYM_NON_TERMINAL )
		{
			done = list_push( done, sym );
			stack = list_push( stack, sym );
		}
	}

	while( list_count( stack ) )
	{
		stack = list_pop( stack, (void**)&sym );

		for( l = sym->productions; l; l = l->next )
		{
			p = l->pptr;
			for( m = p->rhs; m; m = m->next )
			{
				sym = m->pptr;

				if( sym->type == SYM_NON_TERMINAL )
				{
					if( list_find( done, sym ) == -1 )
					{
						done = list_push( done, sym );
						stack = list_push( stack, sym );
						sym->fixated = TRUE;
					}
				}
			}
		}
	}

	list_free( done );
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_inherit_vtypes()
	
	Author:			Jan Max Meyer
	
	Usage:			Inherits value types of rewritten symbols from their
					base. This is required for symbols that where generated
					before their definition in the code - where a possible
					value type is still unknown.
					
	Parameters:		<type>		<identifier>		<description>
	
	Returns:		<type>							<description>
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_inherit_vtypes( PARSER* parser )
{
	SYMBOL*	sym;
	LIST*	l;
	
	for( l = parser->symbols; l; l = list_next( l ) )
	{
		sym = (SYMBOL*)list_access( l );
		
		if( !sym->vtype && sym->derived_from )
			sym->vtype = sym->derived_from->vtype;
	}
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_setup_single_goal()
	
	Author:			Jan Max Meyer
	
	Usage:			Sets up a single goal symbol, if necessary.
					
	Parameters:		<type>		<identifier>		<description>
	
	Returns:		<type>							<description>
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_setup_single_goal( PARSER* parser )
{
	SYMBOL*	sym;
	PROD*	p;
	uchar*	deriv;

	if( list_count( parser->goal->productions ) == 1 )
	{
		p = ((PROD*)(parser->goal->productions->pptr));
		
		if( list_count( p->rhs ) <= 2 )
		{
			sym = (SYMBOL*)list_getptr( p->rhs, list_count( p->rhs ) - 1 );

			if( list_count( p->rhs ) == 2 &&
					IS_TERMINAL( sym ) && !( parser->end_of_input ) )
			{
				parser->end_of_input = sym;
				/* Pop the end-of-input symbol and add it to the parser as end-of-input! */
				/* list_pop( p->rhs, (void**)( &( parser->end_of_input ) ) );
				list_pop( p->rhs_idents, (void**) &tmp );
				p_free( tmp );
				*/

				return; /* Nothing to do anymore! */
			}
			else if( list_count( p->rhs ) == 1 &&
					sym->type == SYM_NON_TERMINAL )
			{
				if( !( parser->end_of_input ) )
				{
					parser->end_of_input = p_get_symbol( parser,
						ccl_create( P_DEF_EOF_SYMBOL ),
							SYM_CCL_TERMINAL, TRUE );
					parser->end_of_input->generated = TRUE;

					p_error( parser, ERR_ASSUMING_DEF_EOF,
							ERRSTYLE_WARNING, P_DEF_EOF_SYMBOL );
				}

				list_push( p->rhs, (void*)parser->end_of_input );
				list_push( p->rhs_idents, (void*)NULL );
				return; /* Nothing to do anymore! */
			}
		}
	}

	/* Setup a new goal symbol */
	deriv = p_str_append( p_strdup( parser->goal->name ),
				P_REWRITTEN_TOKEN, FALSE );

	sym = p_get_symbol( parser, deriv, SYM_NON_TERMINAL, TRUE );
	if( !sym )
	{
		OUTOFMEM;
		return;
	}
	sym->generated = TRUE;

	p_free( deriv );

	p = p_create_production( parser, sym );
	if( !p )
	{
		OUTOFMEM;
		return;
	}

	p_append_to_production( p, parser->goal, (uchar*)NULL );
	parser->goal = sym;

	if( !( parser->end_of_input ) )
	{
		parser->end_of_input = p_get_symbol( parser,
			(void*)ccl_create( P_DEF_EOF_SYMBOL ), SYM_CCL_TERMINAL, TRUE );

		if( !( parser->end_of_input ) )
		{
			OUTOFMEM;
			return;
		}

		parser->end_of_input->generated = TRUE;

		p_error( parser, ERR_ASSUMING_DEF_EOF, ERRSTYLE_WARNING, P_DEF_EOF_SYMBOL );
	}

	p_append_to_production( p, parser->end_of_input, (uchar*)NULL );
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_symbol_order()
	
	Author:			Jan Max Meyer
	
	Usage:			Re-orders all parser symbols after all parsing and revision
					steps are done according to the following order:

					- Keywords-Terminals
					- Regex-Terminals
					- Character-Class Terminals
					- Nonterminals

					After this, id's are re-assigned.
					
	Parameters:		PARSER*		parser			Pointer to the parser
												information structure.
	
	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	17.01.2010	Jan Max Meyer	Sort keyword regex terminals before any
								other kind of terminal... hmm ... in the past,
								keywords where of type SYM_KW_TERMINAL, now
								they are SYM_REGEX_TERMINAL, so this must be
								hard-coded here, check for it if one's like
								to change this (be careful!!)
----------------------------------------------------------------------------- */
void p_symbol_order( PARSER* parser )
{
	LIST*			new_order		= (LIST*)NULL;
	LIST*			l;
	LIST*			m;
	SYMBOL*			sym;
	pregex_nfa_st*	n_st;

	int				sort_order[]	=	{
											/* SYM_REGEX_TERMINAL,
												once for keywords, then for
												real regex terminals
											*/
											SYM_REGEX_TERMINAL,
											SYM_REGEX_TERMINAL,
											SYM_CCL_TERMINAL,
											SYM_ERROR_RESYNC,
											SYM_NON_TERMINAL
										};
	int				i;
	int				id				= 0;

	/* Sort symbols according above sort order into a new list */
	for( i = 0; i < sizeof( sort_order ) / sizeof( *sort_order ); i++ )
	{
		LISTFOR( parser->symbols, l )
		{
			sym = (SYMBOL*)list_access( l );

			if( sym->type == sort_order[i] )
			{
				/* That's it... */
				if( i == 0 && sym->keyword == FALSE )
					continue;
				else if( i == 1 && sym->keyword == TRUE )
					continue;

				if( !( new_order = list_push( new_order, (void*)sym ) ) )
					OUTOFMEM;

				/* In case of an NFA-based token, the accepting-IDs
					of the constructed machine need to be updated */
				LISTFOR( sym->nfa.states, m )
				{
					n_st = (pregex_nfa_st*)list_access( m );

					if( n_st->accept != REGEX_ACCEPT_NONE &&
							n_st->accept == sym->id )
						n_st->accept = id;
				}

				/* Re-assign new ID! */
				sym->id = id++;
			}
		}
	}

	/* Replace the list */
	list_free( parser->symbols );
	parser->symbols = new_order;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_charclass_to_nfa()
	
	Author:			Jan Max Meyer
	
	Usage:			Turns character-classes into NFAs, to be later integrated
					into lexical analyzers. This step can only be done after
					all grammar-related revisions are done, and no more
					character classes are added.

	Parameters:		PARSER*		parser			Pointer to the parser
												information structure.
	
	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_charsets_to_nfa( PARSER* parser )
{
	SYMBOL*			sym;
	LIST*			l;

	pregex_nfa_st*	begin;
	pregex_nfa_st*	end;

	PROC( "p_charsets_to_nfa" );
	PARMS( "parser", "%p", parser );

	LISTFOR( parser->symbols, l )
	{
		sym = (SYMBOL*)list_access( l );

		if( sym->type == SYM_CCL_TERMINAL )
		{
			/*
				Make a very tiny NFA with simply one transition
			*/
			if( !( begin = pregex_nfa_create_state( &( sym->nfa ),
						(uchar*)NULL, REGEX_MOD_NONE ) ) )
				OUTOFMEM;
				
			if( !( begin = begin->next = pregex_nfa_create_state( &( sym->nfa ),
						(uchar*)NULL, REGEX_MOD_NONE ) ) )
				OUTOFMEM;

			if( !( begin->ccl = ccl_dup( sym->ccl ) ) )
				OUTOFMEM;

			if( !( begin->next = end = pregex_nfa_create_state( &( sym->nfa ),
						(uchar*)NULL, REGEX_MOD_NONE ) ) )
				OUTOFMEM;

			end->accept = sym->id;
			/* sym->type = SYM_REGEX_TERMINAL; */
		}
	}

	VOIDRET;
}

