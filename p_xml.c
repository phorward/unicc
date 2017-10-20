/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2017 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com ++ unicc<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	p_xml.c
Author:	Jan Max Meyer
Usage:	The XML-based Parser Definition code-generator.
----------------------------------------------------------------------------- */

#include "p_global.h"
#include "p_error.h"
#include "p_proto.h"

#define XML_YES		"yes"
#define XML_NO		"no"

#define SYMBOL_VAR	"symbol"

extern char*	pmod[];

/** Dumps a character-class definition into an XML-structure.

//parent_xml// is the Parent element where the character-class block will be
attached to. //ccl// is the character-class to be dumped. */
static void p_xml_ccl( XML_T parent_xml, pccl* ccl )
{
	int			i;
	wchar_t		beg;
	wchar_t		end;
	XML_T		ccl_xml;
	XML_T		range_xml;

	PROC( "p_xml_ccl" );
	PARMS( "parent_xml", "%p", parent_xml );
	PARMS( "ccl", "%p", ccl );

	if( !( ccl_xml = xml_add_child( parent_xml, "character-class", 0 ) ) )
		OUTOFMEM;

	xml_set_int_attr( ccl_xml, "count", p_ccl_count( ccl ) );

	for( i = 0; p_ccl_get( &beg, &end, ccl, i ); i++ )
	{
		if( !( range_xml = xml_add_child( ccl_xml, "range", 0 ) ) )
			OUTOFMEM;

		xml_set_int_attr( range_xml, "from", beg );
		xml_set_int_attr( range_xml, "to", end );
	}

	VOIDRET;
}

/** Utility function that generates a raw-code-tag from a string.

//code_xml// is the parent element where the raw-code block will be attached to.
//code// is the content of the raw code block.

Returns a XML_T raw-code tag.
*/
static XML_T p_xml_raw_code( XML_T code_xml, char* code )
{
	XML_T	raw_code;

	if( !code )
		OUTOFMEM;

	if( !( raw_code = xml_add_child( code_xml, "raw", 0 ) ) )
		OUTOFMEM;

	xml_set_txt_f( raw_code, code );

	return raw_code;
}

