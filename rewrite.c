/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
https://phorward.info ++ unicc<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	rewrite.c
Author:	Jan Max Meyer
Usage:	Grammar revision functions
----------------------------------------------------------------------------- */

#include "unicc.h"

/** Rewrites the grammar.

The revision is done to simulate tokens which are separated by whitespaces.

//parser// is the pointer to parser information structure. */
void rewrite_grammar( PARSER* parser )
{
	plistel	*	e,
			*	f;
	plist	*	stack,
			*	done,
			*	rewritten;
	SYMBOL	*	ws_all		= (SYMBOL*)NULL,
			*	ws_list,
			*	ws_optlist,
			*	sym,
			*	nsym;
	PROD	*	p;
	char	*	deriv;

	/*
	26.03.2008	Jan Max Meyer
	Use keyname with special type prefix for hash table access

	20.08.2011	Jan Max Meyer
	Mark productions a rewritten if they are already done, to avoid problems
	with multiple left-hand sides (they caused a problem that rewritten
	productions would be rewritten multiple times)
	*/

	/* Create productions for all whitespaces */
	plist_for( parser->symbols, e )
	{
		sym = (SYMBOL*)plist_access( e );

		if( sym->whitespace )
		{
			if( !ws_all )
			{
				ws_all = get_symbol( parser, P_WHITESPACE,
										SYM_NON_TERMINAL, TRUE );
				ws_all->lexem = TRUE;
				ws_all->generated = TRUE;
			}

			p = create_production( parser, ws_all );
			append_to_production( p, sym, (char*)NULL );
		}
	}

	if( !ws_all )
		return;

	ws_all->whitespace = TRUE;

	ws_list = positive_closure( parser, ws_all );
	ws_list->lexem = TRUE;
	ws_list->whitespace = TRUE;

	ws_optlist = optional_closure( parser, ws_list );
	ws_optlist->lexem = TRUE;
	ws_optlist->whitespace = TRUE;

	/*
		Find out all lexeme non-terminals and those
		which belong to them.
	*/
	done = plist_create( 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );
	stack = plist_create( 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );
	rewritten = plist_create( 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );

	plist_for( parser->symbols, e )
	{
		sym = (SYMBOL*)plist_access( e );

		if( sym->lexem && sym->type == SYM_NON_TERMINAL )
		{
			plist_push( done, sym );
			plist_push( stack, sym );
		}
	}

	while( plist_pop( stack, &sym ) )
	{
		plist_for( sym->productions, e )
		{
			p = (PROD*)plist_access( e );

			plist_for( p->rhs, f )
			{
				sym = (SYMBOL*)plist_access( f );

				if( sym->type == SYM_NON_TERMINAL )
				{
					if( !plist_get_by_ptr( done, sym ) )
					{
						sym->lexem = TRUE;

						plist_push( done, sym );
						plist_push( stack, sym );
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
		plist_erase( done );
		plist_erase( stack );

		plist_push( done, parser->goal );
		plist_push( stack, parser->goal );

		while( plist_pop( stack, &sym ) )
		{
			plist_for( sym->productions, e )
			{
				p = (PROD*)plist_access( e );

				/* Don't rewrite a production twice! */
				if( plist_get_by_ptr( rewritten, p ) )
					continue;

				plist_for( p->rhs, f )
				{
					sym = (SYMBOL*)plist_access( f );

					if( sym->type == SYM_NON_TERMINAL && !( sym->lexem ) )
					{
						if( !plist_get_by_ptr( done, sym ) )
						{
							plist_push( done, sym );
							plist_push( stack, sym );
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
							deriv = pstrcatstr( deriv,
										P_REWRITTEN_TOKEN, FALSE );
							nsym = get_symbol( parser, deriv,
										SYM_NON_TERMINAL, FALSE );
						}
						while( nsym && nsym->derived_from != sym );

						/* If you already found a symbol, don't do anything! */
						if( !nsym )
						{
							nsym = get_symbol( parser, deriv,
										SYM_NON_TERMINAL, TRUE );

							p = create_production( parser, nsym );
							append_to_production( p, sym, (char*)NULL );
							append_to_production( p, ws_optlist, (char*)NULL );

							/* dump_production( stdout, p, TRUE, FALSE ); */

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
						memcpy( f + 1, &nsym, sizeof( SYMBOL* ) );

						pfree( deriv );
					}
				}

				/* Mark this production as already rewritten! */
				plist_push( rewritten, p );
			}
		}
	}

	plist_free( done );
	plist_free( rewritten );
	plist_free( stack );

	/* Build a new goal symbol */
	deriv = pstrdup( parser->goal->name );

	do
	{
		deriv = pstrcatstr( deriv, P_REWRITTEN_TOKEN, FALSE );
		sym = get_symbol( parser, deriv, SYM_NON_TERMINAL, FALSE );
	}
	while( sym && sym->derived_from != parser->goal );

	sym = get_symbol( parser, deriv, SYM_NON_TERMINAL, TRUE );
	sym->generated = TRUE;
	pfree( deriv );

	p = create_production( parser, sym );
	if( !p )
	{
		OUTOFMEM;
		return;
	}

	append_to_production( p, ws_optlist, (char*)NULL );
	append_to_production( p, parser->goal, (char*)NULL );
	parser->goal = sym;
}

/** Rewrites the grammar to work with uniquely identifyable character sets
instead of overlapping ones. This function was completely rewritten in Nov 2009.

//parser// is the pointer to parser to be rewritten.
*/
void unique_charsets( PARSER* parser )
{
	plistel*	e;
	plistel*	f;
	SYMBOL*		sym;
	SYMBOL*		tsym;
	SYMBOL*		nsym;
	SYMBOL*		rsym;
	PROD*		p;
	pccl*		inter;
	pccl*		diff;
	int			old_prod_cnt;

	/*
	11.11.2009	Jan Max Meyer
	Entire redesign of function, to work with full
	Unicode-range character classes.
	*/

	PROC( "unique_charsets" );

	do
	{
		old_prod_cnt = plist_count( parser->productions );

		plist_for( parser->symbols, e )
		{
			/* Get symbol pointer */
			sym = (SYMBOL*)plist_access( e );
			if( sym->type != SYM_CCL_TERMINAL )
				continue;

			MSG( "NEXT SYMBOL FOR REVISION" );
			VARS( "sym->name", "%s", sym->name );

			/*
			fprintf( stderr, "sym->ccl: %d\n", pccl_size( sym->ccl ) );
			pccl_print( stderr, sym->ccl, 1 );
			*/

			/* Find overlapping character classes */
			MSG( "Searching for overlapping character classes" );
			plist_for( parser->symbols, f )
			{
				tsym = (SYMBOL*)plist_access( f );

				if( tsym->type != SYM_CCL_TERMINAL )
					continue;

				VARS( "tsym->name", "%s", tsym->name );

				inter = pccl_intersect( sym->ccl, tsym->ccl );

				/*
				fprintf( stdout, "inter = >%s< sym = >%s< tsym = >%s<\n",
					pccl_to_str( inter, TRUE ),
					pccl_to_str( sym->ccl, TRUE ),
					pccl_to_str( tsym->ccl, TRUE ) );
				*/

				VARS( "inter", "%p", inter );
				if( inter )
				{
					MSG( "Intersections found with tsym" );

					/* Create charclass-symbol for remaining symbols */
					diff = pccl_diff( tsym->ccl, inter );
					if( !pccl_size( diff ) )
					{
						diff = pccl_free( diff );
						inter = pccl_free( inter );
						continue;
					}

					/* Disallow intersections in context-sensitive model */
					if( parser->p_mode == MODE_SCANNER )
					{
						print_error( parser, ERR_CHARCLASS_OVERLAP,
									ERRSTYLE_FATAL,
										pccl_to_str( inter, TRUE ));

						inter = pccl_free( inter );
						continue;
					}

					/* Create charclass-symbol for intersecting symbols */
					if( !( nsym = get_symbol( parser, (void*)inter,
									SYM_CCL_TERMINAL, FALSE ) ) )
					{
						nsym = get_symbol( parser, (void*)inter,
										SYM_CCL_TERMINAL, TRUE );
						nsym->used = TRUE;
						nsym->defined = TRUE;
					}
					else
						inter = pccl_free( inter );

					rsym = get_symbol( parser, (void*)diff,
									SYM_CCL_TERMINAL, TRUE );
					rsym->used = TRUE;
					rsym->defined = TRUE;

					/* Re-configure symbol */
					tsym->ccl = pccl_free( tsym->ccl );
					tsym->name = pstrcatstr( tsym->name,
									P_REWRITTEN_CCL, FALSE );
					tsym->type = SYM_NON_TERMINAL;
					plist_erase( tsym->first );
					tsym->productions = plist_create( 0, PLIST_MOD_PTR );

					/* Create & append productions */
					p = create_production( parser, tsym );
					append_to_production( p, nsym, (char*)NULL );

					p = create_production( parser, tsym );
					append_to_production( p, rsym, (char*)NULL );
				}
				else
				{
					MSG( "Has no intersections, next" );
				}
			}
		}

		/*
		fprintf( stderr, "-----\nCURRENT GRAMMAR:\n" );
		dump_grammar( stderr, parser );
		getchar();
		*/
	}
	while( old_prod_cnt != plist_count( parser->productions ) );

	VOIDRET;
}

/** Fixes the precedence and associativity information of the current grammar
to be prepared for LALR(1) table generation.

//parser// is the pointer to parser information structure. */
void fix_precedences( PARSER* parser )
{
	PROD*		p;
	plistel*	e;
	plistel*	f;
	BOOLEAN		found;
	SYMBOL*		sym;

	/*
	 * If nonterminal symbol has a precedence,
	 * attach it to all its productions!
	 */
	plist_for( parser->symbols, e )
	{
		sym = (SYMBOL*)plist_access( e );

		if( sym->type == SYM_NON_TERMINAL
				&& sym->prec > 0 && !( sym->generated ) )
		{
			plist_for( sym->productions, f )
			{
				p = (PROD*)plist_access( f );

				if( p->prec <= sym->prec )
					p->prec = sym->prec;
			}
		}
	}

	/*
		Set production's precedence level to the
		one of the rightmost terminal!
	*/
	plist_for( parser->productions, e )
	{
		p = (PROD*)plist_access( e );

		if( p->prec > 0 )
			continue;

		/* First try to find leftmost terminal */
		found = FALSE;
		for( f = plist_last( p->rhs ); f; f = plist_prev( f ) )
		{
			sym = (SYMBOL*)plist_access( f );

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
			for( f = plist_last( p->rhs ); f; f = plist_prev( f ) )
			{
				sym = (SYMBOL*)plist_access( f );

				if( sym->prec > p->prec )
				{
					p->prec = sym->prec;
					break;
				}
			}
	}
}

/** Inherits the fixiation definitions once done with "fixate" parser directive.

//parser// is the pointer to parser information structure; This huge shit of
data, holding everything :) */
void inherit_fixiations( PARSER* parser )
{
	plistel*	e;
	plistel*	f;
	SYMBOL*		sym;
	PROD*		p;

	plist*		done;
	plist*		stack;

	done = plist_create( 0, PLIST_MOD_PTR );
	stack = plist_create( 0, PLIST_MOD_PTR );

	plist_for( parser->symbols, e )
	{
		sym = (SYMBOL*)plist_access( e );

		if( sym->fixated && sym->type == SYM_NON_TERMINAL )
		{
			plist_push( done, sym );
			plist_push( stack, sym );
		}
	}

	while( plist_pop( stack, &sym ) )
	{
		plist_for( sym->productions, e )
		{
			p = (PROD*)plist_access( e );

			plist_for( p->rhs, f )
			{
				sym = (SYMBOL*)plist_access( f );

				if( sym->type == SYM_NON_TERMINAL )
				{
					if( !plist_get_by_ptr( done, sym ) )
					{
						sym->fixated = TRUE;

						plist_push( done, sym );
						plist_push( stack, sym );
					}
				}
			}
		}
	}

	plist_free( stack );
	plist_free( done );
}

/** Inherits value types of rewritten symbols from their base.

This is required for symbols that where generated before their definition in
the code - where a possible value type is still unknown. */
void inherit_vtypes( PARSER* parser )
{
	SYMBOL*		sym;
	plistel*	e;

	plist_for( parser->symbols, e )
	{
		sym = (SYMBOL*)plist_access( e );

		if( !sym->vtype && sym->derived_from )
			sym->vtype = sym->derived_from->vtype;
	}
}

/** Sets up a single goal symbol, if necessary. */
void setup_single_goal( PARSER* parser )
{
	SYMBOL*	sym;
	PROD*	p;
	char*	deriv;

	if( plist_count( parser->goal->productions ) == 1 )
	{
		p = (PROD*)plist_access( plist_first( parser->goal->productions ) );

		if( plist_count( p->rhs ) == 1 )
		{
			sym = (SYMBOL*)plist_access( plist_last( p->rhs ) );

			if( sym->type == SYM_NON_TERMINAL )
			{
				plist_push( p->rhs, parser->end_of_input );
				return; /* Nothing to do anymore! */
			}
		}
	}

	/* Setup a new goal symbol */
	deriv = pstrcatstr( pstrdup( parser->goal->name ),
				P_REWRITTEN_TOKEN, FALSE );

	if( !( sym = get_symbol( parser, deriv, SYM_NON_TERMINAL, TRUE ) ) )
	{
		OUTOFMEM;
		return;
	}

	sym->generated = TRUE;
	sym->vtype = parser->goal->vtype;

	pfree( deriv );

	if( !( p = create_production( parser, sym ) ) )
	{
		OUTOFMEM;
		return;
	}

	append_to_production( p, parser->goal, (char*)NULL );
	append_to_production( p, parser->end_of_input, (char*)NULL );

	parser->goal = sym;
}

/** Turns character-classes into patterns, to be later integrated into lexical
analyzers.

This step can only be done after all grammar-related revisions had been
finished, and no more character classes are added.

//parser// is the Pointer to the parser information structure. */
void charsets_to_ptn( PARSER* parser )
{
	SYMBOL*			sym;
	plistel*			e;

	PROC( "charsets_to_ptn" );
	PARMS( "parser", "%p", parser );

	plist_for( parser->symbols, e )
	{
		sym = (SYMBOL*)plist_access( e );

		if( sym->type == SYM_CCL_TERMINAL )
			sym->ptn = pregex_ptn_create_char( sym->ccl );
	}

	VOIDRET;
}


/** Re-arrange symbol orders. */
void symbol_orders( PARSER* parser )
{
	plistel*	e;
	SYMBOL*		sym;

	PROC( "symbol_orders" );
	PARMS( "parser", "%p", parser );

	plist_sort( parser->symbols );
	plist_for( parser->symbols, e )
	{
		sym = (SYMBOL*)plist_access( e );
		sym->id = plist_offset( e );
	}

	VOIDRET;
}
