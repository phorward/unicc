/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2017 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com ++ unicc<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	mem.c
Author:	Jan Max Meyer
Usage:	Memory maintenance and management functions for several datatypes used
		in UniCC for symbol and state management.
----------------------------------------------------------------------------- */

#include "unicc.h"

/** Creates a new grammar symbol, or returns the grammar symbol, if it already
exists in the symbol table. A grammar symbol can either be a terminal symbol or
a non-terminal symbol.

//p// is the parser information structure
//dfn// is the symbol definition; in case of a charclass terminal, this is a
pointer to the ccl, else an identifying name.
//type// is the symbol type.
//atts// is the symbol attributes.
//create// defines, if TRUE, create symbol if it does not exist!

Returns a SYMBOL*-pointer to the SYMBOL structure representing the symbol. */
SYMBOL* get_symbol( PARSER* p, void* dfn, int type, BOOLEAN create )
{
	char*		keyname;
	char		keych;
	char*		name		= (char*)dfn;
	SYMBOL*		sym			= (SYMBOL*)NULL;
	plistel*	e;

	/*
	26.03.2008	Jan Max Meyer
	Distinguish between the different symbol types even in the hash-table,
	parameters added for error symbol.

	11.11.2009	Jan Max Meyer
	Changed name-parameter to dfn, to allow direct charclass-assignments to
	get_symbol() instead of a name that defines the charclass.
	Also, added trace macros.
	*/

	PROC( "get_symbol" );
	PARMS( "p", "%p", p );
	PARMS( "dfn", "%p", dfn );
	PARMS( "type", "%d", type );
	PARMS( "create", "%d", create );

	/* In case of a ccl-terminal, generate the name from the ccl */
	if( type == SYM_CCL_TERMINAL )
	{
		MSG( "SYM_CCL_TERMINAL detected - converting character class" );
		name = p_ccl_to_str( (pccl*)dfn, TRUE );

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

		case SYM_SYSTEM_TERMINAL:
			MSG( "type is a system terminal" );
			keych = '~';
			break;

		default:
			MSG( "type is something else?" );
			RETURN( (SYMBOL*)NULL );
	}

	if( !( keyname = (char*)pmalloc(
			( pstrlen( name ) + 1 + 1 )
				* sizeof( char ) ) ) )
	{
		OUTOFMEM;
		return (SYMBOL*)NULL;
	}

	sprintf( keyname, "%c%s", keych, name );
	VARS( "keyname", "%s", keyname );

	if( !( e = plist_get_by_key( p->symbols, keyname ) ) && create )
	{
		/* Create symbol */
		if( !( sym = (SYMBOL*)plist_access( plist_insert( p->symbols,
										(plistel*)NULL, keyname,
											(void*)NULL ) ) ) )
			RETURN( sym );

		/* Set up attributes */
		sym->id = plist_count( p->symbols ) - 1;
		sym->type = type;
		sym->line = -1;
		sym->nullable = FALSE;
		sym->greedy = TRUE;

		sym->first = plist_create( 0, PLIST_MOD_PTR );

		/* Terminal symbols have always theirself in the FIRST-set... */
		if( IS_TERMINAL( sym ) )
		{
			plist_push( sym->first, sym );
			sym->all_sym = plist_create( 0, PLIST_MOD_PTR );
		}
		else
			sym->productions = plist_create( 0, PLIST_MOD_PTR );

		/* Initialize options table */
		sym->options = plist_create( sizeof( OPT ), PLIST_MOD_EXTKEYS );

		/* Identifying name */
		if( type == SYM_CCL_TERMINAL )
			sym->ccl = (pccl*)dfn;

		sym->keyname = keyname;
		keyname = (char*)NULL;

		if( !( sym->name = pstrdup( name ) ) )
		{
			OUTOFMEM;
			RETURN( (SYMBOL*)NULL );
		}

		/* System terminals are linked to the parser object */
		if( type == SYM_SYSTEM_TERMINAL )
		{
			sym->used = TRUE;
			sym->defined = TRUE;

			if( strcmp( sym->name, P_ERROR_RESYNC ) == 0 )
				p->error = sym;
			else if( strcmp( sym->name, P_END_OF_FILE ) == 0 )
				p->end_of_input = sym;
		}
	}
	else if( e )
		sym = (SYMBOL*)plist_access( e );

	pfree( keyname );
	RETURN( sym );
}