/** Constructs a XML-structure for a production's semantic action code, by
splitting the original program code into XML-blocks, which can later be easier
translated into the particular parser program.

//code_xml// is the code-XML-node where elements will be attached to.
//parser// is the parser information structure.
//p// is the production.
//base// is the code-base template for the reduction action.
//def_code// defines if the base-pointer is a default-code block or an
individually coded one.

Returns TRUE on success.
*/
static BOOLEAN p_xml_build_action( XML_T code_xml, PARSER* parser, PROD* p,
		char* base, BOOLEAN def_code )
{
	plex*			lex;
	int				off;
	char*			last		= base;
	char*			start;
	char*			end;
	int				match;
	char*			chk;
	char*			tmp;
	LIST*			l;
	LIST*			m;
	LIST*			rhs			= p->rhs;
	LIST*			rhs_idents	= p->rhs_idents;
	BOOLEAN			on_error	= FALSE;
	SYMBOL*			sym;
	char*			raw;
	XML_T			code;

	/*
	12.07.2010	Jan Max Meyer

	Print warning if code symbol references to undefined symbol on the
	semantic rhs!
	*/

	PROC( "p_xml_build_action" );
	PARMS( "code_xml", "%p", code_xml );
	PARMS( "parser", "%p", parser );
	PARMS( "p", "%p", p );
	PARMS( "base", "%s", base );
	PARMS( "def_code", "%s", BOOLEAN_STR( def_code ) );

	/* Prepare regular expression engine */
	lex = plex_create( 0 );

	if( !( plex_define( lex, "@'([^']|\\')*'", 1, 0 )
			&& plex_define( lex, "@\"([^\"]|\\\")*\"", 1, 0 )
			&& plex_define( lex, "@[A-Za-z_][A-Za-z0-9_]*", 2, 0 )
			&& plex_define( lex, "@[0-9]+", 3, 0 )
			&& plex_define( lex, "@@", 4, 0 )
			/*
			 * Hmm ... this way looks "cooler" for future versions, maybe
			 * this would be a nice extension: @<command>:<parameters>
			 */
			&& plex_define( lex, "@!" SYMBOL_VAR ":[A-Za-z_][A-Za-z0-9_]*",
										5, 0 )
		) )
	{
		plex_free( lex );
		RETURN( FALSE );
	}

	VARS( "p->sem_rhs", "%p", p->sem_rhs  );
	/* Ok, perform replacement operations */
	if( p->sem_rhs )
	{
		MSG( "Replacing semantic right-hand side" );
		rhs = p->sem_rhs;
		rhs_idents = p->sem_rhs_idents;
	}

	MSG( "Iterating trough result array" );
	while( ( start = plex_next( lex, last, &match, &end ) ) && !on_error )
	{
		off = 0;
		tmp = (char*)NULL;

		/* Copy raw part of code into its own tag */
		if( last < start )
		{
			if( !( raw = pstrncatstr(
					(char*)NULL, last, start - last ) ) )
				OUTOFMEM;

			p_xml_raw_code( code_xml, raw );

			VARS( "end", "%s", end );
		}

		last = end;

		VARS( "match", "%d", match );
		switch( match )
		{
			case 1:
				start++;
				end--;

			case 2:
				MSG( "Identifier" );
				for( l = rhs_idents, m = rhs, off = 1; l && m;
						l = list_next( l ), m = list_next( m ), off++ )
				{
					chk = (char*)list_access( l );
					VARS( "chk", "%s", chk ? chk : "(NULL)" );

					/*
					printf( "check >%s< with >%.*s<\n",
						chk, end - start - 1, start + 1 );
					*/
					if( chk && !strncmp( chk, start + 1,
									end - start - 1 )
							&& pstrlen( chk ) == end - start - 1 )
					{
						break;
					}
				}

				if( !l )
				{
					p_error( parser, ERR_UNDEFINED_SYMREF, ERRSTYLE_WARNING,
										end - start, start  );
					off = 0;

					tmp = pstrdup( start );
				}

				VARS( "off", "%d", off );
				break;

			case 3:
				MSG( "Offset" );
				off = atoi( start + 1 );
				break;

			case 4:
				MSG( "Left-hand side" );

				if( !( code = xml_add_child( code_xml, "variable", 0 ) ) )
					OUTOFMEM;

				if( !( xml_set_attr( code, "target", "left-hand-side" ) ) )
					OUTOFMEM;

				if( p->lhs->vtype )
				{
					if( !( xml_set_attr( code, "value-type",
							p->lhs->vtype->real_def ) ) )
						OUTOFMEM;

					if( !( xml_set_int_attr( code, "value-type-id",
							p->lhs->vtype->id ) ) )
						OUTOFMEM;
				}

				break;

			case 5:
				MSG( "Assign left-hand side symbol" );

				if( !( tmp = pasprintf( "%.*s",
								end - start - ( pstrlen( SYMBOL_VAR ) + 3 ),
								start + ( pstrlen( SYMBOL_VAR ) + 3 )
									) ) )
				{
					OUTOFMEM;
					RETURN( FALSE );
				}

				VARS( "tmp", "%s", tmp );

				/* Go through all possible left-hand side symbols */
				for( l = p->all_lhs; l; l = list_next( l ) )
				{
					sym = (SYMBOL*)list_access( l );

					if( !strcmp( sym->name, tmp ) )
					{
						MSG( "Found a matching symbol!" );

						pfree( tmp );
						tmp = (char*)NULL;

						if( !( code = xml_add_child( code_xml,
								"command", 0 ) ) )
							OUTOFMEM;

						if( !( xml_set_attr( code,
								"action", "set-symbol" ) ) )
							OUTOFMEM;

						if( !( xml_set_int_attr( code, "symbol", sym->id ) ) )
							OUTOFMEM;

						break;
					}
				}

				if( !l )
				{
					MSG( "No match found..." );

					p_error( parser, ERR_UNDEFINED_LHS, ERRSTYLE_WARNING, tmp );
					pfree( tmp );

					if( !( tmp = pstrdup( start ) ) )
					{
						OUTOFMEM;
						RETURN( FALSE );
					}
				}

				break;

			default:
				MSG( "Uncaught regular expression match!" );
				break;
		}

		VARS( "off", "%d", off );
		if( off > 0 )
		{
			MSG( "Handing offset" );
			sym = (SYMBOL*)list_getptr( rhs, off - 1 );

			if( !( sym->keyword ) )
			{
				if( !( code = xml_add_child( code_xml, "variable", 0 ) ) )
					OUTOFMEM;

				if( !( xml_set_attr( code, "target", "right-hand-side" ) ) )
					OUTOFMEM;

				if( !( xml_set_int_attr( code, "offset",
						list_count( rhs ) - off ) ) )
					OUTOFMEM;

				if( sym->vtype )
				{
					if( !( xml_set_attr( code, "value-type",
							sym->vtype->real_def ) ) )
						OUTOFMEM;

					if( !( xml_set_int_attr( code, "value-type-id",
							sym->vtype->id ) ) )
						OUTOFMEM;
				}
			}
			else
			{
				if( !def_code )
				{
					p_error( parser, ERR_NO_VALUE_TYPE, ERRSTYLE_FATAL,
							p_find_base_symbol( sym )->name,
								p->id, end - start + 1, start );
				}

				on_error = TRUE;
			}
		}

		if( tmp )
			p_xml_raw_code( code_xml, tmp );
	}

	if( last && *last )
		p_xml_raw_code( code_xml, pstrdup( last ) );

	plex_free( lex );

	RETURN( !on_error );
}

