/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator 
Copyright (C) 2006-2009 by Phorward Software Technologies, Jan Max Meyer
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
					uchar*		name				Symbol name
					int			type				Symbol type
					int			atts				Symbol attributes
					BOOLEAN		create				Create symbol if it does not exist!
	
	Returns:		SYMBOL*							Pointer to the SYMBOL structure
													representing the symbol.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	26.03.2008	Jan Max Meyer	Distinguish between the different symbol types
								even in the hash-table, parameters added for
								error symbol
----------------------------------------------------------------------------- */
SYMBOL* p_get_symbol( PARSER* p, uchar* name, int type, BOOLEAN create )
{
	uchar		keych;
	uchar*		keyname;
	SYMBOL*		sym			= (SYMBOL*)NULL;
	HASHELEM*	he;

	/*
		To distinguish between the different types,
		a special identification character is prefixed to
		the hashtable-keys
	*/
	switch( type )
	{
		case SYM_CCL_TERMINAL:
			keych = '#';
			break;

		case SYM_KW_TERMINAL:
			keych = '$';
			break;

		case SYM_REGEX_TERMINAL:
			keych = '@';
			break;

		case SYM_EXTERN_TERMINAL:
			keych = '*';
			break;

		case SYM_NON_TERMINAL:
			keych = '!';
			break;

		case SYM_ERROR_RESYNC:
			keych = '~';
			break;

		default:
			return (SYMBOL*)NULL;
	}

	keyname = (uchar*)p_malloc( ( strlen( name ) + 1 + 1 ) * sizeof( uchar ) );
	if( !keyname )
	{
		OUT_OF_MEMORY;
		return (SYMBOL*)NULL;
	}

	sprintf( keyname, "%c%s", keych, name );

	if( !( he = hashtab_get( p->definitions, keyname ) ) && create )
	{	
		sym = (SYMBOL*)p_malloc( sizeof( SYMBOL ) );

		if( sym )
		{
			memset( sym, 0, sizeof( SYMBOL ) );

			/* Set up attributes */
			sym->id = list_count( p->symbols ); /* add 1 to here on errors... */
			sym->type = type;
			sym->line = -1;
			sym->nullable = FALSE;

			/* Terminal symbols have always theirself in the FIRST-set... */
			if( IS_TERMINAL( sym ) )
				sym->first = list_push( sym->first, sym );
			
			sym->name = strdup( name );
			if( !( sym->name ) )
			{
				OUT_OF_MEMORY;
				return (SYMBOL*)NULL;
			}

			/* Insert pointer into hash-table */
			if( !( hashtab_insert( p->definitions, keyname, (void*)sym ) ) )
			{
				OUT_OF_MEMORY;
				return (SYMBOL*)NULL;
			}
			
			/* Insert pointer into symbol list */
			p->symbols = list_push( p->symbols, sym );

			/* This is the error symbol */
			if( type == SYM_ERROR_RESYNC )
			{
				p->error = sym;
				sym->keyword = TRUE;
			}
		}
		else
			OUT_OF_MEMORY;
	}
	else if( he )
		sym = (SYMBOL*)( he->data );

	p_free( keyname );
	return sym;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_free_symbol()
	
	Author:			Jan Max Meyer
	
	Usage:			p_frees a symbol structure and all its members.
					
	Parameters:		SYMBOL*			sym				Symbol to be p_freed.
	
	Returns:		void

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_free_symbol( SYMBOL* sym )
{
	p_free( sym->name );
	list_free( sym->first );
	list_free( sym->productions );
	p_free( sym->char_map );

	if( sym->nfa_def )
		re_free_nfa_table( sym->nfa_def );

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
		prod = (PROD*)p_malloc( sizeof( PROD ) );
		if( prod )
		{
			memset( prod, 0, sizeof( PROD ) );

			/* Set up production attributes */
			prod->lhs = lhs;
			prod->id = list_count( p->productions );
			
			/* Insert into production table */
			p->productions = list_push( p->productions, prod );
			
			if( !( p->productions ) )
			{
				OUT_OF_MEMORY;
				p_free( prod );
				return (PROD*)NULL;
			}
			
			/* Add to left-hand side non-terminal */
			if( lhs )
			{
				lhs->productions = list_push( lhs->productions, prod );
				if( !( lhs->productions ) )
				{
					OUT_OF_MEMORY;
					p_free( prod );
					return (PROD*)NULL;
				}
			}
		}
		else
			OUT_OF_MEMORY;
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
----------------------------------------------------------------------------- */
void p_append_to_production( PROD* p, SYMBOL* sym, uchar* name )
{
	if( p && sym )
	{
		p->rhs = list_push( p->rhs, sym );
		
		/* If no name is given, then use the symbol's name, if it's a
			nonterminal or a regex-terminal. */
		if( !name )
		{
			if( !( sym->generated ) &&
					sym->type == SYM_NON_TERMINAL
						|| sym->type == SYM_REGEX_TERMINAL )
			{
				name = p_strdup( sym->name );
			}
		}

		p->rhs_idents = list_push( p->rhs_idents, name );
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

	for( li = prod->rhs_idents; li; li = li->next )
		p_free( li->pptr );

	list_free( prod->rhs_idents );
	list_free( prod->rhs );
	p_free( prod->code );

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
		OUT_OF_MEMORY;
	
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
			OUT_OF_MEMORY;
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
	
	Returns:		TABCOL*								Pointer to the new
														action item. On error,
														(TABCOL*)NULL is returned.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */	
TABCOL* p_create_tabcol( SYMBOL* sym, short action, int idx )
{
	TABCOL*		act		= (TABCOL*)NULL;
	
	act = (TABCOL*)p_malloc( sizeof( TABCOL ) );
	if( act )
	{
		memset( act, 0, sizeof( TABCOL ) );

		act->symbol = sym;
		act->action = action;
		act->index = idx;
	}
	else
		OUT_OF_MEMORY;

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
	Function:		p_create_parser()
	
	Author:			Jan Max Meyer
	
	Usage:			Allocates and initializes a new parser information strucure.
					
	Parameters:		void
	
	Returns:		PARSER*							Pointer to the newly created
													PARSER-structure, (PARSER*)NULL
													in error case.

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
PARSER* p_create_parser( void )
{
	PARSER*		pptr	= (PARSER*)NULL;

	pptr = p_malloc( sizeof( PARSER ) );
	if( !pptr )
	{
		OUT_OF_MEMORY;
		return (PARSER*)NULL;
	}

	memset( pptr, 0, sizeof( PARSER ) );

	/* Creating the goal symbol */
	/* eos = p_get_symbol( pptr, SYM_TERMINAL, END_OF_STRING_SYMBOL ); */

	/* Initialize the hash table for fast symbol access */
	pptr->definitions = hashtab_create( BUCKET_COUNT, FALSE );
	if( !( pptr->definitions ) )
	{
		OUT_OF_MEMORY;
		p_free( pptr );
	}

	/* Setup defaults */
	pptr->p_model = MODEL_CONTEXT_SENSITIVE;
	pptr->p_universe = 255;
	pptr->optimize_states = TRUE;

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
	LIST*		dfa_list;

	hashtab_free( parser->definitions, (void*)NULL );

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
		dfa_list = (LIST*)( it->pptr );
		re_free_dfa( &dfa_list );
	}

	list_free( parser->symbols );
	list_free( parser->productions );
	list_free( parser->lalr_states );
	list_free( parser->vtypes );
	list_free( parser->kw );

	if( parser->nfa_m )
		re_free_nfa_table( parser->nfa_m );

	p_free( parser->p_name );
	p_free( parser->p_desc );
	p_free( parser->p_language );
	p_free( parser->p_copyright );
	p_free( parser->p_version );
	p_free( parser->p_prefix );
	p_free( parser->p_header );
	p_free( parser->p_footer );

	p_free( parser->p_def_action );
	p_free( parser->p_def_action_e );
	p_free( parser->p_invalid_suf );
	
	p_free( parser->source );

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
		OUT_OF_MEMORY;

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
			OUT_OF_MEMORY;

		vt->id = list_count( p->vtypes );
		
		vt->int_name = p_strdup( name );
		if( !( vt->int_name ) )
			OUT_OF_MEMORY;

		p_str_no_whitespace( vt->int_name );

		vt->real_def = p_strdup( name );
		if( !( vt->real_def ) )
			OUT_OF_MEMORY;

		/*
		printf( "Adding vtype >%s< >%s< and >%s<\n", name, vt->int_name, vt->real_def );
		*/

		p->vtypes = list_push( p->vtypes, (void*)vt );
	}

	return vt;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_free_vtype()
	
	Author:			Jan Max Meyer
	
	Usage:			Frees a VTYPE-structure.
										
	Parameters:		VTYPE*	vt						VTYPE-Structure to be deleted.
	
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

