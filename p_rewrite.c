/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator 
Copyright (C) 2006-2009 by Phorward Software Technologies, Jan Max Meyer
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
	
	Usage:			Rewrites the grammar. The revision is done to simulate tokens
					which are separated by whitespaces.
					
	Parameters:		<type>		<identifier>		<description>
	
	Returns:		<type>							<description>
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	26.03.2008	Jan Max Meyer	Use keyname with special type prefix for hash table
								access
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
							deriv = p_str_append( deriv, P_REWRITTEN_TOKEN, FALSE );
							nsym = p_get_symbol( parser, deriv, SYM_NON_TERMINAL, FALSE );
						}
						while( nsym && nsym->derived_from != sym );
						
						/* If you already found a symbol, don't do anything! */
						if( !nsym )
						{
							nsym = p_get_symbol( parser, deriv, SYM_NON_TERMINAL, TRUE );

							p = p_create_production( parser, nsym );
							p_append_to_production( p, sym, (uchar*)NULL );
							p_append_to_production( p, ws_optlist, (uchar*)NULL );
							
							/* p_dump_production( stdout, p, TRUE, FALSE ); */
	
							nsym->prec = sym->prec;
							nsym->assoc = sym->assoc;
							nsym->nullable = sym->nullable;
							nsym->generated = TRUE;
							nsym->keyword = sym->keyword;
							nsym->vtype = sym->vtype;
	
							nsym->derived_from = sym;
						}
						
						/* Replace the rewritten symbol with the production's symbol! */						
						m->pptr = nsym;

						p_free( deriv );
					}
				}
			}
		}
	}

	done = list_free( done );
	stack = (LIST*)NULL;

	/* Rewrite goal symbol, possibly again, for whitespaces in front of source */
	/*
	p = parser->goal->productions->pptr; / * Goal IS here already single! * /

	l = p->rhs;
	m = p->rhs_idents;

	p->rhs = list_push( (LIST*)NULL, ws_optlist );
	p->rhs->next = l;

	p->rhs_idents = list_push( (LIST*)NULL, (char*)NULL );
	p->rhs_idents->next = m;
	*/

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
		OUT_OF_MEMORY;
		return;
	}

	sym->generated = TRUE;
	p_free( deriv );

	p = p_create_production( parser, sym );
	if( !p )
	{
		OUT_OF_MEMORY;
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
					character sets instead of overlapping ones.
					
	Parameters:		<type>		<identifier>		<description>
	
	Returns:		<type>							<description>
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_unique_charsets( PARSER* parser )
{
	int		i;
	int		j;
	LIST*	l;
	LIST*	m;
	LIST*	n;
	int*	cmap;
	bitset	charmap;
	bitset	nmap;
	bitset	xmap;
	LIST*	csets;
	SYMBOL*	sym;
	SYMBOL* tsym;
	SYMBOL*	xsym;
	uchar*	nname;
	int		old_prod_cnt;
	BOOLEAN	done_something;
	PROD*	p;
	
	if( !parser )
		return;

	/* Allocate character maps over character universe */
	cmap = (int*)p_malloc( parser->p_universe * sizeof( int ) );

	/* nmap = (int*)p_malloc( parser->p_universe * sizeof( int ) ); */
	/* xmap = (int*)p_malloc( parser->p_universe * sizeof( int ) ); */

	if( !cmap )
	{
		OUT_OF_MEMORY;
		return;
	}

	/* Perform the algorithm! */
	do
	{
		old_prod_cnt = list_count( parser->productions );

		for( l = parser->symbols; l; l = l->next )
		{
			sym = l->pptr;

			if( !IS_TERMINAL( sym ) || sym->keyword || sym->extern_token )
				continue;

			/* memset( cmap, 0, parser->p_universe * sizeof( int ) ); */
			for( i = 0; i < parser->p_universe; i++ )
				cmap[i] = -1;

			charmap = p_ccl_to_map( parser, sym->name );
			
			if( !charmap )
			{
				OUT_OF_MEMORY;
				return;
			}

			for( i = 0; i < parser->p_universe; i++ )
				if( bitset_get( charmap, i ) )
					cmap[i] = sym->id;

			/*
			p_dump_map( stdout, charmap, parser->p_universe );
			getchar();
			*/

			bitset_free( charmap );

			/* Find overlapping character classes */
			for( m = parser->symbols; m; m = m->next )
			{
				tsym = m->pptr;
				
				if( !IS_TERMINAL( tsym )
						|| tsym->keyword
							|| tsym->extern_token
								|| tsym == sym )
					continue;

				charmap = p_ccl_to_map( parser, tsym->name );
				if( !charmap )
				{
					OUT_OF_MEMORY;
					return;
				}

				for( i = 0; i < parser->p_universe; i++ )
					if( bitset_get( charmap, i ) && cmap[i] == sym->id )
					{
						if( parser->p_model == MODEL_CONTEXT_INSENSITIVE )
						{
							char	tmp[ 10 + 1 ];

							if( i >= 32 && i <= 126 )
								sprintf( tmp, "%c", i );
							else
								sprintf( tmp, "\\%d", i );

							p_error( ERR_CHARCLASS_OVERLAP, ERRSTYLE_FATAL, tmp );
						}

						cmap[i] = tsym->id;
					}

				bitset_free( charmap );
			}
			
			/*
			if( !strcmp( sym->name, "\\n" ) || !strcmp( sym->name, "\\n'" ) )
			{
				printf( "2 sym->name = >%s< (id %d)\n", sym->name, sym->id );
				for( i = 0; i < 128; i++ )
					printf( "%03d:%02d%s", i, cmap[i], ( i > 0 && i % 10 == 0 ? "\n" : " " ) );
				printf( "\n\n" );
			}
			
			p_dump_grammar( stderr, parser );
			getchar();			
			*/


			csets = (LIST*)NULL;
			for( m = parser->symbols; m; m = m->next )
			{
				tsym = m->pptr;

				if( !IS_TERMINAL( tsym ) || tsym->keyword || tsym->extern_token )
					continue;

				done_something = FALSE;
				/* memset( nmap, 0, parser->p_universe * sizeof( int ) ); */
				nmap = bitset_create( parser->p_universe );
				if( !nmap )
				{
					OUT_OF_MEMORY;
					return;
				}

				for( i = 0; i < parser->p_universe; i++ )
					if( cmap[i] == tsym->id )
					{
						bitset_set( nmap, i, 1 );
						done_something = TRUE;
					}

				if( done_something )
				{
					/* p_dump_map( stdout, nmap, parser->p_universe ); */

					/* Is there already such a set? */
					for( n = parser->symbols; n; n = n->next )
					{
						xsym = n->pptr;

						if( !IS_TERMINAL( xsym )
							|| xsym->keyword
								|| xsym->extern_token )
							continue;

						xmap = p_ccl_to_map( parser, xsym->name );
						if( !xmap )
						{
							OUT_OF_MEMORY;
							return;
						}

						for( j = 0; j < parser->p_universe; j++ )
							if( bitset_get( nmap, j ) != bitset_get( xmap, j ) )
								break;
						
						/*
						if( j == parser->p_universe )
						{
							printf( "@@ >%s<\n", xsym->name );
							p_dump_map( stdout, xmap, parser->p_universe );
						}
						*/

						bitset_free( xmap );

						if( j == parser->p_universe )
							break;
					}

					/* printf( "%p %p\n", sym, xsym ); */
					if( !n )
					{
						nname = p_map_to_ccl( parser, nmap );
						/*
						printf( "Making >%s<\n", nname );
						p_dump_map( stdout, nmap, parser->p_universe );
						getchar();
						*/
						
						xsym = p_get_symbol( parser, nname,
							SYM_CCL_TERMINAL, TRUE );
						xsym->generated = TRUE;
						xsym->used = TRUE;
						xsym->defined = TRUE;
						p_free( nname );
					}

					if( sym != xsym )
						csets = list_push( csets, xsym );
				}

				bitset_free( nmap );
			}

			if( list_count( csets ) > 0 )
			{
				/*printf( "csets counts %d\n", list_count( csets ) );*/
				nname = p_str_append( p_strdup( sym->name ), P_REWRITTEN_CCL, FALSE );
				
				p_free( sym->name );
				sym->name = nname;
				sym->type = SYM_NON_TERMINAL;

				for( m = csets; m; m = m->next )
				{
 					/*
 					printf( "appending >%s< >%s<...\n",
 						sym->name, ((SYMBOL*)(m->pptr))->name );
 					*/
					p = p_create_production( parser, sym );
					p_append_to_production( p, m->pptr, (uchar*)NULL );
				}

				csets = list_free( csets );
			}
		}

		/* printf( "Here I am %d %d\n", old_prod_cnt, list_count( parser->productions ) ); */
	}
	while( old_prod_cnt < list_count( parser->productions ) );
	
	p_free( cmap );
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
					
	Parameters:		<type>		<identifier>		<description>
	
	Returns:		<type>							<description>
  
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
						sym->fixated = TRUE;
					}
				}
			}
		}
	}

	list_free( done );
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
			else if( list_count( p->rhs ) == 1 && sym->type == SYM_NON_TERMINAL )
			{
				if( !( parser->end_of_input ) )
				{
					parser->end_of_input = p_get_symbol( parser,
						P_DEF_EOF_SYMBOL, SYM_CCL_TERMINAL, TRUE );
					parser->end_of_input->generated = TRUE;

					p_error( ERR_ASSUMING_DEF_EOF, ERRSTYLE_WARNING, P_DEF_EOF_SYMBOL );
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
		OUT_OF_MEMORY;
		return;
	}
	sym->generated = TRUE;

	p_free( deriv );

	p = p_create_production( parser, sym );
	if( !p )
	{
		OUT_OF_MEMORY;
		return;
	}

	p_append_to_production( p, parser->goal, (uchar*)NULL );
	parser->goal = sym;

	if( !( parser->end_of_input ) )
	{
		parser->end_of_input = p_get_symbol( parser,
			P_DEF_EOF_SYMBOL, SYM_CCL_TERMINAL, TRUE );

		if( !( parser->end_of_input ) )
		{
			OUT_OF_MEMORY;
			return;
		}

		parser->end_of_input->generated = TRUE;

		p_error( ERR_ASSUMING_DEF_EOF, ERRSTYLE_WARNING, P_DEF_EOF_SYMBOL );
	}

	p_append_to_production( p, parser->end_of_input, (uchar*)NULL );
}