/** Builds the scanner actions */
static BOOLEAN p_xml_build_scan_action(
	XML_T code_xml, PARSER* parser, SYMBOL* s, char* base )
{
	plex*			lex;
	char*			last	= base;
	char*			start;
	char*			end;
	int				match;
	char*			raw;
	XML_T			code;
	char*			tmp;
	SYMBOL*			sym;
	LIST*			l;

	PROC( "p_build_scan_action" );
	PARMS( "code_xml", "%p", code_xml );
	PARMS( "parser", "%p", parser );
	PARMS( "s", "%p", s );
	PARMS( "base", "%s", base );

	/* Prepare regular expression engine */
	lex = plex_create( PREGEX_COMP_NOANCHORS );

	if( !( plex_define( lex, "@>", 1, 0 )
		&& plex_define( lex, "@<", 2, 0 )
		&& plex_define( lex, "@@", 3, 0 )
		&& plex_define( lex, "@!" SYMBOL_VAR ":[A-Za-z_][A-Za-z0-9_]*",  4, 0 )
		) )
	{
		plex_free( lex );
		RETURN( FALSE );
	}

	while( ( start = plex_next( lex, last, &match, &end ) ) )
	{
		if( last < start )
		{
			if( !( raw = pstrncatstr(
					(char*)NULL, last, start - last ) ) )
				OUTOFMEM;

			p_xml_raw_code( code_xml, raw );

			VARS( "raw", "%s", raw );
		}

		last = end;

		VARS( "match", "%d", match );
		switch( match )
		{
			case 1:
				MSG( "@>" );

				if( !( code = xml_add_child( code_xml,
						"begin-of-match", 0 ) ) )
					OUTOFMEM;

				break;

			case 2:
				MSG( "@<" );

				if( !( code = xml_add_child( code_xml,
						"end-of-match", 0 ) ) )
					OUTOFMEM;

				break;

			case 3:
				MSG( "@@" );

				if( !( code = xml_add_child( code_xml,
						"return-value", 0 ) ) )
					OUTOFMEM;

				if( s->vtype && list_count( parser->vtypes ) > 1 )
				{
					if( !( xml_set_attr( code, "value-type",
								s->vtype->real_def ) ) )
						OUTOFMEM;

					if( !( xml_set_int_attr( code, "value-type-id",
							s->vtype->id ) ) )
						OUTOFMEM;
				}
				break;

			case 4:
				MSG( "Set terminal symbol" );

				if( !( tmp = pasprintf( "%.*s",
								end - start - ( pstrlen( SYMBOL_VAR ) + 3 ),
								start + ( pstrlen( SYMBOL_VAR ) + 3 )
									) ) )
					OUTOFMEM;

				VARS( "tmp", "%s", tmp );

				/* Go through all possible terminal symbols */
				for( l = s->all_sym; l; l = list_next( l ) )
				{
					sym = (SYMBOL*)list_access( l );

					if( !strcmp( sym->name, tmp ) )
					{
						MSG( "Found a matching symbol!" );

						if( !( code = xml_add_child( code_xml,
								"command", 0 ) ) )
							OUTOFMEM;

						if( !( xml_set_attr( code,
								"action", "set-symbol" ) ) )
							OUTOFMEM;

						if( !( xml_set_int_attr( code, "symbol", sym->id ) ) )
							OUTOFMEM;
						break;
					}
				}

				if( !l )
				{
					MSG( "No match found..." );

					p_error( parser, ERR_UNDEFINED_TERMINAL,
								ERRSTYLE_WARNING, tmp );
				}

				pfree( tmp );
				break;

			default:
				MSG( "Uncaught regular expression match!" );
				break;
		}
	}

	if( last && *last )
		p_xml_raw_code( code_xml, pstrdup( last ) );

	plex_free( lex );

	RETURN( TRUE );
}

