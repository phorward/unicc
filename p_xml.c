/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator 
Copyright (C) 2006-2011 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com/ ++ unicc<<AT>>phorward-software<<DOT>>com

File:	p_build.c
Author:	Jan Max Meyer
Usage:	Builds the target parser based on configurations from a
		parser definition file.
		
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
 * Defines
 */
#define XML_YES		"yes"
#define XML_NO		"no"

/*
 * Global variables
 */
extern uchar*	pmod[];
extern BOOLEAN	first_progress;

/*
 * Functions
 */

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_xml_ccl()
	
	Author:			Jan Max Meyer
	
	Usage:			Dumps a character-class definition into an XML-structure.
					
	Parameters:		XML_T		parent_xml			Parent element where the
													character-class block will
													be attached to.
					CCL			ccl					The character-class.
	
	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
static void p_xml_ccl( XML_T parent_xml, CCL ccl )
{
	CCL		i;
	XML_T	ccl_xml;
	XML_T	range_xml;
	
	PROC( "p_xml_ccl" );
	PARMS( "parent_xml", "%p", parent_xml );
	PARMS( "ccl", "%p", ccl );
	
	if( !( ccl_xml = xml_add_child( parent_xml, "character-class", 0 ) ) )
		OUTOFMEM;
		
	xml_set_int_attr( ccl_xml, "count", ccl_count( ccl ) );
	
	for( i = ccl; i->begin != CCL_MAX; i++ )
	{
		if( !( range_xml = xml_add_child( ccl_xml, "range", 0 ) ) )
			OUTOFMEM;

		xml_set_int_attr( range_xml, "from", i->begin );
		xml_set_int_attr( range_xml, "to", i->end );
	}
	
	VOIDRET;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_xml_raw_code()
	
	Author:			Jan Max Meyer
	
	Usage:			Utility function that generates a raw-code-tag from a
					string.
					
	Parameters:		XML_T		code_xml			Parent element where the
													raw-code block will be 
													attached to.
					uchar*		code				The content of the raw
													code block.
	
	Returns:		XML_T							Raw-code tag.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
static XML_T p_xml_raw_code( XML_T code_xml, uchar* code )
{
	XML_T	raw_code;
	
	if( !code )
		OUTOFMEM;
	
	if( !( raw_code = xml_add_child( code_xml, "raw", 0 ) ) )
		OUTOFMEM;
		
	xml_set_txt_f( raw_code, code );
	
	return raw_code;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_xml_build_action()
	
	Author:			Jan Max Meyer
	
	Usage:			Constructs a XML-structure for a production's semantic
					action code, by splitting the original program code into
					XML-blocks, which can later be easier translated into the
					particular parser program.
					
	Parameters:		XML_T		code_xml			Code-XML-node where elements
													will be attached to.
					PARSER*		parser				Parser information structure
					PROD*		p					Production
					uchar*		base				Code-base template for the
													reduction action
					BOOLEAN		def_code			Defines if the base-pointer
													is a default-code block or
													an individually coded one.
	
	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	12.07.2010	Jan Max Meyer	Print warning if code symbol references to un-
								defined symbol on the semantic rhs!
----------------------------------------------------------------------------- */
static void p_xml_build_action( XML_T code_xml, PARSER* parser, PROD* p,
		uchar* base, BOOLEAN def_code )
{
	pregex			replacer;
	pregex_result*	result;
	int				result_cnt;
	int				i;
	int				off;
	uchar*			last;
	uchar*			ret		= (uchar*)NULL;
	uchar*			chk;
	uchar*			tmp;
	uchar*			att;
	LIST*			l;
	LIST*			m;
	LIST*			rhs			= p->rhs;
	LIST*			rhs_idents	= p->rhs_idents;
	BOOLEAN			on_error	= FALSE;
	SYMBOL*			sym;
	uchar*			raw;
	XML_T			code;
	
	PROC( "p_xml_build_action" );
	PARMS( "xml_code", "%p", xml_code );
	PARMS( "parser", "%p", parser );
	PARMS( "p", "%p", p );
	PARMS( "base", "%s", base );
	PARMS( "def_code", "%s", BOOLEAN_STR( def_code ) );
	
	/* Prepare regular expression engine */
	pregex_comp_init( &replacer, REGEX_MOD_GLOBAL );

	if( pregex_comp_compile( &replacer, "@'([^']|\\')*'", 0 ) != ERR_OK )
		VOIDRET;

	if( pregex_comp_compile( &replacer, "@\"([^\"]|\\\")*\"", 0 ) != ERR_OK )
		VOIDRET;

	if( pregex_comp_compile( &replacer, "@[A-Za-z_][A-Za-z0-9_]*", 1 )
			!= ERR_OK )
		VOIDRET;

	if( pregex_comp_compile( &replacer, "@[0-9]+", 2 ) != ERR_OK )
		VOIDRET;

	if( pregex_comp_compile( &replacer, "@@", 3 ) != ERR_OK )
		VOIDRET;
		
	/* Run regular expression */
	if( ( result_cnt = pregex_comp_match( &replacer, base,
							REGEX_NO_CALLBACK, &result ) ) < 0 )
	{
		MSG( "Error occured" );
		VARS( "result_cnt", "%d", result_cnt );
		VOIDRET;
	}
	else if( !result_cnt )
	{
		MSG( "Nothing to do at all" );
		p_xml_raw_code( code_xml, pstrdup( base ) );
		VOIDRET;
	}
	
	VARS( "result_cnt", "%d", result_cnt );
	
	/* Free the regular expression facilities - we have everything we 
		need from here! */
	pregex_comp_free( &replacer );
	
	VARS( "p->sem_rhs", "%p", p->sem_rhs  );
	/* Ok, perform replacement operations */
	if( p->sem_rhs )
	{
		MSG( "Replacing semantic right-hand side" );
		rhs = p->sem_rhs;
		rhs_idents = p->sem_rhs_idents;
	}
	
	MSG( "Iterating trough result array" );	
	for( i = 0, last = base; i < result_cnt && !on_error; i++ )
	{
		VARS( "i", "%d", i );
		off = 0;
		tmp = (uchar*)NULL;
		
		/* Copy raw part of code into its own tag */
		if( last < result[i].begin )
		{
			if( !( raw = pstr_append_nchar(
					(uchar*)NULL, last, result[i].begin - last ) ) )
				OUTOFMEM;
				
			p_xml_raw_code( code_xml, raw );
				
			VARS( "ret", "%s", ret );
			last = result[i].end;
		}
		
		VARS( "result[i].accept", "%d", result[i].accept );
		switch( result[i].accept )
		{
			case 0:
				result[i].begin++;
				result[i].len -= 2;

			case 1:
				MSG( "Identifier" );
				for( l = rhs_idents, m = rhs, off = 1; l && m;
						l = list_next( l ), m = list_next( m ), off++ )
				{
					chk = (uchar*)list_access( l );
					VARS( "chk", "%s", chk ? chk : "(NULL)" );
					
					/*
					printf( "check >%s< with >%.*s<\n",
						chk, result[i].len - 1, result[i].begin + 1 );
					*/
					if( chk && !pstrncmp( chk, result[i].begin + 1,
									result[i].len - 1 )
							&& pstrlen( chk ) == result[i].len - 1 )
					{
						break;
					}
				}
				
				if( !l )
				{
					p_error( ERR_UNDEFINED_SYMREF, ERRSTYLE_WARNING,
						result[i].begin + 1 );
					off = 0;
					
					tmp = p_strdup( result[i].begin );
				}
				
				VARS( "off", "%d", off );
				break;

			case 2:
				MSG( "Offset" );
				off = patoi( result[i].begin + 1 );
				break;

			case 3:
				MSG( "Left-hand side" );
								
				if( !( code = xml_add_child( code_xml, "variable", 0 ) ) )
					OUTOFMEM;

				if( !( xml_set_attr( code, "target", "left-hand-side" ) ) )
					OUTOFMEM;

				if( p->lhs->vtype && !( xml_set_attr( code, "value-type",
							p->lhs->vtype->real_def ) ) )
					OUTOFMEM;

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
					xml_set_attr( code, "value-type", sym->vtype->real_def );
			}
			else
			{
				if( !def_code )
				{
					p_error( ERR_NO_VALUE_TYPE, ERRSTYLE_FATAL,
							p_find_base_symbol( sym )->name,
								p->id, result[i].len + 1, result[i].begin );
				}

				on_error = TRUE;
			}
		}
		
		if( tmp )
			p_xml_raw_code( code_xml, tmp );
	}
		
	if( last && *last )				
		p_xml_raw_code( code_xml, pstrdup( last ) );
	
	MSG( "Free result array" );
	pfree( result );

	VOIDRET;
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
static void p_xml_build_scan_action(
	XML_T code_xml, PARSER* parser, SYMBOL* s, uchar* base )
{
	pregex			replacer;
	pregex_result*	result;
	int				result_cnt;
	uchar*			last;
	uchar*			raw;
	XML_T			code;
	int				i;
	
	PROC( "p_build_scan_action" );
	PARMS( "code_xml", "%p", code_xml );
	PARMS( "parser", "%p", parser );
	PARMS( "s", "%p", s );
	PARMS( "base", "%s", base );
	
	/* Prepare regular expression engine */
	pregex_comp_init( &replacer, REGEX_MOD_GLOBAL | REGEX_MOD_NO_ANCHORS );
	
	if( pregex_comp_compile( &replacer, "@>", 0 )
			!= ERR_OK )
		VOIDRET;

	if( pregex_comp_compile( &replacer, "@<", 1 )
			!= ERR_OK )
		VOIDRET;

	if( pregex_comp_compile( &replacer, "@@", 2 )
			!= ERR_OK )
		VOIDRET;
		
	/* Run regular expression */
	if( ( result_cnt = pregex_comp_match( &replacer, base,
							REGEX_NO_CALLBACK, &result ) ) < 0 )
	{
		MSG( "Error occured" );
		VARS( "result_cnt", "%d", result_cnt );
		VOIDRET;
	}
	else if( !result_cnt )
	{
		MSG( "Nothing to do at all" );
		p_xml_raw_code( code_xml, pstrdup( base ) );
		VOIDRET;
	}
	
	VARS( "result_cnt", "%d", result_cnt );
	
	/* Free the regular expression facilities - we have everything we 
		need from here! */
	pregex_comp_free( &replacer );
	
	MSG( "Iterating trough result array" );	
	for( i = 0, last = base; i < result_cnt; i++ )
	{
		VARS( "i", "%d", i );

		if( last < result[i].begin )
		{
			if( !( raw = pstr_append_nchar(
					(uchar*)NULL, last, result[i].begin - last ) ) )
				OUTOFMEM;
				
			p_xml_raw_code( code_xml, raw );
				
			VARS( "ret", "%s", ret );
		}

		last = result[i].end;
		
		VARS( "result[i].accept", "%d", result[i].accept );
		switch( result[i].accept )
		{
			case 0:
				MSG( "@>" );

				if( !( code = xml_add_child( code_xml,
						"begin-of-match", 0 ) ) )
					OUTOFMEM;

				break;

			case 1:
				MSG( "@<" );

				if( !( code = xml_add_child( code_xml,
						"end-of-match", 0 ) ) )
					OUTOFMEM;

				break;

			case 2:
				MSG( "@@" );
				
				if( !( code = xml_add_child( code_xml,
						"return-value", 0 ) ) )
					OUTOFMEM;
				
				if( s->vtype && list_count( parser->vtypes ) > 1 )
					xml_set_attr( code, "value-type", s->vtype->real_def );
				break;
				
			default:
				MSG( "Uncaught regular expression match!" );
				break;
		}
	}
		
	if( last && *last )
		p_xml_raw_code( code_xml, pstrdup( last ) );
	
	MSG( "Free result array" );
	pfree( result );
		
	VOIDRET;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_build_dfa()
	
	Author:			Jan Max Meyer
	
	Usage:			<usage>
					
	Parameters:		<type>		<identifier>		<description>
	
	Returns:		<type>							<description>
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
static void p_build_dfa( XML_T parent, pregex_dfa* dfa )
{
	pregex_dfa_st*	st;
	pregex_dfa_tr*	tr;
	int 			i;
	LIST*			l;
	LIST*			m;
	XML_T			state;
	XML_T			trans;

	PROC( "p_build_dfa" );

	for( l = dfa->states, i = 0; l; l = list_next( l ), i++ )
	{
		st = (pregex_dfa_st*)list_access( l );
		
		if( !( state = xml_add_child( parent, "state", 0 ) ) )
			OUTOFMEM;

		if( !( xml_set_int_attr( state, "id", i ) ) )
				OUTOFMEM;

		if( st->accept > REGEX_ACCEPT_NONE &&
			!( xml_set_int_attr( state, "accept", st->accept ) ) )
				OUTOFMEM;

		if( st->def_trans )
			if( !( xml_set_int_attr( state, "default-transition",
					st->def_trans->go_to ) ) )
				OUTOFMEM;
		
		LISTFOR( st->trans, m )
		{
			tr = (pregex_dfa_tr*)list_access( m );

			if( !( trans = xml_add_child( state, "transition", 0 ) ) )
				OUTOFMEM;
				
			if( !( xml_set_int_attr( trans, "goto", tr->go_to ) ) )
				OUTOFMEM;

			p_xml_ccl( trans, tr->ccl );
		}
	}

	VOIDRET;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_build_xml()
	
	Author:			Jan Max Meyer
	
	Usage:			Serves a universal XML-code generator.
					
	Parameters:		PARSER*		parser				Parser information structure
	
	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_build_xml( PARSER* parser )
{
	XML_T			par;

	XML_T			sym_tab;
	XML_T			symbol;
	
	XML_T			prod_tab;
	XML_T			prod;
	XML_T			lhs;
	XML_T			rhs;
	
	XML_T			state_tab;
	XML_T			state;
	XML_T			action;
	XML_T			go_to;
	XML_T			kernel;
	XML_T			epsilon;
	
	XML_T			lex_tab;
	XML_T			lex;
	XML_T			trans;

	XML_T			code;
	XML_T			attrib;
	XML_T			option;

	uchar*			transtype;
	
	FILE* 			out					= (FILE*)NULL;
	uchar*			outname;

	int				max_action			= 0;
	int				max_goto			= 0;
	int				max_dfa_idx			= 0;
	int				max_dfa_accept		= 0;
	int				max_symbol_name		= 0;
	int				column;
	int				row;
	int				st_lex;
	LIST*			l;
	LIST*			m;
	LIST*			n;
	pregex_dfa*		dfa;
	pregex_dfa_st*	dfa_st;
	pregex_dfa_tr*	dfa_ent;
	uchar*			tmp;
	uchar*			act;
	SYMBOL*			sym;
	STATE*			st;
	TABCOL*			col;
	PROD*			p;
	PROD*			goalprod;
	VTYPE*			vt;
	ITEM*			item;
	CCL				c;
	int				i;
	int				j;
	BOOLEAN			is_default_code;
	uchar*			xmlstr;
	pregex_dfa		tmp_dfa;
	HASHELEM*		he;

	PROC( "p_build_xml" );
	PARMS( "root", "%p", root );
	PARMS( "parser", "%p", parser );

	/* Create root node */
	if( !( par = xml_new( "parser" ) ) )
		OUTOFMEM;

	/* Parser model */
	xml_set_attr( par, "model", pmod[ parser->p_model ] );
		
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
	if( parser->p_language &&
			!( xml_set_attr( par, "target-language", parser->p_language ) ) )
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

	if( !xml_set_int_attr( par, "char-min", CCL_MIN ) )
		OUTOFMEM;
	if( !xml_set_int_attr( par, "char-max", CCL_MAX - 1 ) )
		OUTOFMEM;

	/* Set parser options */
	for( l = hashtab_list( &( parser->options ) ); l; l = list_next( l ) )
	{
		he = (HASHELEM*)list_access( l );

		if( !( option = xml_add_child( par, "option", 0 ) ) )
			OUTOFMEM;

		if( !xml_set_attr( option, "name", hashelem_key( he ) ) )
			OUTOFMEM;
		
		if( !( xml_set_txt( option, (uchar*)hashelem_access( he ) ) ) )
			OUTOFMEM;
	}

	/* Build table of symbols ----------------------------------------------- */
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
						tmp = "keyword";
					else
					{
						tmp = "regular-expression";

						pregex_dfa_from_nfa( &tmp_dfa, &( sym->nfa ) );
						pregex_dfa_minimize( &tmp_dfa );

						if( !( lex = xml_add_child( symbol, "dfa", 0 ) ) )
							OUTOFMEM;

						p_build_dfa( lex, &tmp_dfa );
						pregex_dfa_free( &tmp_dfa );
					}
					break;
				case SYM_ERROR_RESYNC:
					tmp = "error-resynchronization";
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
			
			/* Code (in case of regex/keyword terminals */
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
			xml_set_attr( symbol, "value-type", sym->vtype->real_def );
			
		if( sym->line > 0 )
			xml_set_int_attr( symbol, "defined-at", sym->line );
	
		/* Set symbol options */
		for( m = hashtab_list( &( sym->options ) ); m; m = list_next( m ) )
		{
			he = (HASHELEM*)list_access( m );

			if( !( option = xml_add_child( symbol, "option", 0 ) ) )
				OUTOFMEM;

			if( !xml_set_attr( option, "name", hashelem_key( he ) ) )
				OUTOFMEM;
			
			if( !( xml_set_txt( option, (uchar*)hashelem_access( he ) ) ) )
				OUTOFMEM;
		}
	}
	
	/* Build table of productions ------------------------------------------- */
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
			
			if( ( tmp = (uchar*)list_getptr( p->sem_rhs_idents, i ) ) )
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
			
			if( ( tmp = (uchar*)list_getptr( p->rhs_idents, j ) ) )
				xml_set_attr( rhs, "named", tmp );
		}

		/* Set productions options */
		for( m = hashtab_list( &( p->options ) ); m; m = list_next( m ) )
		{
			he = (HASHELEM*)list_access( m );

			if( !( option = xml_add_child( prod, "option", 0 ) ) )
				OUTOFMEM;

			if( !xml_set_attr( option, "name", hashelem_key( he ) ) )
				OUTOFMEM;
			
			if( !( xml_set_txt( option, (uchar*)hashelem_access( he ) ) ) )
				OUTOFMEM;
		}
		
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
			act = (uchar*)NULL;
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

	/* Build state table ---------------------------------------------------- */
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
			
			/* Shift, reduce or shift&reduce? */
			switch( col->action )
			{
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
			if( col->action & REDUCE )
				xml_set_int_attr( action, "by-production", col->index );
			else
				xml_set_int_attr( action, "to-state", col->index );
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
	
	/* Build keyword/regular expression matching lexers --------------------- */
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
	
	/* Write to output file */
	if( outname = pasprintf( "%s.xml", parser->p_basename ) )
	{
		if( !( out = fopen( outname, "wb" ) ) )
		{
			p_error( ERR_OPEN_OUTPUT_FILE, ERRSTYLE_WARNING, outname );
		}
	}
	
	if( !out )
	{	
		out = stdout;
		first_progress = FALSE;
	}
	
	if( ( xmlstr = xml_toxml( par ) ) )
		fprintf( out, "%s", xmlstr );
	
	xml_free( par );
	
	if( out )
		fclose( out );

	if( outname )
		pfree( outname );

	VOIDRET;
}

