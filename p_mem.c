/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator 
Copyright (C) 2006-2011 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com/ ++ unicc<<AT>>phorward-software<<DOT>>com

File:	p_mem.c
Author:	Jan Max Meyer
Usage:	Memory maintenance and management functions for several datatypes used
		in UniCC for symbol and state management.

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
	Function:		p_get_symbol()
	
	Author:			Jan Max Meyer
	
	Usage:			Creates a new grammar symbol, or returns the grammar symbol,
					if it already exists in the symbol table.
					
					A grammar symbol can either be a terminal symbol or a
					non-terminal symbol.
					
	Parameters:		PARSER*		p					Parser information structure
					void*		dfn					Symbol definition; in case
													of a charclass terminal,
													this is a pointer to the ccl,
													else an identifying name.

					int			type				Symbol type
					int			atts				Symbol attributes
					BOOLEAN		create				Create symbol if it does not exist!
	
	Returns:		SYMBOL*							Pointer to the SYMBOL
													structure representing the
													symbol.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	26.03.2008	Jan Max Meyer	Distinguish between the different symbol types
								even in the hash-table, parameters added for
								error symbol
	11.11.2009	Jan Max Meyer	Changed name-parameter to dfn, to allow direct
								charclass-assignments to p_get_symbol() instead
								of a name that defines the charclass. Also,
								added trace macros.
----------------------------------------------------------------------------- */
SYMBOL* p_get_symbol( PARSER* p, void* dfn, int type, BOOLEAN create )
{
	uchar		keych;
	uchar*		keyname;
	uchar*		name		= (uchar*)dfn;
	SYMBOL*		sym			= (SYMBOL*)NULL;
	HASHELEM*	he;

	PROC( "p_get_symbol" );
	PARMS( "p", "%p", p );
	PARMS( "dfn", "%p", dfn );
	PARMS( "type", "%d", type );
	PARMS( "create", "%d", create );

	/* In case of a CCL-terminal, generate the name from the CCL */
	if( type == SYM_CCL_TERMINAL )
	{
		MSG( "SYM_CCL_TERMINAL detected - converting character class" );
		if( !( name = ccl_to_str( (CCL)dfn, TRUE ) ) )
		{
			OUTOFMEM;
			RETURN( (SYMBOL*)NULL );
		}

		VARS( "name", "%s", name );
	}

	/*
		To distinguish between the different types,
		a special identification character is prefixed to
		the hashtable-keys
	*/
	switch( type )
	{
		case SYM_CCL_TERMINAL:
			MSG( "type is SYM_CCL_TERMINAL" );
			keych = '#';
			break;

		case SYM_REGEX_TERMINAL:
			MSG( "type is SYM_REGEX_TERMINAL" );
			keych = '@';
			break;

		case SYM_NON_TERMINAL:
			MSG( "type is SYM_NON_TERMINAL" );
			keych = '!';
			break;

		case SYM_ERROR_RESYNC:
			MSG( "type is SYM_ERROR_RESYNC" );
			keych = '~';
			break;

		default:
			MSG( "type is something else?" );
			RETURN( (SYMBOL*)NULL );
	}

	if( !( keyname = (uchar*)p_malloc(
			( pstrlen( name ) + 1 + 1 )
				* sizeof( uchar ) ) ) )
	{
		OUTOFMEM;
		return (SYMBOL*)NULL;
	}

	sprintf( keyname, "%c%s", keych, name );
	VARS( "keyname", "%s", keyname );

	if( !( he = hashtab_get( &( p->definitions ), keyname ) ) && create )
	{
		MSG( "Hash table not found - going to create entry" );

		if( ( sym = (SYMBOL*)p_malloc( sizeof( SYMBOL ) ) ) )
		{
			memset( sym, 0, sizeof( SYMBOL ) );

			/* Set up attributes */
			sym->id = list_count( p->symbols ); /* add 1 to here on errors... */
			sym->type = type;
			sym->line = -1;
			sym->nullable = FALSE;

			/* Initialize options table */
			hashtab_init( &( sym->options ), BUCKET_COUNT,
				HASHTAB_MOD_LIST | HASHTAB_MOD_EXTKEYS );

			/* Terminal symbols have always theirself in the FIRST-set... */
			if( IS_TERMINAL( sym ) )
				sym->first = list_push( sym->first, sym );
			
			/* Identifying name */
			if( type == SYM_CCL_TERMINAL )
			{
				sym->name = name;
				sym->ccl = (CCL)dfn;
			}
			else if( !( sym->name = pstrdup( name ) ) )
			{
				OUTOFMEM;
				RETURN( (SYMBOL*)NULL );
			}

			/* Insert pointer into hash-table */
			if( !( hashtab_insert( &( p->definitions ),
					keyname, (void*)sym ) ) )
			{
				OUTOFMEM;
				RETURN( (SYMBOL*)NULL );
			}
			
			/* Insert pointer into symbol list */
			p->symbols = list_push( p->symbols, sym );

			/* This is the error symbol */
			if( type == SYM_ERROR_RESYNC )
				p->error = sym;
		}
		else
		{
			OUTOFMEM;
			RETURN( (SYMBOL*)NULL );
		}
	}
	else if( he )
	{
		sym = (SYMBOL*)( he->data );

		if( type == SYM_CCL_TERMINAL )
			p_free( name );
	}

	p_free( keyname );
	RETURN( sym );
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_free_symbol()
	
	Author:			Jan Max Meyer
	
	Usage:			p_frees a symbol structure and all its members.
					
	Parameters:		SYMBOL*			sym				Symbol to be p_freed.
	
	Returns:		void

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	11.11.2009	Jan Max Meyer	Free charclass
	16.11.2009	Jan Max Meyer	Free NFA from new regex lib :)