/** Builds the DFAs XML structure. */
static void p_build_dfa( XML_T parent, pregex_dfa* dfa )
{
	pregex_dfa_st*	st;
	pregex_dfa_tr*	tr;
	int 			i;
	plistel*		e;
	plistel*		f;
	XML_T			state;
	XML_T			trans;

	PROC( "p_build_dfa" );

	for( e = plist_first( dfa->states ), i = 0; e; e = plist_next( e ), i++ )
	{
		st = (pregex_dfa_st*)plist_access( e );

		if( !( state = xml_add_child( parent, "state", 0 ) ) )
			OUTOFMEM;

		if( !( xml_set_int_attr( state, "id", i ) ) )
				OUTOFMEM;

		if( st->accept.accept &&
			!( xml_set_int_attr( state, "accept", st->accept.accept ) ) )
				OUTOFMEM;

		if( st->def_trans )
			if( !( xml_set_int_attr( state, "default-transition",
					st->def_trans->go_to ) ) )
				OUTOFMEM;

		plist_for( st->trans, f )
		{
			tr = (pregex_dfa_tr*)plist_access( f );

			if( !( trans = xml_add_child( state, "transition", 0 ) ) )
				OUTOFMEM;

			if( !( xml_set_int_attr( trans, "goto", tr->go_to ) ) )
				OUTOFMEM;

			p_xml_ccl( trans, tr->ccl );
		}
	}

	VOIDRET;
}

static void p_xml_print_options( plist* opts, XML_T append_to )
{
	plistel*	e;
	OPT*		opt;
	XML_T		option;

	/* Set parser options */
	plist_for( opts, e )
	{
		opt = (OPT*)plist_access( e );

		if( !( option = xml_add_child( append_to, "option", 0 ) ) )
			OUTOFMEM;

		if( !xml_set_attr( option, "name", opt->opt ) )
			OUTOFMEM;

		if( !xml_set_int_attr( option, "line", opt->line ) )
			OUTOFMEM;

		if( !( xml_set_txt( option, opt->def ) ) )
			OUTOFMEM;
	}
}


