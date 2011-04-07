/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator 
Copyright (C) 2006-2009 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ mail@phorward-software.com

File:	p_build.c
Author:	Jan Max Meyer
Usage:	Builds the target parser based on configurations from a
		parser definition file.
----------------------------------------------------------------------------- */

/*
 * Includes
 */
#include "p_global.h"
#include "p_error.h"
#include "p_proto.h"

/*
 * Defines
 */
#define	LEN_EXT		"_len"

/*
 * Global variables
 */

/*
 * Functions
 */

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_build_code()
	
	Author:			Jan Max Meyer
	
	Usage:			This is the main function for the code-generator.
					It first reads a target language generator, and then
					constructs code segments, which are finally pasted into
					the parser template (which is defined within the
					<driver>-tag of the generator file).

					Maybe the success of UniCC is even this feature, maybe not.
					I think, that most programming languages - ignoring
					Brainfuck - can be implemented with this concept!
					
	Parameters:		FILE*		stream				Stream, where the output
													file is written to.
					PARSER*		parser				Parser information structure
	
	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_build_code( FILE* stream, PARSER* parser )
{
	GENERATOR	generator;
	GENERATOR*	gen					= (GENERATOR*)NULL;
	uchar		xml_file			[ BUFSIZ + 1 ];
	uchar*		complete			= (uchar*)NULL;
	uchar*		all					= (uchar*)NULL;
	uchar*		action_table		= (uchar*)NULL;
	uchar*		action_table_row	= (uchar*)NULL;
	uchar*		goto_table			= (uchar*)NULL;
	uchar*		goto_table_row		= (uchar*)NULL;
	uchar*		prod_rhs_count		= (uchar*)NULL;
	uchar*		prod_lhs			= (uchar*)NULL;
	uchar*		def_prod			= (uchar*)NULL;
	uchar*		char_map			= (uchar*)NULL;
	uchar*		whitespaces			= (uchar*)NULL;
	uchar*		symbols				= (uchar*)NULL;
	uchar*		productions			= (uchar*)NULL;
	uchar*		production			= (uchar*)NULL;
	uchar*		dfa_select			= (uchar*)NULL;
	uchar*		dfa_idx				= (uchar*)NULL;
	uchar*		dfa_idx_row			= (uchar*)NULL;
	uchar*		dfa_char			= (uchar*)NULL;
	uchar*		dfa_trans			= (uchar*)NULL;
	uchar*		dfa_accept			= (uchar*)NULL;
	uchar*		dfa_accept_row		= (uchar*)NULL;
	uchar*		kw_invalid_suffix	= (uchar*)NULL;
	uchar*		type_def			= (uchar*)NULL;
	uchar*		actions				= (uchar*)NULL;
	uchar*		scan_actions		= (uchar*)NULL;
	uchar*		top_value			= (uchar*)NULL;
	uchar*		act					= (uchar*)NULL;
	uchar*		rhs_item			= (uchar*)NULL;

	int*		chr_sym;

	int			max_action			= 0;
	int			max_goto			= 0;
	int			max_dfa_idx			= 0;
	int			max_dfa_accept		= 0;
	int			kw_start			= 0;
	int			kw_count			= 0;
	int			whitespaces_count	= 0;
	int			max_symbol_name		= 0;
	int			column;
	int			row;
	bitset		ccl;
	LIST*		l;
	LIST*		m;
	LIST*		dfa;
	DFA*		dfa_elem;
	SYMBOL*		sym;
	STATE*		st;
	TABCOL*		col;
	PROD*		p;
	PROD*		goalprod;
	VTYPE*		vt;
	int			i;
	BOOLEAN		is_default_code;

	if( !stream )
		stream = stdout;

	gen = &generator;
	memset( gen, 0, sizeof( GENERATOR ) );

	sprintf( xml_file, "%s.xml", parser->p_language );
	if( !p_load_generator( gen, xml_file ) )
		return;

	/* Now that we have the generator, do some code generation-related
		integrity preparatories on the grammar */
	for( l = parser->symbols; l; l = l->next )
	{
		sym = (SYMBOL*)( l->pptr );

		if( sym->type == SYM_NON_TERMINAL && !( sym->vtype ) &&
			( gen->vstack_def_type && *( gen->vstack_def_type ) ) )
			sym->vtype = p_create_vtype( parser, gen->vstack_def_type );
		else if( IS_TERMINAL( sym ) && !( sym->keyword )
			&& !( sym->vtype ) && ( gen->vstack_term_type &&
				*( gen->vstack_term_type ) ) )
			sym->vtype = p_create_vtype( parser, gen->vstack_term_type );
	}

	/* Create piece of code for the value at the top of the value stack
		(e.g. used to store the next terminal character onto the value stack) */
	if( list_count( parser->vtypes ) == 1 )
		top_value = p_tpl_insert( gen->action_single,
			GEN_WILD_PREFIX "offset", p_int_to_str( 0 ), TRUE,
				(uchar*)NULL );
	else
	{
		vt = p_find_vtype( parser, gen->vstack_term_type );
		if( vt )
			top_value = p_tpl_insert( gen->action_union,
				GEN_WILD_PREFIX "offset", p_int_to_str( 0 ), TRUE,
					GEN_WILD_PREFIX "attribute", p_tpl_insert( gen->vstack_union_att,
						GEN_WILD_PREFIX "value-type-id", p_int_to_str( vt->id ), TRUE,
							(uchar*)NULL ), TRUE,
								(uchar*)NULL );
		else
			/* ERROR */
			;
	}

	/* Build action, goto and dfa_select tables */
	for( l = parser->lalr_states, i = 0; l; l = l->next, i++ )
	{
		st = (STATE*)(l->pptr);
		
		/* Action table */
		action_table_row = p_tpl_insert( gen->acttab.row_start,
				GEN_WILD_PREFIX "number-of-columns", p_int_to_str( list_count( st->actions ) ), TRUE,
					GEN_WILD_PREFIX "state-number", p_int_to_str( st->state_id ), TRUE,
						(uchar*)NULL );

		if( max_action < list_count( st->actions ) )
			max_action = list_count( st->actions );

		for( m = st->actions, column = 0, kw_count = 0; m; m = m->next, column++ )
		{
			col = (TABCOL*)(m->pptr);

			action_table_row = p_str_append( action_table_row,
				p_tpl_insert( gen->acttab.col,
					GEN_WILD_PREFIX "symbol", p_int_to_str( col->symbol->id ), TRUE,
						GEN_WILD_PREFIX "action", p_int_to_str( col->action ), TRUE,
							GEN_WILD_PREFIX "index", p_int_to_str( col->index ), TRUE,
								GEN_WILD_PREFIX "column", p_int_to_str( column ), TRUE,
									(uchar*)NULL ), TRUE );

			if( col->symbol->keyword )
				kw_count++;

			if( m->next )
				action_table_row = p_str_append( action_table_row,
					gen->acttab.col_sep, FALSE );
		}

		action_table_row = p_str_append( action_table_row,
				p_tpl_insert( gen->acttab.row_end,
					GEN_WILD_PREFIX "number-of-columns", p_int_to_str( list_count( st->actions ) ), TRUE,
						GEN_WILD_PREFIX "state-number", p_int_to_str( st->state_id ), TRUE,
							(uchar*)NULL ), TRUE );

		if( l->next )
			action_table_row = p_str_append( action_table_row,
				gen->acttab.row_sep, FALSE );

		action_table = p_str_append( action_table, action_table_row, TRUE );

		/* Goto table */
		goto_table_row = p_tpl_insert( gen->gotab.row_start,
				GEN_WILD_PREFIX "number-of-columns", p_int_to_str( list_count( st->gotos ) ), TRUE,
					GEN_WILD_PREFIX "state-number", p_int_to_str( st->state_id ), TRUE,
						(uchar*)NULL );

		if( max_goto < list_count( st->gotos ) )
			max_goto = list_count( st->gotos );

		for( m = st->gotos, column = 0; m; m = m->next, column++ )
		{
			col = (TABCOL*)(m->pptr);

			/*
			goto_table_row = p_str_append( goto_table_row,
				p_tpl_insert( gen->gotab.col,
					GEN_WILD_PREFIX "symbol", p_int_to_str( col->symbol->id ), TRUE,
						GEN_WILD_PREFIX "index", p_int_to_str( col->index ), TRUE,
							GEN_WILD_PREFIX "column", p_int_to_str( column ), TRUE,
								(uchar*)NULL ), TRUE );
			*/
			goto_table_row = p_str_append( goto_table_row,
				p_tpl_insert( gen->gotab.col,
					GEN_WILD_PREFIX "symbol", p_int_to_str( col->symbol->id ), TRUE,
						GEN_WILD_PREFIX "action", p_int_to_str( col->action ), TRUE,
							GEN_WILD_PREFIX "index", p_int_to_str( col->index ), TRUE,
								GEN_WILD_PREFIX "column", p_int_to_str( column ), TRUE,
									(uchar*)NULL ), TRUE );

			if( m->next )
				goto_table_row = p_str_append( goto_table_row,
					gen->gotab.col_sep, FALSE );
		}

		goto_table_row = p_str_append( goto_table_row,
				p_tpl_insert( gen->gotab.row_end,
					GEN_WILD_PREFIX "number-of-columns", p_int_to_str( list_count( st->actions ) ), TRUE,
						GEN_WILD_PREFIX "state-number", p_int_to_str( st->state_id ), TRUE,
							(uchar*)NULL ), TRUE );

		if( l->next )
			goto_table_row = p_str_append( goto_table_row,
				gen->gotab.row_sep, FALSE );

		goto_table = p_str_append( goto_table, goto_table_row, TRUE );

		/* Only in context-sensitive model */
		if( parser->p_model == MODEL_CONTEXT_SENSITIVE )
		{
			/* dfa machine selection */
			dfa_select = p_str_append( dfa_select,
				p_tpl_insert( gen->dfa_select.col,
					GEN_WILD_PREFIX "machine",
						p_int_to_str( list_find( parser->kw, st->dfa ) ), TRUE,
							(char*)NULL ), TRUE );

			if( l->next )
				dfa_select = p_str_append( dfa_select, gen->dfa_select.col_sep, FALSE );
		}
		
		/* Default production table */
		def_prod = p_str_append( def_prod,
				p_tpl_insert( gen->defprod.col,
					GEN_WILD_PREFIX "state-number", p_int_to_str( st->state_id ), TRUE,
					GEN_WILD_PREFIX "production-number", p_int_to_str(
							( ( st->def_prod ) ? st->def_prod->id : -1 ) ), TRUE,
						(uchar*)NULL ), TRUE );

		if( l->next )
			def_prod = p_str_append( def_prod, gen->defprod.col_sep, FALSE );
	}

	/* Production length and production left-hand side tables */
	for( l = parser->productions, row = 0; l; l = l->next, row++ )
	{
		p = (PROD*)l->pptr;

		prod_rhs_count = p_str_append( prod_rhs_count,
			p_tpl_insert( gen->prodlen.col,
				GEN_WILD_PREFIX "length-of-rhs", p_int_to_str( list_count( p->rhs ) ), TRUE,
					GEN_WILD_PREFIX "production-number", p_int_to_str( row ), TRUE,
						(uchar*)NULL ), TRUE );

		prod_lhs = p_str_append( prod_lhs,
			p_tpl_insert( gen->prodlhs.col,
				GEN_WILD_PREFIX "lhs", p_int_to_str( p->lhs->id ), TRUE,
					GEN_WILD_PREFIX "production-number", p_int_to_str( row ), TRUE,
						(uchar*)NULL ), TRUE );

		if( l->next )
		{
			prod_rhs_count = p_str_append( prod_rhs_count, gen->prodlen.col_sep, FALSE );
			prod_lhs = p_str_append( prod_lhs, gen->prodlhs.col_sep, FALSE );
		}
	}

	/* Lexical symbol recognition machine table composition */
	for( l = parser->kw, row = 0, column = 0; l; l = l->next, row++ )
	{
		dfa = (LIST*)(l->pptr);

		dfa_idx_row = p_tpl_insert( gen->dfa_idx.row_start,
				GEN_WILD_PREFIX "number-of-columns", p_int_to_str( list_count( dfa ) ), TRUE,
					GEN_WILD_PREFIX "state-number", p_int_to_str( row ), TRUE,
						(uchar*)NULL );

		dfa_accept_row = p_tpl_insert( gen->dfa_accept.row_start,
				GEN_WILD_PREFIX "number-of-columns", p_int_to_str( list_count( dfa ) ), TRUE,
					GEN_WILD_PREFIX "state-number", p_int_to_str( row ), TRUE,
						(uchar*)NULL );

		if( max_dfa_idx < list_count( dfa ) )
			max_dfa_accept = max_dfa_idx = list_count( dfa );

		for( m = dfa; m; m = m->next )
		{
			dfa_elem = (DFA*)(m->pptr);

			if( dfa_char && dfa_trans )
			{
				dfa_char = p_str_append( dfa_char, gen->dfa_char.col_sep, FALSE );
				dfa_trans = p_str_append( dfa_trans, gen->dfa_trans.col_sep, FALSE );
			}

			if( m != dfa )
			{
				dfa_idx_row = p_str_append( dfa_idx_row,
					gen->dfa_idx.col_sep, FALSE );
				dfa_accept_row = p_str_append( dfa_accept_row,
					gen->dfa_accept.col_sep, FALSE );
			}

			dfa_idx_row = p_str_append( dfa_idx_row,
				p_tpl_insert( gen->dfa_idx.col,
					GEN_WILD_PREFIX "index", p_int_to_str( column ), TRUE,
						(uchar*)NULL ), TRUE );
			dfa_accept_row = p_str_append( dfa_accept_row,
				p_tpl_insert( gen->dfa_accept.col,
					GEN_WILD_PREFIX "accept", p_int_to_str( dfa_elem->accept ), TRUE,
						(uchar*)NULL ), TRUE );

			for( i = 0; i < parser->p_universe; i++ )
			{
				if( dfa_elem->line[ i ] > -1 )
				{					
					dfa_char = p_str_append( dfa_char, p_tpl_insert(
						gen->dfa_char.col, GEN_WILD_PREFIX "code", p_int_to_str( i ),
							TRUE, (uchar*)NULL ), TRUE );
					dfa_trans = p_str_append( dfa_trans, p_tpl_insert(
						gen->dfa_trans.col, GEN_WILD_PREFIX "goto", p_int_to_str( dfa_elem->line[i] ),
							TRUE, (uchar*)NULL ), TRUE );

					column++;

					dfa_char = p_str_append( dfa_char, gen->dfa_char.col_sep, FALSE );
					dfa_trans = p_str_append( dfa_trans, gen->dfa_trans.col_sep, FALSE );
				}
			}

			dfa_char = p_str_append( dfa_char, p_tpl_insert(
				gen->dfa_char.col, GEN_WILD_PREFIX "code", p_int_to_str( -1 ),
					TRUE, (uchar*)NULL ), TRUE );
			dfa_trans = p_str_append( dfa_trans, p_tpl_insert(
				gen->dfa_trans.col, GEN_WILD_PREFIX "goto", p_int_to_str( -1 ),
					TRUE, (uchar*)NULL ), TRUE );

			column++;
		}

		dfa_idx_row = p_str_append( dfa_idx_row,
				p_tpl_insert( gen->dfa_idx.row_end,
					GEN_WILD_PREFIX "number-of-columns", p_int_to_str( list_count( dfa ) ), TRUE,
						GEN_WILD_PREFIX "state-number", p_int_to_str( row ), TRUE,
							(uchar*)NULL ), TRUE );
		dfa_accept_row = p_str_append( dfa_accept_row,
				p_tpl_insert( gen->dfa_accept.row_end,
					GEN_WILD_PREFIX "number-of-columns", p_int_to_str( list_count( dfa ) ), TRUE,
						GEN_WILD_PREFIX "state-number", p_int_to_str( row ), TRUE,
							(uchar*)NULL ), TRUE );

		if( l->next )
		{
			dfa_idx_row = p_str_append( dfa_idx_row,
				gen->dfa_idx.row_sep, FALSE );
			dfa_accept_row = p_str_append( dfa_accept_row,
				gen->dfa_accept.row_sep, FALSE );
		}

		dfa_idx = p_str_append( dfa_idx, dfa_idx_row, TRUE );
		dfa_accept = p_str_append( dfa_accept, dfa_accept_row, TRUE );
	}

	/* Map of invalid keyword suffix characters */

		/* This character map is reused ... */
	if( !( chr_sym = (int*)p_malloc( ( parser->p_universe + 1 ) * sizeof( int ) ) ) )
		OUT_OF_MEMORY;

	for( i = 0; i < parser->p_universe + 1; i++ )
		chr_sym[i] = 0;

	if( parser->p_invalid_suf )
	{
		ccl = p_ccl_to_map( parser, parser->p_invalid_suf );
		
		for( i = 0; i < parser->p_universe + 1; i++ )
			if( bitset_get( ccl, i ) )
				chr_sym[i] = 1;

		p_free( ccl );
	}

	for( i = 0; i < parser->p_universe; i++ )
	{
		if( chr_sym[i] )
			kw_invalid_suffix = p_str_append( kw_invalid_suffix,
				gen->kw_invalid_suffix.col_true, FALSE );
		else
			kw_invalid_suffix = p_str_append( kw_invalid_suffix,
				gen->kw_invalid_suffix.col_false, FALSE );

		if( i+1 < parser->p_universe )
			kw_invalid_suffix = p_str_append( kw_invalid_suffix,
				gen->kw_invalid_suffix.col_sep, FALSE );
	}

	/* Character map (charsets ARE unique!) */
	for( i = 0; i < parser->p_universe + 1; i++ )
		chr_sym[i] = -1;

	for( l = parser->symbols; l; l = l->next )
	{
		sym = (SYMBOL*)l->pptr;

		if( IS_TERMINAL( sym ) && !( sym->keyword ) )
		{
			ccl = p_ccl_to_map( parser, sym->name );
			
			for( i = 0; i < parser->p_universe + 1; i++ )
				if( bitset_get( ccl, i ) )
					chr_sym[i] = sym->id;

			p_free( ccl );
		}
	}

	for( i = 0; i < parser->p_universe; i++ )
	{
		char_map = p_str_append( char_map, p_tpl_insert( gen->charmap.col,
			GEN_WILD_PREFIX "code", p_int_to_str( i ), TRUE,
				GEN_WILD_PREFIX "symbol", p_int_to_str( chr_sym[i] ), TRUE, (uchar*)NULL ),
					TRUE ); 

		if( i+1 < parser->p_universe )
			char_map = p_str_append( char_map, gen->charmap.col_sep, FALSE );
	}

	p_free( chr_sym ); /* Now it can be freed... */

	/* Whitespace identification table and symbol-name-table */
	for( l = parser->symbols; l; l = l->next ) /* Okidoki, now do the generation */
	{
		sym = (SYMBOL*)l->pptr;

		/* printf( "sym->name = >%s< at %d\n", sym->name, sym->id ); */
		if( sym->whitespace )
			whitespaces = p_str_append( whitespaces,
				gen->whitespace.col_true, FALSE );
		else
			whitespaces = p_str_append( whitespaces,
				gen->whitespace.col_false, FALSE );

		symbols = p_str_append( symbols, p_tpl_insert( gen->symbols.col,
				GEN_WILD_PREFIX "symbol-name", p_escape_for_target( gen, sym->name, FALSE ), TRUE,
					GEN_WILD_PREFIX "symbol", p_int_to_str( sym->id ), TRUE,
						GEN_WILD_PREFIX "type", p_int_to_str( sym->type ), TRUE,
							(char*)NULL ), TRUE );

		if( max_symbol_name < (int)strlen( sym->name ) )
			max_symbol_name = (int)strlen( sym->name );

		if( l->next )
		{
			whitespaces = p_str_append( whitespaces,
				gen->whitespace.col_sep, FALSE );
			symbols = p_str_append( symbols,
				gen->symbols.col_sep, FALSE );
		}
	}

	/* Type definition union */
	if( list_count( parser->vtypes ) == 1 )
	{
		vt = (VTYPE*)( parser->vtypes->pptr );
		type_def = p_tpl_insert( gen->vstack_single,
				GEN_WILD_PREFIX "value-type", vt->real_def, FALSE,
					(uchar*)NULL );
	}
	else
	{
		type_def = p_tpl_insert( gen->vstack_union_start,
				GEN_WILD_PREFIX "number-of-value-types",
					p_int_to_str( list_count( parser->vtypes ) ),
						TRUE, (uchar*)NULL );

		for( l = parser->vtypes; l; l = l->next )
		{
			vt = (VTYPE*)(l->pptr);

			type_def = p_str_append( type_def,
				p_tpl_insert( gen->vstack_union_def,
					GEN_WILD_PREFIX "value-type", vt->real_def, FALSE,
						GEN_WILD_PREFIX "attribute", p_tpl_insert( gen->vstack_union_att,
									GEN_WILD_PREFIX "value-type-id", p_int_to_str( vt->id ), TRUE,
										(uchar*)NULL ), TRUE,
							GEN_WILD_PREFIX "value-type-id", p_int_to_str( vt->id ), TRUE,
								(uchar*)NULL ), TRUE );
		}

		type_def = p_str_append( type_def,
					p_tpl_insert( gen->vstack_union_end,
						GEN_WILD_PREFIX "number-of-value-types",
							p_int_to_str( list_count( parser->vtypes ) ),
								TRUE, (uchar*)NULL ), TRUE );
	}

	/* Reduction action code and production definition table */
	for( l = parser->productions, row = 0; l; l = l->next, row++ )
	{
		p = (PROD*)( l->pptr );

		actions = p_str_append( actions, p_tpl_insert( gen->action_start, 
			GEN_WILD_PREFIX "production-number", p_int_to_str( p->id ), TRUE,
				(uchar*)NULL ), TRUE );

		/* Select the semantic code to be processed! */
		act = (uchar*)NULL;

		if( 1 ) /* !( p->lhs->keyword )  */
		{
			is_default_code = FALSE;

			if( p->code )
				act = p->code;
			else if( list_count( p->rhs ) == 0 )
			{
				act = parser->p_def_action_e;
				is_default_code = TRUE;
			}
			else
			{
				act = parser->p_def_action;
				is_default_code = TRUE;
			}
			
			if( is_default_code &&
				( p->lhs->whitespace ||
					list_find( p->rhs, parser->error ) > -1 ) )
			{
				act = (uchar*)NULL;
			}

			if( act )
			{
				if( gen->code_localization )
				{
					actions = p_str_append( actions,
						p_tpl_insert( gen->code_localization,
							GEN_WILD_PREFIX "line",
								p_int_to_str( p->code_at ), TRUE,	
							(uchar*)NULL ),
						TRUE );
				}

				act = p_build_action( parser, gen, p, act, is_default_code );
				actions = p_str_append( actions, act, TRUE );
			}
		}

		actions = p_str_append( actions, p_tpl_insert( gen->action_end, 
			GEN_WILD_PREFIX "production-number", p_int_to_str( p->id ), TRUE,
				(uchar*)NULL ), TRUE );

		/* Generate production name */
		productions = p_str_append( productions, p_tpl_insert(
			gen->productions.col,
				GEN_WILD_PREFIX "production", p_escape_for_target(
						gen, p_mkproduction_str( p ), TRUE ),
			(uchar*)NULL ), TRUE );
			
		if( l->next )
			productions = p_str_append( productions,
				gen->productions.col_sep, FALSE );
	}

	/* Scanner action code */
	for( l = parser->symbols, row = 0; l; l = l->next, row++ )
	{
		sym = (SYMBOL*)( l->pptr );
		if( sym->type != SYM_REGEX_TERMINAL )
			continue;

		scan_actions = p_str_append( scan_actions, p_tpl_insert( gen->scan_action_start,
			GEN_WILD_PREFIX "symbol-number", p_int_to_str( sym->id ), TRUE,
				(uchar*)NULL ), TRUE );

		/* Select the semantic code to be processed! */
		act = (uchar*)NULL;

		if( sym->code )
			act = sym->code;

		if( act )
		{
			act = p_build_scan_action( parser, gen, sym, act );
			scan_actions = p_str_append( scan_actions, act, TRUE );
		}

		scan_actions = p_str_append( scan_actions, p_tpl_insert( gen->scan_action_end, 
			GEN_WILD_PREFIX "symbol-number", p_int_to_str( sym->id ), TRUE,
				(uchar*)NULL ), TRUE );
	}

	/* Get the goal production */
	goalprod = (PROD*)( parser->goal->productions->pptr );

	/* Assembling all together! */
	all = p_tpl_insert( gen->driver,
	
		/* Lengths of names and Prologue/Epilogue codes */
		GEN_WILD_PREFIX "name" LEN_EXT,
			p_long_to_str( (long)p_strlen( parser->p_name ) ), TRUE,
		GEN_WILD_PREFIX "copyright" LEN_EXT,
			p_long_to_str( (long)p_strlen( parser->p_copyright ) ), TRUE,
		GEN_WILD_PREFIX "version" LEN_EXT,
			p_long_to_str( (long)p_strlen( parser->p_version ) ), TRUE,
		GEN_WILD_PREFIX "description" LEN_EXT,
			p_long_to_str( (long)p_strlen( parser->p_desc ) ), TRUE,
		GEN_WILD_PREFIX "prologue" LEN_EXT,
			p_long_to_str( (long)p_strlen( parser->p_header ) ), TRUE,
		GEN_WILD_PREFIX "epilogue" LEN_EXT,
			p_long_to_str( (long)p_strlen( parser->p_footer ) ), TRUE,

		/* Names and Prologue/Epilogue codes */
		GEN_WILD_PREFIX "name", parser->p_name, FALSE,
		GEN_WILD_PREFIX "copyright", parser->p_copyright, FALSE,
		GEN_WILD_PREFIX "version", parser->p_version, FALSE,
		GEN_WILD_PREFIX "description", parser->p_desc, FALSE,
		GEN_WILD_PREFIX "prologue", parser->p_header, FALSE,
		GEN_WILD_PREFIX "epilogue", parser->p_footer, FALSE,

		/* Limits and sizes, parse tables */
		GEN_WILD_PREFIX "number-of-symbols", p_int_to_str( list_count( parser->symbols ) ), TRUE,
		GEN_WILD_PREFIX "number-of-states", p_int_to_str( list_count( parser->lalr_states ) ), TRUE,
		GEN_WILD_PREFIX "number-of-productions", p_int_to_str( list_count( parser->productions ) ), TRUE,
		GEN_WILD_PREFIX "number-of-dfa-machines", p_int_to_str( list_count( parser->kw ) ), TRUE, 
		GEN_WILD_PREFIX "deepest-action-row", p_int_to_str( max_action ), TRUE,
		GEN_WILD_PREFIX "deepest-goto-row", p_int_to_str( max_goto ), TRUE,
		GEN_WILD_PREFIX "deepest-dfa-index-row", p_int_to_str( max_dfa_idx ), TRUE,
		GEN_WILD_PREFIX "deepest-dfa-accept-row", p_int_to_str( max_dfa_accept ), TRUE,
		GEN_WILD_PREFIX "size-of-dfa-characters", p_int_to_str( column ), TRUE, 
		GEN_WILD_PREFIX "action-table", action_table, TRUE,
		GEN_WILD_PREFIX "goto-table", goto_table, TRUE,
		GEN_WILD_PREFIX "production-lengths", prod_rhs_count, TRUE,
		GEN_WILD_PREFIX "production-lhs", prod_lhs, TRUE,
		GEN_WILD_PREFIX "default-productions", def_prod, TRUE,
		GEN_WILD_PREFIX "character-map", char_map, TRUE,
		GEN_WILD_PREFIX "character-universe", p_int_to_str( parser->p_universe ), TRUE,
		GEN_WILD_PREFIX "whitespaces", whitespaces, TRUE,
		GEN_WILD_PREFIX "symbols", symbols, TRUE,
		GEN_WILD_PREFIX "productions", productions, TRUE,
		GEN_WILD_PREFIX "max-symbol-name-length", p_int_to_str( max_symbol_name ), TRUE,
		GEN_WILD_PREFIX "dfa-select", dfa_select, TRUE,
		GEN_WILD_PREFIX "dfa-index", dfa_idx, TRUE,
		GEN_WILD_PREFIX "dfa-char", dfa_char, TRUE,
		GEN_WILD_PREFIX "dfa-trans", dfa_trans, TRUE,
		GEN_WILD_PREFIX "dfa-accept", dfa_accept, TRUE,
		GEN_WILD_PREFIX "keyword-invalid-suffixes", kw_invalid_suffix, TRUE,
		GEN_WILD_PREFIX "value-type-definition", type_def, TRUE,
		GEN_WILD_PREFIX "actions", actions, TRUE,
		GEN_WILD_PREFIX "scan_actions", scan_actions, TRUE,
		GEN_WILD_PREFIX "top-value", top_value, TRUE,
		GEN_WILD_PREFIX "model", p_int_to_str( parser->p_model ), TRUE,
		GEN_WILD_PREFIX "error", ( parser->error ? p_int_to_str( parser->error->id ) :
									p_int_to_str( -1 ) ), TRUE,
		GEN_WILD_PREFIX "eof", ( parser->end_of_input ? 
									p_int_to_str( parser->end_of_input->id ) :
										p_int_to_str( -1 ) ), TRUE,
		GEN_WILD_PREFIX "goal-production", p_int_to_str( goalprod->id ), TRUE,
		GEN_WILD_PREFIX "goal", p_int_to_str( parser->goal->id ), TRUE,

		(uchar*)NULL
	);

	complete = p_tpl_insert( all,
				GEN_WILD_PREFIX "prefix", parser->p_prefix, FALSE,
				GEN_WILD_PREFIX "filename" LEN_EXT,
					p_long_to_str( (long)p_strlen( parser->filename ) ), TRUE,
				GEN_WILD_PREFIX "filename", parser->filename, FALSE,
				(uchar*)NULL );

	p_free( all );

	fprintf( stream, "%s", complete );

	p_free( complete );

	/* Freeing the generator's structure */
	p_free( gen->for_sequences );
	p_free( gen->do_sequences );
	xml_free( gen->xml );
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_escape_for_target()
	
	Author:			Jan Max Meyer
	
	Usage:			Escapes the input-string according to the parser template's
					escaping-sequence definitions. This function is used to
					print identifiers and character-class definitions to the
					target parser without getting trouble with the target
					language's escape characters (e.g. as in C - I love C!!!).
					
	Parameters:		GENERATOR*	g					Generator template structure			
					uchar*		str					Source string
					BOOLEAN		clear				If TRUE, str will be free'd,
													else not
	
	Returns:		uchar*							The final (escaped) string
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
uchar* p_escape_for_target( GENERATOR* g, uchar* str, BOOLEAN clear )
{
	int		i;
	uchar*	ret;
	uchar*	tmp;

	if( !( ret = p_strdup( str ) ) )
		OUT_OF_MEMORY;

	if( clear )
		p_free( str );

	for( i = 0; i < g->sequences_count; i++ )
	{
		if( !( tmp = p_tpl_insert( ret, g->for_sequences[ i ], g->do_sequences[ i ], FALSE, (uchar*)NULL ) ) )
			OUT_OF_MEMORY;

		p_free( ret );
		ret = tmp;
	}

	return ret;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_build_action()
	
	Author:			Jan Max Meyer
	
	Usage:			Constructs target language code for production reduction
					code blocks.
					
	Parameters:		PARSER*		parser				Parser information structure
					GENERATOR*	g					Generator template structure
					PROD*		p					Production
					uchar*		base				Code-base template for the
													reduction action
					BOOLEAN		def_code			Defines if the base-pointer
													is a default-code block or
													an individually coded one.
	
	Returns:		uchar*							Pointer to the generated
													code - must be freed by
													caller.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
uchar* p_build_action( PARSER* parser, GENERATOR* g, PROD* p,
			uchar* base, BOOLEAN def_code )
{
	LIST*		rhs			= p->rhs;
	LIST*		rhs_idents	= p->rhs_idents;
	BOOLEAN		cont		= TRUE;
	LIST*		l;
	LIST*		m;
	LIST*		nfa			= (LIST*)NULL;
	SYMBOL*		sym;
	uchar*		ptr			= base;
	uchar*		ret			= (uchar*)NULL;
	uchar*		tmp;
	uchar*		chk;
	uchar*		att;
	uchar*		last		= (uchar*)NULL;
	int			off;
	int			len;
	int			match;
	BOOLEAN		on_error	= FALSE;

	if( re_build_nfa( &nfa, "@[A-Za-z_][A-Za-z0-9_]*", 0 ) != RE_ERR_NONE )
		return (uchar*)NULL;

	if( re_build_nfa( &nfa, "@[0-9]+", 1 ) != RE_ERR_NONE )
		return (uchar*)NULL;

	if( re_build_nfa( &nfa, "@@", 2 ) != RE_ERR_NONE )
		return (uchar*)NULL;
		
	if( p->sem_rhs )
	{
		rhs = p->sem_rhs;
		rhs_idents = p->sem_rhs_idents;
	}

	/* re_dbg_print_nfa( nfa, 255 ); */

	while( *ptr && !on_error )
	{
		off = 0;
		/* The brainstorming within software was propably powered by GUINNESS Draught!! */
		while( ptr[ off ] && ( match = re_execute_nfa( nfa, ptr + off, &len ) ) < 0 )
			off++;

		if( off )
		{
			tmp = p_malloc( ( off + 1 ) * sizeof( uchar ) );
			if( !tmp )
			{
				OUT_OF_MEMORY;
				return (uchar*)NULL;
			}

			strncpy( tmp, ptr, off * sizeof( uchar ) );
			tmp[ off ] = '\0';

			ret = p_str_append( ret, tmp, TRUE );

			ptr += off;
		}

		len--;
		tmp = (uchar*)NULL;

		switch( match )
		{
			case 0:
				tmp = (uchar*)p_malloc( ( len + 1 ) * sizeof( uchar ) );
				if( !tmp )
				{
					OUT_OF_MEMORY;
					return (uchar*)NULL;
				}

				strncpy( tmp, ptr+1, len * sizeof( char ) );
				tmp[ len ] = '\0';

				for( l = rhs_idents, m = rhs, off = 1;
						l && m; l = l->next, m = m->next, off++ )
				{
					chk = (uchar*)( l->pptr );
					if( chk && !strcmp( chk, tmp ) )
						break;
				}
				
				p_free( tmp );
				break;

			case 1:
				tmp = (uchar*)p_malloc( ( len + 1 ) * sizeof( uchar ) );
				if( !tmp )
				{
					OUT_OF_MEMORY;
					return (uchar*)NULL;
				}

				strncpy( tmp, ptr+1, len * sizeof( char ) );
				tmp[ len ] = '\0';

				off = atoi( tmp );
				p_free( tmp );

				break;

			case 2:
				if( p->lhs->vtype && list_count( parser->vtypes ) > 1 )				
					tmp = p_tpl_insert( g->action_lhs_union,
						GEN_WILD_PREFIX "attribute", p_tpl_insert( g->vstack_union_att,
							GEN_WILD_PREFIX "value-type-id", p_int_to_str( p->lhs->vtype->id ), TRUE,
								(uchar*)NULL ), TRUE, (uchar*)NULL );
				else
					tmp = p_strdup( g->action_lhs_single );

				off = -1;
				break;

			default:
				len = -1;
				off = -1;
				break;
		}

		if( off > 0 && off <= list_count( rhs ) )
		{
			sym = (SYMBOL*)list_getptr( rhs, off - 1 );

			if( !( sym->type == SYM_KW_TERMINAL ) )
			{				
				if( list_count( parser->vtypes ) > 1 )
				{
					if( sym->vtype )
					{
						/* fprintf( stderr, "At >%s< using %s\n", sym->name, sym->vtype->int_name ); */
						att  = p_tpl_insert( g->vstack_union_att,
							GEN_WILD_PREFIX "value-type-id", p_int_to_str( sym->vtype->id ),
								TRUE, (uchar*)NULL );
					}
					else
					{
						p_error( ERR_NO_VALUE_TYPE, ERRSTYLE_FATAL,
								sym->name, p->id, len+1, ptr );
					
						att = (uchar*)NULL;
						on_error = TRUE;
					}

					tmp = p_tpl_insert( g->action_union,
						GEN_WILD_PREFIX "offset", p_int_to_str( list_count( rhs ) - off ), TRUE,
							GEN_WILD_PREFIX "attribute", att, TRUE, (uchar*)NULL );
				}
				else
					tmp = p_tpl_insert( g->action_single,
						GEN_WILD_PREFIX "offset", p_int_to_str( list_count( rhs ) - off ), TRUE,
							(uchar*)NULL );
			}
			else
			{
				if( !def_code )
				{
					p_error( ERR_NO_VALUE_TYPE, ERRSTYLE_FATAL,
							p_find_base_symbol( sym )->name, p->id, len+1, ptr );
				}

				on_error = TRUE;
			}
		}

		if( tmp )
			ret = p_str_append( ret, tmp, TRUE );

		ptr += ( len + 1 );
	}

	re_free_nfa_table( nfa );

	if( on_error && ret )
	{
		p_free( ret );
		ret = (uchar*)NULL;
	}

	return ret;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_build_scan_action()
	
	Author:			Jan Max Meyer
	
	Usage:			<usage>
					
	Parameters:		<type>		<identifier>		<description>
	
	Returns:		<type>							<description>
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
uchar* p_build_scan_action( PARSER* parser, GENERATOR* g, SYMBOL* s,
			uchar* base )
{
	BOOLEAN		cont		= TRUE;
	LIST*		nfa			= (LIST*)NULL;
	uchar*		ptr			= base;
	uchar*		ret			= (uchar*)NULL;
	uchar*		tmp;
	uchar*		last		= (uchar*)NULL;
	int			off;
	int			len;
	int			match;
	BOOLEAN		on_error	= FALSE;

	if( re_build_nfa( &nfa, "@>", 0 ) != RE_ERR_NONE )
		return (uchar*)NULL;

	if( re_build_nfa( &nfa, "@<", 1 ) != RE_ERR_NONE )
		return (uchar*)NULL;

	if( re_build_nfa( &nfa, "@@", 2 ) != RE_ERR_NONE )
		return (uchar*)NULL;

	while( *ptr && !on_error )
	{
		off = 0;

		while( ptr[ off ] && ( match = re_execute_nfa( nfa, ptr + off, &len ) ) < 0 )
			off++;

		if( off )
		{
			tmp = p_malloc( ( off + 1 ) * sizeof( uchar ) );
			if( !tmp )
			{
				OUT_OF_MEMORY;
				return (uchar*)NULL;
			}

			strncpy( tmp, ptr, off * sizeof( uchar ) );
			tmp[ off ] = '\0';

			ret = p_str_append( ret, tmp, TRUE );

			ptr += off;
		}

		len--;
		tmp = (uchar*)NULL;

		switch( match )
		{
			case 0:
				tmp = p_strdup( g->scan_action_begin_offset );
				break;

			case 1:
				tmp = p_strdup( g->scan_action_end_offset );
				break;

			case 2:
				if( s->vtype && list_count( parser->vtypes ) > 1 )
					tmp = p_tpl_insert( g->scan_action_ret_union,
						GEN_WILD_PREFIX "attribute", p_tpl_insert( g->vstack_union_att,
							GEN_WILD_PREFIX "value-type-id", p_int_to_str( s->vtype->id ), TRUE,
								(uchar*)NULL ), TRUE, (uchar*)NULL );
				else
					tmp = p_strdup( g->scan_action_ret_single );

				break;

			default:
				len = -1;				
				break;
		}

		if( tmp )
			ret = p_str_append( ret, tmp, TRUE );

		ptr += ( len + 1 );
	}

	re_free_nfa_table( nfa );

	return ret;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_mkproduction_str()
	
	Author:			Jan Max Meyer
	
	Usage:			Converts a production into a dynamic string.
					
	Parameters:		PROD*		p					Production pointer
	
	Returns:		uchar*							Generated string
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
uchar* p_mkproduction_str( PROD* p )
{
	uchar*		ret;
	uchar		wtf		[ 512 ];
	LIST*		l;
	SYMBOL*		sym;
	
	sprintf( wtf, "%s -> ", p->lhs->name );
	ret = p_strdup( wtf );
	
	for( l = p->rhs; l; l = l->next )
	{
		sym = (SYMBOL*)( l->pptr );
		
		switch( sym->type )
		{
			case SYM_CCL_TERMINAL:
				sprintf( wtf, "\'%s\'", sym->name );
				break;
			case SYM_KW_TERMINAL:
				sprintf( wtf, "\"%s\"", sym->name );
				break;
			case SYM_REGEX_TERMINAL:
				sprintf( wtf, "@%s", sym->name );
				break;
			case SYM_EXTERN_TERMINAL:
				sprintf( wtf, "*%s", sym->name );
				break;
			case SYM_ERROR_RESYNC:
				strcpy( wtf, P_ERROR_RESYNC );
				break;
				
			default:
				strcpy( wtf, sym->name );
				break;			
		}
		
		if( l->next )
			strcat( wtf, " " );
		
		ret = p_str_append( ret, wtf, FALSE );
	}
	
	return ret;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_load_generator()
	
	Author:			Jan Max Meyer
	
	Usage:			Loads a XML-defined code generator into an adequate
					GENERATOR structure. Pointers are only set to the
					values mapped to the XML-structure, so no memory is
					wasted.
					
	Parameters:		GENERATOR*		g				The target generator
					uchar*			genfile			Path to generator file
	
	Returns:		BOOLEAN			TRUE			on success
									FALSE			on error.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
BOOLEAN p_load_generator( GENERATOR* g, uchar* genfile )
{
	XML_T	tmp;
	uchar*	att_for;
	uchar*	att_do;
	int		i;

#define GET_XML_DEF( source, target, tagname ) \
	if( xml_child( (source), (tagname) ) ) \
		(target) = (uchar*)( xml_txt( xml_child( (source), (tagname) ) ) ); \
	else \
		p_error( ERR_TAG_NOT_FOUND, ERRSTYLE_WARNING, (tagname), genfile );

#define GET_XML_TAB_1D( target, tagname ) \
	if( ( tmp = xml_child( g->xml, (tagname) ) ) ) \
	{ \
		GET_XML_DEF( tmp, (target).col, "col" ) \
		GET_XML_DEF( tmp, (target).col_sep, "col_sep" ) \
	} \
	else \
		p_error( ERR_TAG_NOT_FOUND, ERRSTYLE_WARNING, (tagname), genfile );

#define GET_XML_BOOLTAB_1D( target, tagname ) \
	if( ( tmp = xml_child( g->xml, (tagname) ) ) ) \
	{ \
		GET_XML_DEF( tmp, (target).col_true, "col_true" ) \
		GET_XML_DEF( tmp, (target).col_false, "col_false" ) \
		GET_XML_DEF( tmp, (target).col_sep, "col_sep" ) \
	} \
	else \
		p_error( ERR_TAG_NOT_FOUND, ERRSTYLE_WARNING, (tagname), genfile );

#define GET_XML_TAB_2D( target, tagname ) \
	if( ( tmp = xml_child( g->xml, (tagname) ) ) ) \
	{ \
		GET_XML_DEF( tmp, (target).row_start, "row_start" ) \
		GET_XML_DEF( tmp, (target).row_end, "row_end" ) \
		GET_XML_DEF( tmp, (target).row_sep, "row_sep" ) \
		GET_XML_DEF( tmp, (target).col, "col" ) \
		GET_XML_DEF( tmp, (target).col_sep, "col_sep" ) \
	} \
	else \
		p_error( ERR_TAG_NOT_FOUND, ERRSTYLE_WARNING, (tagname), genfile );


	if( !( g->xml = xml_parse_file( genfile ) ) )
	{
		p_error( ERR_NO_GENERATOR_FILE, ERRSTYLE_FATAL, genfile );
		return FALSE;
	}

	if( *xml_error( g->xml ) )
	{
		p_error( ERR_XML_ERROR, ERRSTYLE_FATAL, genfile, xml_error( g->xml ) );
		return FALSE;
	}

	GET_XML_DEF( g->xml, g->driver, "driver" );
	GET_XML_DEF( g->xml, g->vstack_def_type, "vstack_def_type" );
	GET_XML_DEF( g->xml, g->vstack_term_type, "vstack_term_type" );

	GET_XML_DEF( g->xml, g->action_start, "action_start" );
	GET_XML_DEF( g->xml, g->action_end, "action_end" );
	GET_XML_DEF( g->xml, g->action_single, "action_single" );
	GET_XML_DEF( g->xml, g->action_union, "action_union" );
	GET_XML_DEF( g->xml, g->action_lhs_single, "action_lhs_single" );
	GET_XML_DEF( g->xml, g->action_lhs_union, "action_lhs_union" );

	GET_XML_DEF( g->xml, g->scan_action_start, "scan_action_start" );
	GET_XML_DEF( g->xml, g->scan_action_end, "scan_action_end" );
	GET_XML_DEF( g->xml, g->scan_action_begin_offset, "scan_action_begin_offset" );
	GET_XML_DEF( g->xml, g->scan_action_end_offset, "scan_action_end_offset" );
	GET_XML_DEF( g->xml, g->scan_action_ret_single, "scan_action_ret_single" );
	GET_XML_DEF( g->xml, g->scan_action_ret_union, "scan_action_ret_union" );

	GET_XML_DEF( g->xml, g->vstack_single, "vstack_single" );
	GET_XML_DEF( g->xml, g->vstack_union_start, "vstack_union_start" );
	GET_XML_DEF( g->xml, g->vstack_union_end, "vstack_union_end" );
	GET_XML_DEF( g->xml, g->vstack_union_def, "vstack_union_def" );
	GET_XML_DEF( g->xml, g->vstack_union_att, "vstack_union_att" );

	GET_XML_TAB_1D( g->prodlen, "prodlen" )
	GET_XML_TAB_1D( g->prodlhs, "prodlhs" )
	GET_XML_TAB_1D( g->defprod, "defprod" )
	GET_XML_TAB_1D( g->charmap, "charmap" )
	GET_XML_TAB_1D( g->dfa_select, "dfa_select" )
	GET_XML_TAB_1D( g->dfa_char, "dfa_char" )
	GET_XML_TAB_1D( g->dfa_trans, "dfa_trans" )
	GET_XML_BOOLTAB_1D( g->kw_invalid_suffix, "kw_invalid_suffix" )
	GET_XML_BOOLTAB_1D( g->whitespace, "whitespace" )

	GET_XML_TAB_2D( g->acttab, "acttab" )
	GET_XML_TAB_2D( g->gotab, "gotab" )
	GET_XML_TAB_2D( g->dfa_idx, "dfa_idx" )
	GET_XML_TAB_2D( g->dfa_accept, "dfa_accept" )

	GET_XML_TAB_1D( g->symbols, "symbols" )
	GET_XML_TAB_1D( g->productions, "productions" )
	
	GET_XML_DEF( g->xml, g->code_localization, "code_localization" );

	/* Escape sequence definitions */
	for( tmp = xml_child( g->xml, "escape-sequence" ); tmp; tmp = xml_next( tmp ) )
	{
		att_for = (uchar*)xml_attr( tmp, "for" );
		att_do = (uchar*)xml_attr( tmp, "do" );
		
		if( att_for && att_do )
		{
			for( i = 0; i < g->sequences_count; i++ )
			{
				if( !strcmp( g->for_sequences[ i ], att_for ) )
				{
					p_error( ERR_DUPLICATE_ESCAPE_SEQ, ERRSTYLE_WARNING, att_for, genfile );
					break;
				}
			}

			if( i < g->sequences_count )
				continue;

			g->for_sequences = (uchar**)p_realloc( (uchar**)g->for_sequences,
					( g->sequences_count + 1 ) * sizeof( uchar* ) );
			g->do_sequences = (uchar**)p_realloc( (uchar**)g->do_sequences,
					( g->sequences_count + 1 ) * sizeof( uchar* ) );

			if( !( g->for_sequences && g->do_sequences ) )
				OUT_OF_MEMORY;

			g->for_sequences[ g->sequences_count ] = (uchar*)( att_for );
			g->do_sequences[ g->sequences_count ] = (uchar*)( att_do );

			if( !( g->for_sequences[ g->sequences_count ]
				&& g->do_sequences[ g->sequences_count ] ) )
				OUT_OF_MEMORY;

			g->sequences_count++;
		}
		else
		{
			if( !att_for )
				p_error( ERR_XML_INCOMPLETE, ERRSTYLE_FATAL, genfile, xml_name( tmp ), "for" );
			if( !att_do )
				p_error( ERR_XML_INCOMPLETE, ERRSTYLE_FATAL, genfile, xml_name( tmp ), "do" );
		}
	}

	return TRUE;
}