/** Frees a symbol structure and all its members.

//sym// is the symbol to be freed. */
void free_symbol( SYMBOL* sym )
{
	pfree( sym->code );
	pfree( sym->name );

	if( sym->ptn )
		pregex_ptn_free( sym->ptn );
	else
		sym->ccl = p_ccl_free( sym->ccl );

	plist_free( sym->first );
	plist_free( sym->productions );
	plist_free( sym->all_sym );

	sym->options = free_opts( sym->options );
}

/** Creates a production for an according left-hand side and inserts it into the
global list of productions.

//p// is the parser information structure.
//lhs// is the pointer to the left-hand side the production belongs to.
(SYMBOL*)NULL avoids a creation of a production.

Returns a PROD*-pointer to the production, or (PROD*)NULL in error case.
*/
PROD* create_production( PARSER* p, SYMBOL* lhs )
{
	PROD*		prod		= (PROD*)NULL;

	if( !p )
		return (PROD*)NULL;

	prod = (PROD*)plist_malloc( p->productions );
	prod->id = plist_count( p->productions ) - 1;

	prod->rhs = plist_create( 0, PLIST_MOD_PTR );
	prod->sem_rhs = plist_create( 0, PLIST_MOD_PTR );
	prod->all_lhs = plist_create( 0, PLIST_MOD_PTR );

	/* Initialize options table */
	prod->options = plist_create( sizeof( OPT ), PLIST_MOD_EXTKEYS );

	/* Set up production attributes */
	if( lhs )
	{
		prod->lhs = lhs;

		plist_push( prod->all_lhs, lhs );
		plist_push( lhs->productions, prod );
	}

	return prod;
}

/** Appends a symbol and a possible, semantic identifier to the right-hand side
of a production.

//p// is the production to be extended.
//sym// is the symbol to append to production.
//name// is the optional semantic identifier for the symbol on the right-hand
side, can be (char*)NULL.
*/
void append_to_production( PROD* p, SYMBOL* sym, char* name )
{
	SYMBOL*		deriv	= sym;

	if( !( p && sym ) )
	{
		WRONGPARAM;
		return;
	}

	/* If no name is given, then use the symbol's name, if it's a
		nonterminal or a regex-terminal. */
	if( !name )
	{
		/*
		11.07.2010	Jan Max Meyer
		Use name of derived symbol in case of virtual productions.
		*/
		while( deriv->derived_from )
			deriv = deriv->derived_from;

		if( !( deriv->generated ) &&
				( deriv->type == SYM_NON_TERMINAL
					|| deriv->type == SYM_REGEX_TERMINAL ) )
			name = deriv->name;
	}

	plist_insert( p->rhs, (plistel*)NULL, name, sym );
}

/** Frees a production structure and all its members.

//prod// is the production to be freed. */
void free_production( PROD* prod )
{
	plist_free( prod->rhs );
	plist_free( prod->sem_rhs );
	plist_free( prod->all_lhs );

	pfree( prod->code );

	prod->options = free_opts( prod->options );
}


/** Creates a new state item to be used for performing the closure.

//st// is the state-pointer where to add the new item to.

//p// is the pointer to the production that should be associated by the item.

//lookahead// is a possible list of lookahead terminal symbol pointers
This can also be filled later.

Returns an ITEM*-pointer to the newly created item, (ITEM*)NULL in error case.
*/
ITEM* create_item( STATE* st, PROD* p )
{
	ITEM*		i		= (ITEM*)NULL;

	i = (ITEM*)pmalloc( sizeof( ITEM ) );
	if( i )
	{
		i->prod = p;
		i->next_symbol = (SYMBOL*)plist_access( plist_first( p->rhs ) );
		i->lookahead = plist_create( 0, PLIST_MOD_PTR );

		if( st != (STATE*)NULL )
			st->kernel = list_push( st->kernel, i );
	}
	else
		OUTOFMEM;

	return i;
}

/** Frees an item structure and all its members.

//it// is the pointer to item structure to be freed. */
void free_item( ITEM* it )
{
	plist_free( it->lookahead );
	pfree( it );
}