static void p_xml_print_symbols( PARSER* parser, XML_T par )
{
	LIST*			l;
	SYMBOL*			sym;
	char*			tmp;
	pregex_nfa*		tmp_nfa;
	pregex_dfa*		tmp_dfa;

	XML_T			sym_tab;
	XML_T			symbol;
	XML_T			code;
	XML_T			lex;
	XML_T			regex;

	PROC( "p_xml_print_symbols" );
	MSG( "Printing symbol table" );

	if( !( sym_tab = xml_add_child( par, "symbols", 0 ) ) )
		OUTOFMEM;

	for( l = parser->symbols; l; l = list_next( l ) )
	{
		sym = (SYMBOL*)list_access( l );

		if( !( symbol = xml_add_child( sym_tab, "symbol", 0 ) ) )
			OUTOFMEM;

		xml_set_attr_d( symbol, "type",
			IS_TERMINAL( sym ) ? "terminal" : "non-terminal" );
		xml_set_int_attr( symbol, "id", sym->id );
		xml_set_attr( symbol, "name", sym->name );

		if( IS_TERMINAL( sym ) )
		{
			switch( sym->type )
			{
				case SYM_CCL_TERMINAL:
					tmp = "character-class";
					p_xml_ccl( symbol, sym->ccl );
					break;
				case SYM_REGEX_TERMINAL:
					if( sym->keyword )
						tmp = "string";
					else
					{
						tmp = "regular-expression";

						if( !( regex = xml_add_child(
								symbol, "regex", 0 ) ) )
							OUTOFMEM;

						xml_set_txt_d( regex, pregex_ptn_to_regex( sym->ptn ) );

						/*
							Convert regular pattern into DFA state machine
						*/
						tmp_nfa = pregex_nfa_create();
						tmp_dfa = pregex_dfa_create();

						if( !sym->ptn->accept )
							sym->ptn->accept = (pregex_accept*)pmalloc(
													sizeof( pregex_accept ) );

						sym->ptn->accept->accept = sym->id;

						pregex_ptn_to_nfa( tmp_nfa, sym->ptn );

						pregex_dfa_from_nfa( tmp_dfa, tmp_nfa );
						pregex_dfa_minimize( tmp_dfa );

						if( !( lex = xml_add_child( symbol, "dfa", 0 ) ) )
							OUTOFMEM;

						p_build_dfa( lex, tmp_dfa );
						tmp_nfa = pregex_nfa_free( tmp_nfa );
						tmp_dfa = pregex_dfa_free( tmp_dfa );
					}
					break;
				case SYM_SYSTEM_TERMINAL:
					tmp = "system";
					break;

				default:
					tmp = "!!!UNDEFINED!!!";
					MISS_MSG( "Unhandled terminal type in "
								"XML code generator" );
					break;
			}

			xml_set_attr_d( symbol, "terminal-type", tmp );

			if( sym->whitespace )
				xml_set_attr( symbol, "is-whitespace", XML_YES );

			/* Code (in case of regex terminals */
			if( sym->code && *( sym->code ) )
			{
				if( !( code = xml_add_child( symbol, "code", 0 ) ) )
					OUTOFMEM;

				if( sym->code_at > 0 )
					xml_set_int_attr( code, "defined-at", sym->code_at );

				p_xml_build_scan_action( code, parser, sym, sym->code );
			}
		}
		else
		{
			/* Goal symbol TODO */
			if( sym->goal )
				xml_set_attr( symbol, "is-goal", XML_YES );

			/* Derived-from */
			if( sym->generated && sym->derived_from )
				xml_set_int_attr( symbol, "derived-from",
					sym->derived_from->id );
		}

		/* Symbol value type */
		if( sym->vtype )
		{
			if( !( xml_set_attr( symbol, "value-type",
						sym->vtype->real_def ) ) )
				OUTOFMEM;

			if( !( xml_set_int_attr( symbol, "value-type-id",
					sym->vtype->id ) ) )
				OUTOFMEM;
		}

		if( sym->line > 0 )
			xml_set_int_attr( symbol, "defined-at", sym->line );

		/* Set symbol options */
		p_xml_print_options( sym->options, symbol );
	}

	VOIDRET;
}