----------------------------------------------------------------------------- */
void p_free_symbol( SYMBOL* sym )
{
	p_free( sym->code );
	p_free( sym->name );
	p_free( sym->ccl );
	list_free( sym->first );
	list_free( sym->productions );
	list_free( sym->all_sym );

	pregex_nfa_free( &( sym->nfa ) );

	hashtab_free( &( sym->options ), (HASHTAB_CALLBACK)p_free_opt );

	p_free( sym );
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_create_production()
	
	Author:			Jan Max Meyer
	
	Usage:			Creates a production for an according left-hand side and
					inserts it into the global list of productions.
					
	Parameters:		PARSER*			p				Parser information structure
					SYMBOL*			lhs				Pointer to the left-hand side
													the production belongs to.
													(SYMBOL*)NULL avoids a creation
													of a production.
	
	Returns:		PROD*							Pointer to the production,
													(PROD*)NULL in error case.

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
PROD* p_create_production( PARSER* p, SYMBOL* lhs )
{
	PROD*		prod		= (PROD*)NULL;
	
	if( p )
	{
		if( ( prod = (PROD*)p_malloc( sizeof( PROD ) ) ) )
		{
			memset( prod, 0, sizeof( PROD ) );

			prod->id = list_count( p->productions );
			
			/* Insert into production table */
			if( !( p->productions = list_push( p->productions, prod ) ) )
			{
				OUTOFMEM;
				p_free( prod );
				return (PROD*)NULL;
			}

			/* Initialize options table */
			hashtab_init( &( prod->options ), BUCKET_COUNT,
				HASHTAB_MOD_LIST | HASHTAB_MOD_EXTKEYS );

			/* Set up production attributes */
			if( lhs )
			{
				prod->lhs = lhs;
				prod->all_lhs = list_push( prod->all_lhs, (void*)lhs );
				
				lhs->productions = list_push( lhs->productions, prod );
				
				/* Add to left-hand side non-terminal */
				if( !( lhs->productions ) )
				{
					OUTOFMEM;
					p_free( prod );
					return (PROD*)NULL;
				}
			}
		}
		else
			OUTOFMEM;
	}

	return prod;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_append_to_production()
	
	Author:			Jan Max Meyer
	
	Usage:			Appends a symbol and a possible, semantic identifier to the
					right-hand side of a production.
					
	Parameters:		PROD*			p				Production
					SYMBOL*			sym				Symbol to append to production
					uchar*			name			Optional semantic identifier
													for the symbol on the right-hand
													side, can be (uchar*)NULL.
	
	Returns:		void

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	11.07.2010	Jan Max Meyer	Use name of derived symbol in case of
								virtual productions.
----------------------------------------------------------------------------- */
void p_append_to_production( PROD* p, SYMBOL* sym, uchar* name )
{
	if( p && sym )
	{
		if( !( p->rhs = list_push( p->rhs, sym ) ) )
			OUTOFMEM;
		
		/* If no name is given, then use the symbol's name, if it's a
			nonterminal or a regex-terminal. */
		if( !name )
		{
			while( sym->derived_from )
				sym = sym->derived_from;
				
			if( !( sym->generated ) &&
					( sym->type == SYM_NON_TERMINAL
						|| sym->type == SYM_REGEX_TERMINAL ) )
			{
				if( !( name = p_strdup( sym->name ) ) )
					OUTOFMEM;
			}
		}

		if( !( p->rhs_idents = list_push( p->rhs_idents, name ) ) )
			OUTOFMEM;
	}
}


/* -FUNCTION--------------------------------------------------------------------
	Function:		p_free_production()
	
	Author:			Jan Max Meyer
	
	Usage:			p_frees a production structure and all its members.
					
	Parameters:		PROD*			prod			Production to be p_freed.
	
	Returns:		void

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_free_production( PROD* prod )
{
	LIST*	li		= (LIST*)NULL;
	
	/* Real right-hand sides */
	for( li = prod->rhs_idents; li; li = li->next )
		p_free( li->pptr );

	list_free( prod->rhs_idents );
	list_free( prod->rhs );

	list_free( prod->all_lhs );
	
	/* Semantic right-hand sides */
	for( li = prod->sem_rhs_idents; li; li = li->next )
		p_free( li->pptr );

	list_free( prod->sem_rhs );
	list_free( prod->sem_rhs_idents );

	p_free( prod->code );

	hashtab_free( &( prod->options ), (HASHTAB_CALLBACK)p_free_opt );

	p_free( prod );
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_create_item()
	
	Author:			Jan Max Meyer
	
	Usage:			Creates a new state item to be used for performing the
					closure.
					
	Parameters:		STATE*		st					State-Pointer where to add
													the new item to.
					PROD*		p					Pointer to the production
													that should be associated by
													the item.
					LIST*		lookahead			A possible list of lookahead
													terminal symbol pointers.
													This can be filled later.
	
	Returns:		ITEM*							Pointer to the newly created
													item, (ITEM*)NULL in error
													case.

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
ITEM* p_create_item( STATE* st, PROD* p, LIST* lookahead )
{
	ITEM*		i		= (ITEM*)NULL;
	
	i = (ITEM*)p_malloc( sizeof( ITEM ) );
	if( i )
	{
		memset( i, 0, sizeof( ITEM ) );
	
		i->prod = p;
	
		if( p->rhs != (LIST*)NULL )
			i->next_symbol = p->rhs->pptr;

		if( st != (STATE*)NULL )
			st->kernel = list_push( st->kernel, i );
		
		if( lookahead != (LIST*)NULL )
			i->lookahead = list_dup( lookahead );
	}
	else
		OUTOFMEM;
	
	return i;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_free_item()
	
	Author:			Jan Max Meyer
	
	Usage:			Frees an item structure and all its members.
					
	Parameters:		ITEM*		it					Pointer to item structure to
													be freed.
	
	Returns:		void

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_free_item( ITEM* it )
{
	list_free( it->lookahead );
	p_free( it );
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_create_state()
	
	Author:			Jan Max Meyer
	
	Usage:			Creates a new state.
					
	Parameters:		PARSER*			p				Parser information structure,
													where the new state is added
													to.
	
	Returns:		STATE*							Pointer to the newly created
													state, (STATE*)NULL in error
													case.

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
STATE* p_create_state( PARSER* p )
{
	STATE*		st		= (STATE*)NULL;
	
	if( p )
	{
		st = (STATE*)p_malloc( sizeof( STATE ) );
		if( st )
		{
			memset( st, 0, sizeof( STATE ) );

			/* Set state unique key */
			st->state_id = list_count( p->lalr_states );

			/* Append to current parser states */
			p->lalr_states = list_push( p->lalr_states, st );
		}
		else
			OUTOFMEM;
	}
	return st;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_free_state()
	
	Author:			Jan Max Meyer
	
	Usage:			Frees a state structure and all its members.
					
	Parameters:		STATE*		st					Pointer to state structure to
													be freed.
	
	Returns:		void

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_free_state( STATE* st )
{
	LIST*	li		= (LIST*)NULL;

	for( li = st->kernel; li; li = li->next )
		p_free_item( li->pptr );

	for( li = st->epsilon; li; li = li->next )
		p_free_item( li->pptr );

	for( li = st->actions; li; li = li->next )
		p_free_tabcol( li->pptr );

	for( li = st->gotos; li; li = li->next )
		p_free_tabcol( li->pptr );

	list_free( st->kernel );
	list_free( st->epsilon );
	list_free( st->actions );
	list_free( st->gotos );

	p_free( st );
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_create_tabcol()
	
	Author:			Jan Max Meyer
	
	Usage:			Creates a table column to be added to a state's goto-table
					or action-table row.
					
	Parameters:		SYMBOL*		sym						Pointer to the symbol on
														which the desired action/
														goto is performed on
					int			action					The action to be performed
														in context of the symbol.
					int			idx						The index for the action.
														At SHIFT, this is the next
														state, at REDUCE and
														SHIFT_REDUCE the index of
														the rule to be reduced.
					ITEM*		item					The item, which caused
														the tab entry. This is
														only required for
														reductions.
	
	Returns:		TABCOL*								Pointer to the new
														action item. On error,
														(TABCOL*)NULL is returned.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */	
TABCOL* p_create_tabcol( SYMBOL* sym, short action, int idx, ITEM* item )
{
	TABCOL*		act		= (TABCOL*)NULL;
	
	act = (TABCOL*)p_malloc( sizeof( TABCOL ) );
	if( act )
	{
		memset( act, 0, sizeof( TABCOL ) );

		act->symbol = sym;
		act->action = action;
		act->index = idx;
		act->derived_from = item;
	}
	else
		OUTOFMEM;

	return act;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_free_tabcol()
	
	Author:			Jan Max Meyer
	
	Usage:			Frees an TABCOL-structure and all its members.
					
	Parameters:		TABCOL*		act						Pointer to action element
														to be freed.
	
	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */	
void p_free_tabcol( TABCOL* act )
{
	p_free( act );
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_find_tabcol()
	
	Author:			Jan Max Meyer
	
	Usage:			Tries to find the entry for a specified symbol within a state's
					action- or goto-table row.
					
	Parameters:		LIST*		row						The row where the entry
														should be found in.
					SYMBOL*		sym						Pointer to the symbol; if
														there is an entry on this
														symbol, it will be returned.
	
	Returns:		TABCOL*								Pointer to the action item.
														If no action item was found
														when searching on the row,
														(TABCOL*)NULL is returned.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */	
TABCOL* p_find_tabcol( LIST* row, SYMBOL* sym )
{
	TABCOL*		act		= (TABCOL*)NULL;
	
	for( ; row; row = row->next )
	{
		act = row->pptr;
		if( act->symbol == sym )
			return act;
	}
	
	return (TABCOL*)NULL;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_create_opt()
	
	Author:			Jan Max Meyer
	
	Usage:			Creates an option data structure and optionally inserts it
					into a hash-table. 
					
	Parameters:		HASHTAB*	ht				Hash-table (optional). If this
												is (HASHTAB*)NULL, only a
												pointer to the new option
												structure will be returned.
					uchar*		opt				Identifiying option name
					uchar*		def				Option definition; Can be
												left (uchar*)NULL.
	
	Returns:		OPT*						Pointer to the newly created
												OPT-structure, (OPT*)NULL
												in error case.

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
OPT* p_create_opt( HASHTAB* ht, uchar* opt, uchar* def )
{
	OPT*		option;

	if( !( option = (OPT*)p_malloc( sizeof( OPT ) ) ) )
	{
		OUTOFMEM;
		return (OPT*)NULL;
	}

	memset( option, 0, sizeof( OPT ) );

	if( !( option->opt = p_strdup( opt ) ) )
	{
		p_free( option );

		OUTOFMEM;
		return (OPT*)NULL;
	}

	option->def = p_strdup( def );

	if( ht && !hashtab_insert( ht, option->opt, (void*)option ) )
	{
		p_free_opt( option );

		OUTOFMEM;
		return (OPT*)NULL;
	}

	return option;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_free_opt()
	
	Author:			Jan Max Meyer
	
	Usage:			Frees an option's memory content.
					
	Parameters:		OPT*	option 				Pointer to option to be freed.
	
	Returns:		void

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_free_opt( OPT* option )
{
	p_free( option->opt );
	p_free( option->def );

	p_free( option );
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_create_parser()
	
	Author:			Jan Max Meyer
	
	Usage:			Allocates and initializes a new parser information strucure.
					
	Parameters:		void
	
	Returns:		PARSER*						Pointer to the newly created
												PARSER-structure, (PARSER*)NULL
												in error case.

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
PARSER* p_create_parser( void )
{
	PARSER*		pptr	= (PARSER*)NULL;

	if( !( pptr = p_malloc( sizeof( PARSER ) ) ) )
	{
		OUTOFMEM;
		return (PARSER*)NULL;
	}

	memset( pptr, 0, sizeof( PARSER ) );

	/* Initialize the hash table for fast symbol access */
	hashtab_init( &( pptr->definitions ), BUCKET_COUNT, FALSE );

	/* Setup defaults */
	pptr->p_mode = MODE_SENSITIVE;
	pptr->p_universe = CCL_MAX;
	pptr->optimize_states = TRUE;
	pptr->gen_prog = TRUE;

	/* Initialize options table */
	hashtab_init( &( pptr->options ), BUCKET_COUNT,
		HASHTAB_MOD_LIST | HASHTAB_MOD_EXTKEYS );

	return pptr;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_free_parser()
	
	Author:			Jan Max Meyer
	
	Usage:			Frees a parser structure and all its members.
					
	Parameters:		PARSER*		parser			Parser structure to be freed.
	
	Returns:		void

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_free_parser( PARSER* parser )
{
	LIST*		it			= (LIST*)NULL;
	pregex_dfa*	dfa;

	hashtab_free( &( parser->definitions ), (void*)NULL );

	for( it = parser->symbols; it; it = it->next )
		p_free_symbol( it->pptr );

	for( it = parser->productions; it; it = it->next )
		p_free_production( it->pptr );

	for( it = parser->lalr_states; it; it = it->next )
		p_free_state( it->pptr );

	for( it = parser->vtypes; it; it = it->next )
		p_free_vtype( it->pptr );

	for( it = parser->kw; it; it = it->next )
	{
		dfa = (pregex_dfa*)list_access( it );
		pregex_dfa_free( dfa );
		p_free( dfa );
	}

	list_free( parser->symbols );
	list_free( parser->productions );
	list_free( parser->lalr_states );
	list_free( parser->vtypes );
	list_free( parser->kw );

	p_free( parser->p_name );
	p_free( parser->p_desc );
	p_free( parser->p_language );
	p_free( parser->p_copyright );
	p_free( parser->p_version );
	p_free( parser->p_prefix );
	p_free( parser->p_header );
	p_free( parser->p_footer );
	p_free( parser->p_pcb );

	p_free( parser->p_def_action );
	p_free( parser->p_def_action_e );
	
	p_free( parser->source );

	hashtab_free( &( parser->options ), (HASHTAB_CALLBACK)p_free_opt );

	xml_free( parser->err_xml );

	p_free( parser );
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_find_vtype()
	
	Author:			Jan Max Meyer
	
	Usage:			Seaches for a value stack type.
										
	Parameters:		PARSER*		p					Parser information structure
					uchar*		name				Value type name
	
	Returns:		VTYPE*							Pointer to the VTYPE structure
													representing the value type,
													if a matching type has been
													found.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
VTYPE* p_find_vtype( PARSER* p, uchar* name )
{
	VTYPE*	vt;
	LIST*	l;
	uchar*	test_name;

	test_name = p_strdup( name );
	if( !test_name )
		OUTOFMEM;

	p_str_no_whitespace( test_name );

	for( l = p->vtypes; l; l = l->next )
	{
		vt = (VTYPE*)(l->pptr);
		if( !strcmp( vt->int_name, test_name ) )
		{
			p_free( test_name );
			return vt;
		}
	}

	p_free( test_name );
	return (VTYPE*)NULL;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_create_vtype()
	
	Author:			Jan Max Meyer
	
	Usage:			Creates a new value stack type, or returns an existing one,
					if it does already exist.
										
	Parameters:		PARSER*		p					Parser information structure
					uchar*		name				Value type name
	
	Returns:		VTYPE*							Pointer to the VTYPE structure
													representing the value type.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
VTYPE* p_create_vtype( PARSER* p, uchar* name )
{
	VTYPE*	vt;

	if( !( vt = p_find_vtype( p, name ) ) )
	{
		vt = (VTYPE*)p_malloc( sizeof( VTYPE ) );
		if( !vt )
			OUTOFMEM;

		vt->id = list_count( p->vtypes );
		
		vt->int_name = p_strdup( name );
		if( !( vt->int_name ) )
			OUTOFMEM;

		p_str_no_whitespace( vt->int_name );

		vt->real_def = p_strdup( name );
		if( !( vt->real_def ) )
			OUTOFMEM;

		/*
		printf( "Adding vtype >%s< >%s< and >%s<\n",
			name, vt->int_name, vt->real_def );
		*/

		p->vtypes = list_push( p->vtypes, (void*)vt );
	}

	return vt;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_free_vtype()
	
	Author:			Jan Max Meyer
	
	Usage:			Frees a VTYPE-structure.
										
	Parameters:		VTYPE*	vt					VTYPE-Structure to be deleted.
	
	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_free_vtype( VTYPE* vt )
{
	p_free( vt->int_name );
	p_free( vt->real_def );
	p_free( vt );
}

