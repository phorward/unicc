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
	20.08.2011	Jan Max Meyer	Mark productions a rewritten if they are
								already done, to avoid problems with multiple
								left-hand sides (they caused a problem that
								rewritten productions would be rewritten
								multiple times)
----------------------------------------------------------------------------- */
void p_rewrite_grammar( PARSER* parser )
{
	LIST	*	l,
			*	m,
			*	stack		= (LIST*)NULL,
			*	done		= (LIST*)NULL,
			*	rewritten	= (LIST*)NULL;
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
				p = (PROD*)list_access( l );

				/* Don't rewrite a production twice! */
				if( list_find( rewritten, p ) > -1 )
					continue;

				for( m = p->rhs; m; m = m->next )
				{
					sym = (SYMBOL*)list_access( m );

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
						/* Do not rewrite system terminals! */
						if( sym->type == SYM_SYSTEM_TERMINAL )
							continue;

						/* Construct derivative symbol name */
						deriv = pstrdup( sym->name );
						
						/* Create unique symbol name */
						do
						{
							deriv = pstr_append_str( deriv,
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
							nsym->keyword = sym->keyword;
							nsym->vtype = sym->vtype;
	
							nsym->generated = TRUE;
							nsym->derived_from = sym;
						}
						
						/* Replace the rewritten symbol with the
						  		production's symbol! */						
						m->pptr = nsym;

						pfree( deriv );
					}
				}

				/* Mark this production as already rewritten! */
				rewritten = list_push( rewritten, p );
			}
		}
	}

	done = list_free( done );
	rewritten = list_free( rewritten );
	stack = (LIST*)NULL;

	/* Build a new goal symbol */
	deriv = pstrdup( parser->goal->name );

	do
	{
		deriv = pstr_append_str( deriv, P_REWRITTEN_TOKEN, FALSE );
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
	pfree( deriv );

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
						diff = ccl_free( diff );
						inter = ccl_free( inter );
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
						inter = ccl_free( inter );

					rsym = p_get_symbol( parser, (void*)diff,
									SYM_CCL_TERMINAL, TRUE );
					rsym->used = TRUE;
					rsym->defined = TRUE;

					/* Re-configure symbol */	
					tsym->ccl = ccl_free( tsym->ccl );
					tsym->name = pstr_append_str( tsym->name,
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
					inter = ccl_free( inter );
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

	/* 
	 * If nonterminal symbol has a precedence,
	 * attach it to all its productions!
	 */
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
					
	Parameters:		PARSER*		parser				Pointer to parser 
													information structure;
													This huge shit of data,
													holding everything :)
	
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
		
		if( list_count( p->rhs ) == 1 )
		{
			sym = (SYMBOL*)list_getptr( p->rhs, list_count( p->rhs ) - 1 );

			if( sym->type == SYM_NON_TERMINAL )
			{
				list_push( p->rhs, (void*)parser->end_of_input );
				list_push( p->rhs_idents, (void*)NULL );

				return; /* Nothing to do anymore! */
			}
		}
	}

	/* Setup a new goal symbol */
	deriv = pstr_append_str( pstrdup( parser->goal->name ),
				P_REWRITTEN_TOKEN, FALSE );

	if( !( sym = p_get_symbol( parser, deriv, SYM_NON_TERMINAL, TRUE ) ) )
	{
		OUTOFMEM;
		return;
	}

	sym->generated = TRUE;
	sym->vtype = parser->goal->vtype;

	pfree( deriv );

	if( !( p = p_create_production( parser, sym ) ) )
	{
		OUTOFMEM;
		return;
	}

	p_append_to_production( p, parser->goal, (uchar*)NULL );
	p_append_to_production( p, parser->end_of_input, (uchar*)NULL );

	parser->goal = sym;
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
	17.01.2011	Jan Max Meyer	Sort keyword regex terminals before any
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
											SYM_SYSTEM_TERMINAL,
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
	Function:		p_charsets_to_ptn()
	
	Author:			Jan Max Meyer
	
	Usage:			Turns character-classes into patterns, to be later
					integrated into lexical analyzers. This step can only be
					done after all grammar-related revisions had been finished,
					and no more character classes are added.

	Parameters:		PARSER*		parser			Pointer to the parser
												information structure.
	
	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_charsets_to_ptn( PARSER* parser )
{
	SYMBOL*			sym;
	LIST*			l;

	pregex_nfa_st*	begin;
	pregex_nfa_st*	end;

	PROC( "p_charsets_to_ptn" );
	PARMS( "parser", "%p", parser );

	LISTFOR( parser->symbols, l )
	{
		sym = (SYMBOL*)list_access( l );

		if( sym->type == SYM_CCL_TERMINAL )
			sym->ptn = pregex_ptn_create_char( sym->ccl );
	}

	VOIDRET;
}