static void p_xml_print_productions( PARSER* parser, XML_T par )
{
	LIST*			l;
	LIST*			m;
	SYMBOL*			sym;
	PROD*			p;
	BOOLEAN			is_default_code;
	char*			act;
	char*			tmp;
	int				i;
	int				j;

	XML_T			prod_tab;
	XML_T			prod;
	XML_T			lhs;
	XML_T			rhs;
	XML_T			code;

	PROC( "p_xml_print_productions" );
	MSG( "Printing production table" );

	if( !( prod_tab = xml_add_child( par, "productions", 0 ) ) )
		OUTOFMEM;

	for( l = parser->productions; l; l = list_next( l ) )
	{
		p = (PROD*)list_access( l );

		if( !( prod = xml_add_child( prod_tab, "production", 0 ) ) )
			OUTOFMEM;

		/* Production id */
		xml_set_int_attr( prod, "id", p->id );
		xml_set_int_attr( prod, "length", list_count( p->rhs ) );
		if( p->line > 0 )
			xml_set_int_attr( prod, "defined-at", p->line );

		/* Print all left-hand sides */
		for( m = p->all_lhs, i = 0; m; m = list_next( m ), i++ )
		{
			sym = (SYMBOL*)list_access( m );

			if( !( lhs = xml_add_child( prod, "left-hand-side", 0 ) ) )
				OUTOFMEM;

			xml_set_int_attr( lhs, "symbol-id", sym->id );
			xml_set_int_attr( lhs, "offset", i );
		}

		/* Print parts of semantic right-hand side */
		for( m = p->sem_rhs, i = 0; m &&
			list_count( m ) > list_count( p->rhs );
				m = list_next( m ), i++ )
		{
			sym = (SYMBOL*)list_access( m );

			if( !( rhs = xml_add_child( prod,
					"semantic-right-hand-side", 0 ) ) )
				OUTOFMEM;

			xml_set_int_attr( rhs, "symbol-id", sym->id );
			xml_set_int_attr( rhs, "offset", i );

			if( ( tmp = (char*)list_getptr( p->sem_rhs_idents, i ) ) )
				xml_set_attr( rhs, "named", tmp );
		}

		/* Print right-hand side */
		for( m = p->rhs, j = 0; m; m = list_next( m ), i++, j++ )
		{
			sym = (SYMBOL*)list_access( m );

			if( !( rhs = xml_add_child( prod, "right-hand-side", 0 ) ) )
				OUTOFMEM;

			xml_set_int_attr( rhs, "symbol-id", sym->id );
			xml_set_int_attr( rhs, "offset", i );

			if( ( tmp = (char*)list_getptr( p->rhs_idents, j ) ) )
				xml_set_attr( rhs, "named", tmp );
		}

		/* Set productions options */
		p_xml_print_options( p->options, prod );

		/* Code */
		is_default_code = FALSE;

		/* Production has attached semantic action */
		if( p->code )
			act = p->code;
		/* Non-empty production */
		else if( list_count( p->rhs ) == 0 )
		{
			act = parser->p_def_action_e;
			is_default_code = TRUE;
		}
		/* Empty production */
		else
		{
			act = parser->p_def_action;
			is_default_code = TRUE;
		}

		/*
			Unset action code if default code was chosen, left-hand side
			is whitespace and error token is part of whitespaces ?
		*/
		if( is_default_code &&
			( p->lhs->whitespace ||
				list_find( p->rhs, parser->error ) > -1 ) )
		{
			act = (char*)NULL;
		}

		if( act && *act )
		{
			if( !( code = xml_add_child( prod, "code", 0 ) ) )
				OUTOFMEM;

			if( p->code_at > 0 )
				xml_set_int_attr( code, "defined-at", p->code_at );

			p_xml_build_action( code, parser, p, act, is_default_code );
		}
	}

	VOIDRET;
}

static void p_xml_print_states( PARSER* parser, XML_T par )
{
	LIST*			l;
	LIST*			m;

	STATE*			st;
	TABCOL*			col;
	char*			transtype;
	int				i;
	int				st_lex;

	XML_T			state_tab;
	XML_T			state;
	XML_T			go_to;
	XML_T			action;

	PROC( "p_xml_print_states" );

	MSG( "State table" );

	if( !( state_tab = xml_add_child( par, "states", 0 ) ) )
		OUTOFMEM;

	for( l = parser->lalr_states, i = 0; l; l = list_next( l ), i++ )
	{
		/* Get state pointer */
		st = (STATE*)list_access( l );

		/* Add state entity */
		if( !( state = xml_add_child( state_tab, "state", 0 ) ) )
			OUTOFMEM;

		/* Set some state-specific options */
		xml_set_int_attr( state, "id", st->state_id );

		/* Default Production */
		if( st->def_prod )
			xml_set_int_attr( state, "default-production",
				st->def_prod->id );

		/* Matching Lexer */
		if( ( st_lex = list_find( parser->kw, st->dfa ) ) >= 0 )
			xml_set_int_attr( state, "lexer", st_lex );

		/* Derived from state CHECK! */
		if( st->derived_from )
			xml_set_int_attr( state, "derived-from-state",
				st->derived_from->state_id );

		/* Action table */
		for( m = st->actions; m; m = list_next( m ) )
		{
			/* Get table column pointer */
			col = (TABCOL*)list_access( m );

			/* Shift, reduce, shift&reduce or even error? */
			switch( col->action )
			{
				case ERROR: /* Error */
					transtype = "error";
					break;
				case REDUCE: /* Reduce */
					transtype = "reduce";
					break;
				case SHIFT: /* Shift */
					transtype = "shift";
					break;
				case SHIFT_REDUCE: /* Shift&Reduce */
					transtype = "shift-reduce";
					break;

				default:
					MISS_MSG( "Unhandled action table action in "
								"XML code generator" );
					break;
			}

			if( !( action = xml_add_child( state, transtype, 0 ) ) )
				OUTOFMEM;

			if( !( xml_set_int_attr( action, "symbol-id",
					col->symbol->id ) ) )
				OUTOFMEM;

			/* CHECK! */
			if( col->action != ERROR )
			{
				if( col->action & REDUCE )
					xml_set_int_attr( action, "by-production", col->index );
				else
					xml_set_int_attr( action, "to-state", col->index );
			}
		}

		/* Goto table */
		for( m = st->gotos; m; m = list_next( m ) )
		{
			/* Get table column pointer */
			col = (TABCOL*)list_access( m );

			if( !( go_to = xml_add_child( state, "goto", 0 ) ) )
				OUTOFMEM;

			if( !( xml_set_int_attr( go_to, "symbol-id",
					col->symbol->id ) ) )
				OUTOFMEM;

			/* Only print goto on reduce; Else, its value is not relevant */
			if( ( col->action & REDUCE ) )
			{
				if( !( xml_set_int_attr( go_to,
							"by-production", col->index ) ) )
					OUTOFMEM;
			}
			else
			{
				if( !( xml_set_int_attr( go_to,
							"to-state", col->index ) ) )
					OUTOFMEM;
			}
		}

		/* TODO Kernel */
		/*
		LISTFOR( st->kernel, m )
		{
			item = (ITEM*)list_access( m );

			LISTFOR( item->prod->rhs, n )
			{

			}
		}
		*/
	}

	VOIDRET;
}