/** Creates a new state.

//p// is the parser information structure where the new state is added to.

Returns a STATE* Pointer to the newly created state, (STATE*)NULL in error case.
*/
STATE* create_state( PARSER* p )
{
	STATE*		st;

	if( !p )
	{
		WRONGPARAM;
		return (STATE*)NULL;
	}

	st = (STATE*)parray_malloc( p->states );

	/* Set state unique key */
	st->state_id = parray_count( p->states ) - 1;

	return st;
}

/** Frees a state structure and all its members.

//st// is the Pointer to state structure to be freed. */
void free_state( STATE* st )
{
	LIST*	li;

	for( li = st->kernel; li; li = li->next )
		free_item( li->pptr );

	for( li = st->epsilon; li; li = li->next )
		free_item( li->pptr );

	for( li = st->actions; li; li = li->next )
		free_tabcol( li->pptr );

	for( li = st->gotos; li; li = li->next )
		free_tabcol( li->pptr );

	list_free( st->kernel );
	list_free( st->epsilon );
	list_free( st->actions );
	list_free( st->gotos );
}

/** Creates a table column to be added to a state's goto-table or action-table
row.

//sym// is the pointer to the symbol on which the desired action or goto is
performed on.
//action// is the action to be performed in context of the symbol.
//idx// is the index for the action. At SHIFT, this is the next state, at REDUCE
and SHIFT_REDUCE the index of the rule to be reduced.
//item// is the item, which caused the tab entry. This is only required for
reductions.

Returns a TABCOL* Pointer to the new action item. On error, (TABCOL*)NULL is
returned. */
TABCOL* create_tabcol( SYMBOL* sym, short action, int idx, ITEM* item )
{
	TABCOL*		act		= (TABCOL*)NULL;

	act = (TABCOL*)pmalloc( sizeof( TABCOL ) );
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

/** Frees an TABCOL-structure and all its members.

//act// is the pointer to action element to be freed.
*/
void free_tabcol( TABCOL* act )
{
	pfree( act );
}

/** Tries to find the entry for a specified symbol within a state's action- or
goto-table row.

//row// is the row where the entry should be found in.
//sym// is the pointer to the symbol; if there is an entry on this symbol, it
will be returned.

Returns a TABCOL* Pointer to the action item. If no action item was found when
searching on the row, (TABCOL*)NULL is returned.
*/
TABCOL* find_tabcol( LIST* row, SYMBOL* sym )
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

/** Creates an option data structure and optionally inserts it into a
hash-table.

//opts// is the hash-table of options.
//opt// is the identifiying option name
//def// is the option definition; Can be left (char*)NULL.

Returns a OPT* Pointer to the newly created OPT-structure, (OPT*)NULL in error
case. */
OPT* create_opt( plist* options, char* opt, char* def )
{
	OPT			option;
	plistel*	e;

	memset( &option, 0, sizeof( OPT ) );

	option.opt = pstrdup( opt );
	option.def = pstrdup( def );

	if( !( e = plist_insert( options,
					(plistel*)NULL, option.opt, (void*)&option ) ) )
	{
		pfree( option.opt );
		pfree( option.def );

		OUTOFMEM;
		return (OPT*)NULL;
	}

	return (OPT*)plist_access( e );
}

/** Frees the entire options list and its options.

//options// is the options list

Always returns (plist*)NULL */
plist* free_opts( plist* options )
{
	OPT		opt;

	if( !options )
		return (plist*)NULL;

	while( plist_pop( options, (void*)&opt ) )
	{
		pfree( opt.opt );
		pfree( opt.def );
	}

	return plist_free( options );
}


static int sort_symbols( plist* lst, plistel* el, plistel* er )
{
	SYMBOL*		l	= (SYMBOL*)plist_access( el );
	SYMBOL*		r	= (SYMBOL*)plist_access( er );

	if( l->type > r->type )
		return 1;
	else if( l->type < r->type )
		return -1;

	if( l->type == SYM_REGEX_TERMINAL && r->type == SYM_REGEX_TERMINAL )
	{
		if( l->keyword > r->keyword )
			return 1;
		else if( l->keyword < r->keyword )
			return -1;
	}

	return 0;
}

/** Allocates and initializes a new parser information structure.

Returns a PARSER* Pointer to the newly created PARSER-structure,
(PARSER*)NULL in error case.
*/
PARSER* create_parser( void )
{
	PARSER*		pptr	= (PARSER*)NULL;

	if( !( pptr = pmalloc( sizeof( PARSER ) ) ) )
	{
		OUTOFMEM;
		return (PARSER*)NULL;
	}

	memset( pptr, 0, sizeof( PARSER ) );

	/* Initialize any dynamic lists and arrays */
	pptr->symbols = plist_create( sizeof( SYMBOL ),
						PLIST_MOD_EXTKEYS | PLIST_MOD_UNIQUE );
	plist_set_sortfn( pptr->symbols, sort_symbols );

	pptr->productions = plist_create( sizeof( PROD ), PLIST_MOD_NONE );

	pptr->states = parray_create( sizeof( STATE ), 32 );

	/* Setup defaults */
	pptr->p_mode = MODE_SENSITIVE;
	pptr->p_universe = PCCL_MAX;
	pptr->optimize_states = TRUE;
	pptr->gen_prog = TRUE;

	/* Initialize options table */
	pptr->options = plist_create( sizeof( OPT ), PLIST_MOD_EXTKEYS );

	/* End of Input symbol must exist in every parser! */
	get_symbol( pptr, P_END_OF_FILE, SYM_SYSTEM_TERMINAL, TRUE );

	return pptr;
}

/** Frees a parser structure and all its members.

//parser// is the Parser structure to be freed. */
void free_parser( PARSER* parser )
{
	LIST*		it			= (LIST*)NULL;
	pregex_dfa*	dfa;

	for( it = parser->vtypes; it; it = it->next )
		free_vtype( it->pptr );

	for( it = parser->dfas; it; it = it->next )
	{
		dfa = (pregex_dfa*)list_access( it );
		pregex_dfa_free( dfa );
	}

	plist_iter_access( parser->symbols, (plistfn)free_symbol );
	plist_free( parser->symbols );

	/* plist_iterate( parser->productions, free_production ); */
	plist_free( parser->productions );

	parray_iter( parser->states, (parrayfn)&free_state );
	parray_free( parser->states );

	list_free( parser->vtypes );
	list_free( parser->dfas );

	pfree( parser->p_name );
	pfree( parser->p_desc );
	pfree( parser->p_template );
	pfree( parser->p_copyright );
	pfree( parser->p_version );
	pfree( parser->p_prefix );
	pfree( parser->p_header );
	pfree( parser->p_footer );
	pfree( parser->p_pcb );

	pfree( parser->p_def_action );
	pfree( parser->p_def_action_e );

	pfree( parser->source );

	parser->options = free_opts( parser->options );

	xml_free( parser->err_xml );

	pfree( parser );
}

/** Seaches for a value stack type.

//p// is the parser information structure
//name// is the value type name

Returns a VTYPE* Pointer to the VTYPE structure representing the value type, if
a matching type has been found.
*/
VTYPE* find_vtype( PARSER* p, char* name )
{
	VTYPE*	vt;
	LIST*	l;
	char*	test_name;

	test_name = pstrdup( name );
	if( !test_name )
		OUTOFMEM;

	str_no_whitespace( test_name );

	for( l = p->vtypes; l; l = l->next )
	{
		vt = (VTYPE*)(l->pptr);
		if( !strcmp( vt->int_name, test_name ) )
		{
			pfree( test_name );
			return vt;
		}
	}

	pfree( test_name );
	return (VTYPE*)NULL;
}

/** Creates a new value stack type, or returns an existing one, if it does
already exists.

//p// is the parser information structure
//name// is the value type name.

Returns a VTYPE*-pointer to the VTYPE structure representing the value type. */
VTYPE* create_vtype( PARSER* p, char* name )
{
	VTYPE*	vt;

	if( !( vt = find_vtype( p, name ) ) )
	{
		vt = (VTYPE*)pmalloc( sizeof( VTYPE ) );
		if( !vt )
			OUTOFMEM;

		vt->id = list_count( p->vtypes );

		vt->int_name = pstrdup( name );
		if( !( vt->int_name ) )
			OUTOFMEM;

		str_no_whitespace( vt->int_name );

		vt->real_def = pstrdup( name );
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

/** Frees a VTYPE-structure.

//vt// is the VTYPE-Structure to be deleted. */
void free_vtype( VTYPE* vt )
{
	pfree( vt->int_name );
	pfree( vt->real_def );
	pfree( vt );
}