static void p_xml_print_lexers( PARSER* parser, XML_T par )
{
	LIST*			l;
	int				i;
	pregex_dfa*		dfa;

	XML_T			lex_tab;
	XML_T			lex;

	PROC( "p_xml_print_lexers" );

	if( !( lex_tab = xml_add_child( par, "lexers", 0 ) ) )
		OUTOFMEM;

	for( l = parser->kw, i = 0; l; l = list_next( l ), i++ )
	{
		dfa = (pregex_dfa*)list_access( l );

		if( !( lex = xml_add_child( lex_tab, "lexer", 0 ) ) )
			OUTOFMEM;

		if( list_count( parser->kw ) > 1 &&
			!( xml_set_int_attr( lex, "id", i ) ) )
				OUTOFMEM;

		p_build_dfa( lex, dfa );
	}

	VOIDRET;
}

static void p_xml_print_vtypes( PARSER* parser, XML_T par )
{
	LIST*			l;
	VTYPE*			vt;

	XML_T			vartype_tab;
	XML_T			vartype;

	PROC( "p_xml_print_vtypes" );

	if( !( vartype_tab = xml_add_child( par, "value-types", 0 ) ) )
		OUTOFMEM;

	for( l = parser->vtypes; l; l = list_next( l ) )
	{
		vt = (VTYPE*)list_access( l );

		if( !( vartype = xml_add_child( vartype_tab, "value-type", 0 ) ) )
			OUTOFMEM;

		if( !xml_set_int_attr( vartype, "id", vt->id ) )
			OUTOFMEM;

		if( !xml_set_attr( vartype, "c_name", vt->int_name ) )
			OUTOFMEM;

		if( !xml_set_txt( vartype, vt->real_def ) )
			OUTOFMEM;
	}

	VOIDRET;
}

/** Serves a universal XML-code generator.

//parser// is the parser information structure.

If //finished// is the TRUE: Parser generation has finished successful.
FALSE: Parser generation failed because of parse errors. */
void p_build_xml( PARSER* parser, BOOLEAN finished )
{
	XML_T			par;

	XML_T			code;
	XML_T			attrib;

	FILE* 			out					= stdout;
	char*			outname				= (char*)NULL;

	char*			xmlstr;

	PROC( "p_build_xml" );
	PARMS( "parser", "%p", parser );
	PARMS( "finished", "%s", BOOLEAN_STR( finished ) );

	/* Create root node */
	if( !( par = xml_new( "parser" ) ) )
		OUTOFMEM;

	/* UniCC version */
	xml_set_attr( par, "unicc-version", p_version( TRUE ) );

	/* Parser model */
	xml_set_attr( par, "mode", pmod[ parser->p_mode ] );

	/* Set general parser attributes */
	if( parser->p_name &&
			!( xml_set_attr( par, "name", parser->p_name ) ) )
		OUTOFMEM;
	if( parser->p_prefix &&
			!( xml_set_attr( par, "prefix", parser->p_prefix ) ) )
		OUTOFMEM;
	if( parser->filename &&
			!( xml_set_attr( par, "source", parser->filename ) ) )
		OUTOFMEM;
	if( parser->p_basename &&
			!( xml_set_attr( par, "basename", parser->p_basename ) ) )
		OUTOFMEM;
	if( parser->p_template &&
			!( xml_set_attr( par, "target-language", parser->p_template ) ) )
		OUTOFMEM;

	/* Set additional parser attributes */
	if( parser->p_version )
	{
		if( !( attrib = xml_add_child( par, "version", 0 ) ) )
			OUTOFMEM;

		if( !( xml_set_txt( attrib, parser->p_version ) ) )
			OUTOFMEM;
	}

	if( parser->p_copyright )
	{
		if( !( attrib = xml_add_child( par, "copyright", 0 ) ) )
			OUTOFMEM;

		if( !( xml_set_txt( attrib, parser->p_copyright ) ) )
			OUTOFMEM;
	}

	if( parser->p_desc )
	{
		if( !( attrib = xml_add_child( par, "description", 0 ) ) )
			OUTOFMEM;

		if( !( xml_set_txt( attrib, parser->p_desc ) ) )
			OUTOFMEM;
	}

	if( !xml_set_int_attr( par, "char-min", PCCL_MIN ) )
		OUTOFMEM;
	if( !xml_set_int_attr( par, "char-max", parser->p_universe - 1 ) )
		OUTOFMEM;

	/* Print parser's options */
	p_xml_print_options( parser->options, par );

	VARS( "finished", "%s", BOOLEAN_STR( finished ) );
	if( finished )
	{
		/* Build table of symbols --------------------------------------- */
		p_xml_print_symbols( parser, par );

		/* Build table of productions ----------------------------------- */
		p_xml_print_productions( parser, par );

		/* Build state table -------------------------------------------- */
		p_xml_print_states( parser, par );

		/* Build keyword/regular expression matching lexers ------------- */
		p_xml_print_lexers( parser, par );

		p_xml_print_vtypes( parser, par );
	}

	/* Put prologue code */
	if( !( code = xml_add_child( par, "prologue", 0 ) ) )
		OUTOFMEM;

	if( parser->p_header )
		xml_set_txt( code, parser->p_header );

	/* Put epilogue code */
	if( !( code = xml_add_child( par, "epilogue", 0 ) ) )
		OUTOFMEM;

	if( parser->p_footer )
		xml_set_txt( code, parser->p_footer );

	/* Put parser control block code */
	if( !( code = xml_add_child( par, "pcb", 0 ) ) )
		OUTOFMEM;

	if( parser->p_pcb )
		xml_set_txt( code, parser->p_pcb );

	/* Put entire parser source into XML output */
	if( !( code = xml_add_child( par, "source", 0 ) ) )
		OUTOFMEM;

	xml_set_txt( code, parser->source );

	/* Write error messages */
	if( parser->err_xml )
		xml_move( xml_child( parser->err_xml, "messages" ), par, 0 );

	/* Write to output file */
	if( !parser->to_stdout )
	{
		if( ( outname = pasprintf( "%s%s",
				parser->p_basename, UNICC_XML_EXTENSION ) ) )
		{
			if( !( out = fopen( outname, "wb" ) ) )
			{
				p_error( parser, ERR_OPEN_OUTPUT_FILE,
					ERRSTYLE_WARNING, outname );

				out = stdout;
			}
		}
	}

	if( out == stdout )
	{
		if( parser->files_count > 0 )
			fprintf( stdout, "%c", EOF );
	}

	if( ( xmlstr = xml_toxml( par ) ) )
	{
		fprintf( out, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" );
		fprintf( out, "<!DOCTYPE parser SYSTEM \"%s\">\n",
#ifndef _WIN32
#ifdef TLTDIR
			TLTDIR "/"
#endif
#endif
			"unicc.dtd"
					);

		parser->files_count++;
		fprintf( out, "%s", xmlstr );
	}
	else
		OUTOFMEM;

	pfree( xmlstr );
	xml_free( par );

	if( out != stdout )
		fclose( out );

	if( outname )
		pfree( outname );

	VOIDRET;
}

