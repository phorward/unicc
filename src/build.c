/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
https://phorward.info ++ unicc<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	build.c
Author:	Jan Max Meyer
Usage:	The dynamic program module generator of the UniCC parser generator,
		to construct a parser program module in a specific programming language
		using a template.
----------------------------------------------------------------------------- */

#include "unicc.h"

#define	LEN_EXT		"_len"
#define SYMBOL_VAR	"symbol"

extern FILE*	status;

static plex*	action_lex;
static plex*	scan_lex;

/** Escapes the input-string according to the parser templates
escaping-sequence definitions. This function is used to print identifiers and
character-class definitions to the target parser without getting trouble with
the target language's escape characters (e.g. as in C - I love C!!!).

//g// is the generator template structure,
//str// is the source string
//clear// is, if TRUE, str will be free'd, else not

Returns a char*, which is the final (escaped) string.
*/
char* escape_for_target( GENERATOR* g, char* str, BOOLEAN clear )
{
	int		i;
	char*	ret;
	char*	tmp;

	if( !( ret = pstrdup( str ) ) )
		ret = pstrdup( "" );

	if( clear )
		str = pfree( str );

	for( i = 0; i < g->sequences_count; i++ )
	{
		if( !( tmp = pstrrender( ret, g->for_sequences[ i ],
				g->do_sequences[ i ], FALSE, (char*)NULL ) ) )
			OUTOFMEM;

		pfree( ret );
		ret = tmp;
	}

	return ret;
}

/** Constructs target language code for production reduction code blocks.

//parser// is the parser information structure,
//g// is the generator template structure
//p// is the production
//base// is the code-base template for the reduction action
//def_code// are the defines if the base-pointer is a default-code block or an
individually coded one.

Returns a char*-pointer to the generated code - must be freed by the caller.
*/
char* build_action( PARSER* parser, GENERATOR* g, PROD* p,
			char* base, BOOLEAN def_code )
{
	char*			last			= base;
	char*			start;
	char*			end;
	int				off;
	unsigned int	match;
	char*			ret				= (char*)NULL;
	char*			chk;
	char*			tmp;
	char*			att;
	plistel*		e;
	plist*			rhs				= p->rhs;
	BOOLEAN			on_error		= FALSE;
	SYMBOL*			sym;
	int				i;
	char			rx				[ ONE_LINE + 1 ];
	char*			rx_postfix[]	= {
										"'([^']|\\')*'",
										"\"([^\"]|\\\")*\"",
										"[A-Za-z_][A-Za-z0-9_]*",
										"[0-9]+",
										"@",
										"!" SYMBOL_VAR ":[A-Za-z_][A-Za-z0-9_]*"
									};

	PROC( "build_action" );
	PARMS( "parser", "%p", parser );
	PARMS( "g", "%p", g );
	PARMS( "p", "%p", p );
	PARMS( "base", "%s", base );
	PARMS( "def_code", "%s", BOOLEAN_STR( def_code ) );

	/* Once generate a lexer */
	if( !action_lex )
	{
		/* Prepare regular expression engine */
		action_lex = plex_create( 0 );

		for( i = 0; i < 6; i++ )
		{
			if( pstrlen( g->prefix ) + pstrlen( rx_postfix[ i ] )
					>= sizeof( rx ) )
			{
				MSG( "Buffer too small!" );
				RETURN( (char*)NULL );
			}

			sprintf( rx, "%s%s", g->prefix, rx_postfix[ i ] );

			VARS( "i", "%d", i );
			VARS( "rx", "%s", rx );

			/* Watch out: First two regex match same id! */
			if( !plex_define( action_lex, rx, !i ? 1 : i, 0 ) )
			{
				MSG( "Something went wrong with the action lexer definition" );
				RETURN( (char*)NULL );
			}
		}
	}

	VARS( "p->sem_rhs counts", "%d", plist_count( p->sem_rhs ) );

	/* Ok, perform replacement operations */
	if( plist_count( p->sem_rhs ) && !def_code )
	{
		MSG( "Replacing semantic right-hand side" );
		rhs = p->sem_rhs;
	}

	MSG( "Iterating trough matches" );

	while( ( start = plex_next( action_lex, last, &match, &end ) )
				&& !on_error )
	{
		off = 0;
		tmp = (char*)NULL;

		if( last < start )
		{
			if( !( ret = pstrncatstr( ret, last, start - last ) ) )
				OUTOFMEM;

			VARS( "ret", "%s", ret );
		}

		last = end;

		VARS( "match", "%d", match );
		switch( match )
		{
			case 1:
				start += 1;
				end -= 1;

			case 2:
				MSG( "Identifier" );
				for( e = plist_first( rhs ), off = 1; e;
						e = plist_next( e ), off++ )
				{
					chk = plist_key( e );
					VARS( "chk", "%s", chk ? chk : "(NULL)" );

					if( chk && !strncmp( chk, start + pstrlen( g->prefix ),
									end - start - pstrlen( g->prefix ) )
							&& pstrlen( chk ) ==
									end - start - pstrlen( g->prefix ) )
					{
						break;
					}
				}

				if( !e )
				{
					print_error( parser, ERR_UNDEFINED_SYMREF,
									ERRSTYLE_WARNING, end - start, start );
					off = 0;
					tmp = pstrdup( start );
				}

				VARS( "off", "%d", off );
				break;

			case 3:
				MSG( "Offset" );
				off = atoi( start + pstrlen( g->prefix ) );
				break;

			case 4:
				MSG( "Left-hand side" );
				if( p->lhs->vtype && list_count( parser->vtypes ) > 1 )
					ret = pstrcatstr( ret,
							pstrrender( g->action_lhs_union,
								GEN_WILD_PREFIX "attribute",
									pstrrender( g->vstack_union_att,
										GEN_WILD_PREFIX "value-type-id",
											int_to_str( p->lhs->vtype->id ),
												TRUE,
										(char*)NULL ), TRUE,
								(char*)NULL ), TRUE );
				else
					ret = pstrcatstr( ret, g->action_lhs_single, FALSE );

				VARS( "ret", "%s", ret );
				break;

			case 5:
				MSG( "Assign left-hand side symbol" );
				off = 0;

				if( !( tmp = pasprintf( "%.*s",
								end - start - ( pstrlen( SYMBOL_VAR )
										+ 2 + pstrlen( g->prefix ) ),
								start + ( pstrlen( SYMBOL_VAR )
										+ 2 + pstrlen( g->prefix ) )
					) ) )
				{
					OUTOFMEM;
					RETURN( (char*)NULL );
				}

				VARS( "tmp", "%s", tmp );

				/* Go through all possible left-hand side symbols */
				plist_for( p->all_lhs, e )
				{
					sym = (SYMBOL*)plist_access( e );

					if( !strcmp( sym->name, tmp ) )
					{
						MSG( "Found a matching symbol!" );

						pfree( tmp );
						tmp = (char*)NULL;

						ret = pstrcatstr( ret,
								pstrrender( g->action_set_lhs,
									GEN_WILD_PREFIX "sym",
										int_to_str( sym->id ), TRUE,
											(char*)NULL ), TRUE );
						break;
					}
				}

				if( !e )
				{
					MSG( "No match found..." );

					print_error( parser, ERR_UNDEFINED_LHS,
									ERRSTYLE_WARNING, tmp );
					pfree( tmp );

					if( !( tmp = pstrdup( start ) ) )
					{
						OUTOFMEM;
						RETURN( (char*)NULL );
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
			MSG( "Handling offset" );
			sym = (SYMBOL*)plist_access( plist_get( rhs, off - 1 ) );

			if( sym && !( sym->keyword ) )
			{
				if( list_count( parser->vtypes ) > 1 )
				{
					if( sym->vtype )
					{
						att  = pstrrender( g->vstack_union_att,
							GEN_WILD_PREFIX "value-type-id",
								int_to_str( sym->vtype->id ), TRUE,
							(char*)NULL );
					}
					else
					{
						print_error( parser, ERR_NO_VALUE_TYPE, ERRSTYLE_FATAL,
								sym->name, p->id, end - start + 1, start );

						att = (char*)NULL;
						on_error = TRUE;
					}

					tmp = pstrrender( g->action_union,
						GEN_WILD_PREFIX "offset",
							int_to_str( plist_count( rhs ) - off ), TRUE,
						GEN_WILD_PREFIX "attribute", att, TRUE,
						(char*)NULL );
				}
				else
					tmp = pstrrender( g->action_single,
						GEN_WILD_PREFIX "offset",
							int_to_str( plist_count( rhs ) - off ), TRUE,
						(char*)NULL );
			}
			else
			{
				if( !def_code )
				{
					print_error( parser, ERR_NO_VALUE_TYPE, ERRSTYLE_FATAL,
							find_base_symbol( sym )->name,
								p->id, end - start + 1, start );
				}

				on_error = TRUE;
			}
		}

		if( tmp )
			ret = pstrcatstr( ret, tmp, TRUE );
	}

	if( last && *last )
		ret = pstrcatstr( ret, last, FALSE );

	VARS( "ret", "%s", ret );
	VARS( "on_error", "%s", BOOLEAN_STR( on_error ) );

	if( on_error && ret )
	{
		MSG( "Okay, on error, everything will be deleted!" );
		pfree( ret );
		ret = (char*)NULL;
	}

	RETURN( ret );
}

/** Construct the scanner action code from templates. */
char* build_scan_action( PARSER* parser, GENERATOR* g, SYMBOL* s, char* base )
{
	char*			last		= base;
	char*			start;
	char*			end;
	unsigned int	match;
	char*			ret			= (char*)NULL;
	char*			tmp;
	plistel*		e;
	SYMBOL*			sym;

	int				i;
	char			rx				[ ONE_LINE + 1 ];
	char*			rx_postfix[]	= {
										">",
										"<",
										"@",
										"!" SYMBOL_VAR ":[A-Za-z_][A-Za-z0-9_]*"
									};

	PROC( "build_scan_action" );
	PARMS( "parser", "%p", parser );
	PARMS( "g", "%p", g );
	PARMS( "s", "%p", s );
	PARMS( "base", "%s", base );


	/* Once generate a lexer */
	if( !scan_lex )
	{
		/* Prepare regular expression engine */
		scan_lex = plex_create( 0 );

		for( i = 0; i < 4; i++ )
		{
			if( pstrlen( g->prefix ) + pstrlen( rx_postfix[ i ] )
					>= sizeof( rx ) )
			{
				MSG( "Buffer too small!" );

				plex_free( scan_lex );
				RETURN( (char*)NULL ); /* Not the best way to handle this */
			}

			sprintf( rx, "%s%s", g->prefix, rx_postfix[ i ] );

			VARS( "i", "%d", i );
			VARS( "rx", "%s", rx );

			if( !plex_define( scan_lex, rx, i + 1, 0 ) )
			{
				plex_free( scan_lex );
				MSG( "Something went wrong with the action lexer definition" );
				RETURN( (char*)NULL );
			}
		}
	}

	MSG( "Iterating trough matches" );

	while( ( start = plex_next( scan_lex, last, &match, &end ) ) )
	{
		if( last < start )
		{
			if( !( ret = pstrncatstr(
					ret, last, start - last ) ) )
				OUTOFMEM;

			VARS( "ret", "%s", ret );
		}

		last = end;

		VARS( "match", "%d", match );
		switch( match )
		{
			case 1:
				MSG( "@>" );
				ret = pstrcatstr( ret,
					g->scan_action_begin_offset, FALSE );
				break;

			case 2:
				MSG( "@<" );
				ret = pstrcatstr( ret,
					g->scan_action_end_offset, FALSE );

				break;

			case 3:
				MSG( "@@" );
				if( s->vtype && list_count( parser->vtypes ) > 1 )
					ret = pstrcatstr( ret,
							pstrrender( g->scan_action_ret_union,
								GEN_WILD_PREFIX "attribute",
									pstrrender( g->vstack_union_att,
										GEN_WILD_PREFIX "value-type-id",
											int_to_str( s->vtype->id ), TRUE,
										(char*)NULL ), TRUE,
								(char*)NULL ), TRUE );
				else
					ret = pstrcatstr( ret,
							g->scan_action_ret_single, FALSE );
				break;

			case 4:
				MSG( "Set terminal symbol" );

				if( !( tmp = pasprintf( "%.*s",
								end - start - ( pstrlen( SYMBOL_VAR )
										+ 2 + pstrlen( g->prefix ) ),
								start + ( pstrlen( SYMBOL_VAR )
										+ 2 + pstrlen( g->prefix ) )
					) ) )
					OUTOFMEM;

				VARS( "tmp", "%s", tmp );

				/* Go through all possible terminal symbols */
				plist_for( s->all_sym, e )
				{
					sym = (SYMBOL*)plist_access( e );

					if( !strcmp( sym->name, tmp ) )
					{
						MSG( "Found a matching symbol!" );
						ret = pstrcatstr( ret,
								pstrrender( g->scan_action_set_symbol,
									GEN_WILD_PREFIX "sym",
										int_to_str( sym->id ), TRUE,
											(char*)NULL ), TRUE );
						break;
					}
				}

				if( !e )
				{
					MSG( "No match found..." );

					print_error( parser, ERR_UNDEFINED_TERMINAL,
								ERRSTYLE_WARNING, tmp );
				}

				pfree( tmp );
				break;

			default:
				MSG( "Uncaught regular expression match!" );
				break;
		}

		VARS( "ret", "%s", ret );
	}

	if( last && *last )
		ret = pstrcatstr( ret, last, FALSE );

	VARS( "ret", "%s", ret );
	RETURN( ret );
}

/** Converts a production into a dynamic string.

//p// is the production pointer

Returns a generated string.
*/
char* mkproduction_str( PROD* p )
{
	char*		ret;
	char		wtf		[ 512 ];	/* yes, it stands for 'what the f...' */
	plistel*	e;
	SYMBOL*		sym;

	sprintf( wtf, "%.*s : ", (int)sizeof( wtf ) - 5, p->lhs->name );
	ret = pstrdup( wtf );

	plist_for( p->rhs, e )
	{
		sym = (SYMBOL*)plist_access( e );

		switch( sym->type )
		{
			case SYM_CCL_TERMINAL:
				sprintf( wtf, "\'%.*s\'", (int)sizeof( wtf ) - 4,
								sym->name );
				break;

			case SYM_REGEX_TERMINAL:
				if( sym->keyword )
					sprintf( wtf, "\"%.*s\"", (int)sizeof( wtf ) - 4,
								sym->name );
				else
					sprintf( wtf, "@%.*s", (int) sizeof( wtf ) - 3,
								sym->name );
				break;

			case SYM_SYSTEM_TERMINAL:
				sprintf( wtf, "~%s", sym->name );
				break;

			default:
				sprintf( wtf, "%.*s", (int)sizeof( wtf ) - 2,
							sym->name );
				break;
		}

		if( plist_next( e ) )
			strcat( wtf, " " );

		ret = pstrcatstr( ret, wtf, FALSE );
	}

	return ret;
}

/** Loads a XML-defined code generator into an adequate GENERATOR structure.
Pointers are only set to the values mapped to the XML-structure, so no memory
is wasted.

//parser// is the parser information structure.
//g// is the target generator.
//genfile// is the path to generator file.

Returns TRUE on success, FALSE on error.
*/
BOOLEAN load_generator( PARSER* parser, GENERATOR* g, char* genfile )
{
	char*	name;
	char*	version;
	char*	lname;
	XML_T	tmp;
	char*	att_for;
	char*	att_do;
	int		i;

#define GET_XML_DEF( source, target, tagname ) \
	if( xml_child( (source), (tagname) ) ) \
		(target) = (char*)( xml_txt( xml_child( (source), (tagname) ) ) ); \
	else \
		print_error( parser, ERR_TAG_NOT_FOUND, ERRSTYLE_WARNING, \
			(tagname), genfile );

#define GET_XML_TAB_1D( target, tagname ) \
	if( ( tmp = xml_child( g->xml, (tagname) ) ) ) \
	{ \
		GET_XML_DEF( tmp, (target).col, "col" ) \
		GET_XML_DEF( tmp, (target).col_sep, "col_sep" ) \
	} \
	else \
		print_error( parser, ERR_TAG_NOT_FOUND, ERRSTYLE_WARNING, \
			(tagname), genfile );

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
		print_error( parser, ERR_TAG_NOT_FOUND, ERRSTYLE_WARNING, \
			(tagname), genfile );

	if( !( g->xml = xml_parse_file( genfile ) ) )
	{
		print_error( parser, ERR_NO_GENERATOR_FILE, ERRSTYLE_FATAL, genfile );
		return FALSE;
	}

	if( *xml_error( g->xml ) )
	{
		print_error( parser, ERR_XML_ERROR, ERRSTYLE_FATAL,
			genfile, xml_error( g->xml ) );
		return FALSE;
	}

	if( ! *( g->truedef = (char*)xml_txt( xml_child( g->xml, "true" ) ) ) )
		g->truedef = "1";

	if( ! *( g->falsedef = (char*)xml_txt( xml_child( g->xml, "false" ) ) ) )
		g->falsedef = "0";

	GET_XML_DEF( g->xml, g->vstack_def_type, "vstack_def_type" );
	GET_XML_DEF( g->xml, g->vstack_term_type, "vstack_term_type" );

	if( ! *( g->prefix = (char*)xml_txt(
				xml_child( g->xml, "action_prefix" ) ) ) )
		g->prefix = "@";

	GET_XML_DEF( g->xml, g->action_start, "action_start" );
	GET_XML_DEF( g->xml, g->action_end, "action_end" );
	GET_XML_DEF( g->xml, g->action_single, "action_single" );
	GET_XML_DEF( g->xml, g->action_union, "action_union" );
	GET_XML_DEF( g->xml, g->action_lhs_single, "action_lhs_single" );
	GET_XML_DEF( g->xml, g->action_lhs_union, "action_lhs_union" );
	GET_XML_DEF( g->xml, g->action_set_lhs, "action_set_lhs" );

	GET_XML_DEF( g->xml, g->scan_action_start, "scan_action_start" );
	GET_XML_DEF( g->xml, g->scan_action_end, "scan_action_end" );
	GET_XML_DEF( g->xml, g->scan_action_begin_offset,
					"scan_action_begin_offset" );
	GET_XML_DEF( g->xml, g->scan_action_end_offset, "scan_action_end_offset" );
	GET_XML_DEF( g->xml, g->scan_action_ret_single, "scan_action_ret_single" );
	GET_XML_DEF( g->xml, g->scan_action_ret_union, "scan_action_ret_union" );
	GET_XML_DEF( g->xml, g->scan_action_set_symbol, "scan_action_set_symbol" );

	GET_XML_DEF( g->xml, g->vstack_single, "vstack_single" );
	GET_XML_DEF( g->xml, g->vstack_union_start, "vstack_union_start" );
	GET_XML_DEF( g->xml, g->vstack_union_end, "vstack_union_end" );
	GET_XML_DEF( g->xml, g->vstack_union_def, "vstack_union_def" );
	GET_XML_DEF( g->xml, g->vstack_union_att, "vstack_union_att" );

	GET_XML_TAB_1D( g->defprod, "defprod" )

	GET_XML_TAB_1D( g->dfa_select, "dfa_select" )
	GET_XML_TAB_1D( g->dfa_char, "dfa_char" )
	GET_XML_TAB_1D( g->dfa_trans, "dfa_trans" )

	GET_XML_TAB_2D( g->acttab, "acttab" )
	GET_XML_TAB_2D( g->gotab, "gotab" )
	GET_XML_TAB_2D( g->dfa_idx, "dfa_idx" )
	GET_XML_TAB_2D( g->dfa_accept, "dfa_accept" )

	GET_XML_TAB_1D( g->symbols, "symbols" )
	GET_XML_TAB_1D( g->productions, "productions" )

	GET_XML_DEF( g->xml, g->code_localization, "code_localization" );

	/* Escape sequence definitions */
	for( tmp = xml_child( g->xml, "escape-sequence" ); tmp;
			tmp = xml_next( tmp ) )
	{
		att_for = (char*)xml_attr( tmp, "for" );
		att_do = (char*)xml_attr( tmp, "do" );

		if( att_for && att_do )
		{
			for( i = 0; i < g->sequences_count; i++ )
			{
				if( !strcmp( g->for_sequences[ i ], att_for ) )
				{
					print_error( parser, ERR_DUPLICATE_ESCAPE_SEQ,
									ERRSTYLE_WARNING, att_for, genfile );
					break;
				}
			}

			if( i < g->sequences_count )
				continue;

			g->for_sequences = (char**)prealloc( (char**)g->for_sequences,
					( g->sequences_count + 1 ) * sizeof( char* ) );
			g->do_sequences = (char**)prealloc( (char**)g->do_sequences,
					( g->sequences_count + 1 ) * sizeof( char* ) );

			if( !( g->for_sequences && g->do_sequences ) )
				OUTOFMEM;

			g->for_sequences[ g->sequences_count ] = (char*)( att_for );
			g->do_sequences[ g->sequences_count ] = (char*)( att_do );

			if( !( g->for_sequences[ g->sequences_count ]
				&& g->do_sequences[ g->sequences_count ] ) )
				OUTOFMEM;

			g->sequences_count++;
		}
		else
		{
			if( !att_for )
				print_error( parser, ERR_XML_INCOMPLETE, ERRSTYLE_FATAL,
					genfile, xml_name( tmp ), "for" );
			if( !att_do )
				print_error( parser, ERR_XML_INCOMPLETE, ERRSTYLE_FATAL,
					genfile, xml_name( tmp ), "do" );
		}
	}

	/* Output some more information */
	if( parser->verbose )
	{
		name = xml_attr( g->xml, "name" );
		version = xml_attr( g->xml, "version" );
		if( !( lname = xml_attr( g->xml, "long-name" ) ) )
			lname = name;

		if( lname && *lname && version && *version )
		{
			fprintf( status, "[%s, v%s]...", lname, version );
			fflush( status );
		}
	}

	return TRUE;
}


/** This is the main function for the code-generator. It first reads a target
language generator, and then constructs code segments, which are finally pasted
into the parser template (which is defined within the <driver>-tag of the
generator file).

//parser// is the parser information structure.
*/
void build_code( PARSER* parser )
{
	GENERATOR		generator;
	GENERATOR*		gen					= (GENERATOR*)NULL;
	XML_T			file;
	FILE*			stream;

	char*			basename;
	char			tlt_file			[ BUFSIZ + 1 ];
	char*			tlt_path;
	char*			option;
	char*			complete			= (char*)NULL;
	char*			all					= (char*)NULL;
	char*			action_table		= (char*)NULL;
	char*			action_table_row	= (char*)NULL;
	char*			goto_table			= (char*)NULL;
	char*			goto_table_row		= (char*)NULL;
	char*			def_prod			= (char*)NULL;
	char*			char_map			= (char*)NULL;
	char*			char_map_sym		= (char*)NULL;
	char*			symbols				= (char*)NULL;
	char*			productions			= (char*)NULL;
	char*			dfa_select			= (char*)NULL;
	char*			dfa_idx				= (char*)NULL;
	char*			dfa_idx_row			= (char*)NULL;
	char*			dfa_char			= (char*)NULL;
	char*			dfa_trans			= (char*)NULL;
	char*			dfa_accept			= (char*)NULL;
	char*			dfa_accept_row		= (char*)NULL;
	char*			type_def			= (char*)NULL;
	char*			actions				= (char*)NULL;
	char*			scan_actions		= (char*)NULL;
	char*			top_value			= (char*)NULL;
	char*			goal_value			= (char*)NULL;
	char*			act					= (char*)NULL;
	char*			filename			= (char*)NULL;

	int				max_action			= 0;
	int				max_goto			= 0;
	int				max_dfa_idx			= 0;
	int				max_dfa_accept		= 0;
	int				max_symbol_name		= 0;
	int				column;
	int				charmap_count		= 0;
	int				row;
	pregex_dfa*		dfa;
	pregex_dfa_st*	dfa_st;
	pregex_dfa_tr*	dfa_ent;
	SYMBOL*			sym;
	STATE*			st;
	TABCOL*			col;
	PROD*			p;
	PROD*			goalprod;
	VTYPE*			vt;
	wchar_t			beg;
	wchar_t			end;
	int				i;
	BOOLEAN			is_default_code;
	plistel*		e;
	plistel*		f;
	LIST*			l;
	LIST*			m;

	PROC( "build_code" );
	PARMS( "parser", "%p", parser );

	gen = &generator;
	memset( gen, 0, sizeof( GENERATOR ) );

	sprintf( tlt_file, "%s%s", parser->p_template, UNICC_TLT_EXTENSION );
	pstrlwr( tlt_file );
	VARS( "tlt_file", "%s", tlt_file );

	if( !( tlt_path = pwhich( tlt_file, "targets" ) )
		&& !( tlt_path = pwhich( tlt_file, getenv( "UNICC_TPLDIR" ) ) )
#ifndef _WIN32
			&& !( tlt_path = pwhich( tlt_file,
#ifdef TLTDIR
			TLTDIR
#else
			"/usr/share/unicc/targets"
#endif
			) )
#endif
		)
	{
		tlt_path = tlt_file;
	}

	VARS( "tlt_path", "%s", tlt_path );

	MSG( "Loading generator" );
	if( !load_generator( parser, gen, tlt_path ) )
		VOIDRET;

	/* Now that we have the generator, do some code generation-related
		integrity preparatories on the grammar */

	MSG( "Performing code generation-related integrity preparatories" );
	plist_for( parser->symbols, e )
	{
		sym = (SYMBOL*)plist_access( e );

		if( !( sym->vtype ) )
			sym->vtype = parser->p_def_type;

		if( sym->type == SYM_NON_TERMINAL && !( sym->vtype ) &&
			( gen->vstack_def_type && *( gen->vstack_def_type ) ) )
			sym->vtype = create_vtype( parser, gen->vstack_def_type );
		else if( IS_TERMINAL( sym ) /* && !( sym->keyword ) */
			&& !( sym->vtype ) && ( gen->vstack_term_type &&
				*( gen->vstack_term_type ) ) )
			sym->vtype = create_vtype( parser, gen->vstack_term_type );
	}

	/* Create piece of code for the value at the top of the value stack
		(e.g. used to store the next terminal character onto the value stack) */
	if( list_count( parser->vtypes ) <= 1 )
		top_value = pstrrender( gen->action_single,
			GEN_WILD_PREFIX "offset", int_to_str( 0 ), TRUE,
				(char*)NULL );
	else if( ( vt = find_vtype( parser, gen->vstack_term_type ) ) )
			top_value = pstrrender( gen->action_union,
				GEN_WILD_PREFIX "offset", int_to_str( 0 ), TRUE,
					GEN_WILD_PREFIX "attribute",
						pstrrender( gen->vstack_union_att,
							GEN_WILD_PREFIX "value-type-id",
									int_to_str( vt->id ), TRUE,
										(char*)NULL ), TRUE,
											(char*)NULL );
	else
		/* ERROR */
		;

	/* Create piece of code for the value that is associated with the
	 * 	goal symbol, to e.g. return it from the parser function */
	if( list_count( parser->vtypes ) <= 1 )
		goal_value = pstrrender( gen->action_single,
			GEN_WILD_PREFIX "offset", int_to_str( 0 ), TRUE,
				(char*)NULL );
	else
		goal_value = pstrrender( gen->action_union,
			GEN_WILD_PREFIX "offset", int_to_str( 0 ), TRUE,
				GEN_WILD_PREFIX "attribute",
					pstrrender( gen->vstack_union_att,
						GEN_WILD_PREFIX "value-type-id",
								int_to_str( parser->goal->vtype->id ), TRUE,
									(char*)NULL ), TRUE,
										(char*)NULL );

	/* Build action, goto and dfa_select tables */
	MSG( "Action, Goto and DFA selection table" );

	i = 0;
	parray_for( parser->states, st )
	{
		/* Action table */
		action_table_row = pstrrender( gen->acttab.row_start,
				GEN_WILD_PREFIX "number-of-columns",
					int_to_str( list_count( st->actions ) ), TRUE,
				GEN_WILD_PREFIX "state-number",
					int_to_str( st->state_id ), TRUE,
						(char*)NULL );

		if( max_action < list_count( st->actions ) )
			max_action = list_count( st->actions );

		for( m = st->actions, column = 0; m; m = m->next, column++ )
		{
			col = (TABCOL*)(m->pptr);

			action_table_row = pstrcatstr( action_table_row,
				pstrrender( gen->acttab.col,
					GEN_WILD_PREFIX "symbol",
							int_to_str( col->symbol->id ), TRUE,
					GEN_WILD_PREFIX "action", int_to_str( col->action ), TRUE,
					GEN_WILD_PREFIX "index", int_to_str( col->index ), TRUE,
					GEN_WILD_PREFIX "column", int_to_str( column ), TRUE,
						(char*)NULL ), TRUE );

			if( m->next )
				action_table_row = pstrcatstr( action_table_row,
					gen->acttab.col_sep, FALSE );
		}

		action_table_row = pstrcatstr( action_table_row,
				pstrrender( gen->acttab.row_end,
					GEN_WILD_PREFIX "number-of-columns",
						int_to_str( list_count( st->actions ) ), TRUE,
					GEN_WILD_PREFIX "state-number",
						int_to_str( st->state_id ), TRUE,
							(char*)NULL ), TRUE );

		if( parray_next( parser->states, st ) )
			action_table_row = pstrcatstr( action_table_row,
				gen->acttab.row_sep, FALSE );

		action_table = pstrcatstr( action_table, action_table_row, TRUE );

		/* Goto table */
		goto_table_row = pstrrender( gen->gotab.row_start,
				GEN_WILD_PREFIX "number-of-columns",
					int_to_str( list_count( st->gotos ) ), TRUE,
				GEN_WILD_PREFIX "state-number",
					int_to_str( st->state_id ), TRUE,
						(char*)NULL );

		if( max_goto < list_count( st->gotos ) )
			max_goto = list_count( st->gotos );

		for( m = st->gotos, column = 0; m; m = m->next, column++ )
		{
			col = (TABCOL*)(m->pptr);

			goto_table_row = pstrcatstr( goto_table_row,
				pstrrender( gen->gotab.col,
					GEN_WILD_PREFIX "symbol",
						int_to_str( col->symbol->id ), TRUE,
					GEN_WILD_PREFIX "action",
						int_to_str( col->action ), TRUE,
					GEN_WILD_PREFIX "index",
						int_to_str( col->index ), TRUE,
					GEN_WILD_PREFIX "column",
						int_to_str( column ), TRUE,
					(char*)NULL ), TRUE );

			if( m->next )
				goto_table_row = pstrcatstr( goto_table_row,
					gen->gotab.col_sep, FALSE );
		}

		goto_table_row = pstrcatstr( goto_table_row,
				pstrrender( gen->gotab.row_end,
					GEN_WILD_PREFIX "number-of-columns",
						int_to_str( list_count( st->actions ) ), TRUE,
					GEN_WILD_PREFIX "state-number",
						int_to_str( st->state_id ), TRUE,
					(char*)NULL ), TRUE );

		if( parray_next( parser->states, st ) )
			goto_table_row = pstrcatstr( goto_table_row,
				gen->gotab.row_sep, FALSE );

		goto_table = pstrcatstr( goto_table, goto_table_row, TRUE );

		/* Only in scannerless mode */
		if( parser->p_mode == MODE_SCANNERLESS )
		{
			/* dfa machine selection */
			dfa_select = pstrcatstr( dfa_select,
				pstrrender( gen->dfa_select.col,
					GEN_WILD_PREFIX "machine",
						int_to_str( list_find( parser->dfas, st->dfa ) ), TRUE,
							(char*)NULL ), TRUE );

			if( parray_next( parser->states, st ) )
				dfa_select = pstrcatstr( dfa_select,
								gen->dfa_select.col_sep, FALSE );
		}

		/* Default production table */
		def_prod = pstrcatstr( def_prod,
				pstrrender( gen->defprod.col,
					GEN_WILD_PREFIX "state-number",
						int_to_str( st->state_id ), TRUE,
					GEN_WILD_PREFIX "production-number",
						int_to_str(
							( ( st->def_prod ) ? st->def_prod->id : -1 ) ),
								TRUE, (char*)NULL ), TRUE );

		if( parray_next( parser->states, st ) )
			def_prod = pstrcatstr( def_prod, gen->defprod.col_sep, FALSE );

		i++;
	}

	/* Lexical recognition machine table composition */
	MSG( "Lexical recognition machine" );
	for( l = parser->dfas, row = 0, column = 0; l; l = list_next( l ), row++ )
	{
		dfa = (pregex_dfa*)list_access( l );

		/* Row start */
		dfa_idx_row = pstrrender( gen->dfa_idx.row_start,
				GEN_WILD_PREFIX "number-of-columns",
					int_to_str( plist_count( dfa->states ) ), TRUE,
				GEN_WILD_PREFIX "row",
					int_to_str( row ), TRUE,
				(char*)NULL );

		dfa_accept_row = pstrrender( gen->dfa_accept.row_start,
				GEN_WILD_PREFIX "number-of-columns",
					int_to_str( plist_count( dfa->states ) ), TRUE,
				GEN_WILD_PREFIX "row",
					int_to_str( row ), TRUE,
				(char*)NULL );

		if( max_dfa_idx < plist_count( dfa->states ) )
			max_dfa_accept = max_dfa_idx = plist_count( dfa->states );

		/* Building row entries */
		plist_for( dfa->states, e )
		{
			dfa_st = (pregex_dfa_st*)plist_access( e );
			VARS( "dfa_st", "%p", dfa_st );

			if( dfa_char && dfa_trans )
			{
				dfa_char = pstrcatstr( dfa_char,
						gen->dfa_char.col_sep, FALSE );
				dfa_trans = pstrcatstr( dfa_trans,
						gen->dfa_trans.col_sep, FALSE );
			}

			dfa_idx_row = pstrcatstr( dfa_idx_row,
				pstrrender( gen->dfa_idx.col,
					GEN_WILD_PREFIX "index",
						int_to_str( column ), TRUE,
					(char*)NULL ), TRUE );

			dfa_accept_row = pstrcatstr( dfa_accept_row,
				pstrrender( gen->dfa_accept.col,
					GEN_WILD_PREFIX "accept",
						int_to_str( dfa_st->accept ), TRUE,
					(char*)NULL ), TRUE );

			/* Iterate trough all transitions */
			MSG( "Iterating to transitions of DFA" );
			plist_for( dfa_st->trans, f )
			{
				dfa_ent = (pregex_dfa_tr*)plist_access( f );

				for( i = 0; pccl_get( &beg, &end, dfa_ent->ccl, i ); i++ )
				{
					dfa_char = pstrcatstr( dfa_char,
								pstrrender( gen->dfa_char.col,
								GEN_WILD_PREFIX "from",
									int_to_str( beg ), TRUE,
								GEN_WILD_PREFIX "to",
									int_to_str( end ), TRUE,
								GEN_WILD_PREFIX "goto",
									int_to_str( dfa_st->accept ), TRUE,
								(char*)NULL ), TRUE );

					dfa_trans = pstrcatstr( dfa_trans,
								pstrrender( gen->dfa_trans.col,
									GEN_WILD_PREFIX "goto",
									int_to_str( dfa_ent->go_to ), TRUE,
								(char*)NULL ), TRUE );


					dfa_char = pstrcatstr( dfa_char,
									gen->dfa_char.col_sep, FALSE );
					dfa_trans = pstrcatstr( dfa_trans,
									gen->dfa_trans.col_sep, FALSE );

					column++;
				}
			}

			/* DFA transition end marker */
			dfa_char = pstrcatstr( dfa_char,
					pstrrender( gen->dfa_char.col,
						GEN_WILD_PREFIX "from",
							int_to_str( -1 ), TRUE,
						GEN_WILD_PREFIX "to",
							int_to_str( -1 ), TRUE,
						(char*)NULL ), TRUE );

			/* DFA transition */
			dfa_trans = pstrcatstr( dfa_trans,
					pstrrender( gen->dfa_trans.col,
						GEN_WILD_PREFIX "goto",
							int_to_str( -1 ),
					TRUE, (char*)NULL ), TRUE );

			column++;

			if( plist_next( e ) )
			{
				dfa_idx_row = pstrcatstr( dfa_idx_row,
						gen->dfa_idx.col_sep, FALSE );
				dfa_accept_row = pstrcatstr( dfa_accept_row,
						gen->dfa_accept.col_sep, FALSE );
			}
		}

		/* Row end */
		dfa_idx_row = pstrcatstr( dfa_idx_row,
				pstrrender( gen->dfa_idx.row_end,
					GEN_WILD_PREFIX "number-of-columns",
						int_to_str( plist_count( dfa->states ) ), TRUE,
					GEN_WILD_PREFIX "row",
						int_to_str( row ), TRUE,
					(char*)NULL ), TRUE );

		dfa_accept_row = pstrcatstr( dfa_accept_row,
				pstrrender( gen->dfa_accept.row_end,
					GEN_WILD_PREFIX "number-of-columns",
						int_to_str( plist_count( dfa->states ) ), TRUE,
					GEN_WILD_PREFIX "row", int_to_str( row ), TRUE,
					(char*)NULL ), TRUE );

		if( list_next( l ) )
		{
			dfa_idx_row = pstrcatstr( dfa_idx_row,
				gen->dfa_idx.row_sep, FALSE );
			dfa_accept_row = pstrcatstr( dfa_accept_row,
				gen->dfa_accept.row_sep, FALSE );
		}

		dfa_idx = pstrcatstr( dfa_idx, dfa_idx_row, TRUE );
		dfa_accept = pstrcatstr( dfa_accept, dfa_accept_row, TRUE );
	}

	MSG( "Construct symbol information table" );

	/* Whitespace identification table and symbol-information-table */
	plist_for( parser->symbols, e ) /* Okidoki, now do the generation */
	{
		sym = (SYMBOL*)plist_access( e );

		symbols = pstrcatstr( symbols, pstrrender( gen->symbols.col,
				GEN_WILD_PREFIX "symbol-name",
					escape_for_target( gen, sym->name, FALSE ), TRUE,
				GEN_WILD_PREFIX "emit",
					escape_for_target( gen, sym->emit, FALSE ), TRUE,
				GEN_WILD_PREFIX "symbol",
					int_to_str( sym->id ), TRUE,
				GEN_WILD_PREFIX "type",
					int_to_str( sym->type ), TRUE,
				GEN_WILD_PREFIX "datatype",
					int_to_str( sym->vtype ? sym->vtype->id : 0 ), TRUE,
				GEN_WILD_PREFIX "is-terminal",
					sym->type > 0 ? gen->truedef : gen->falsedef, TRUE,
				GEN_WILD_PREFIX "is-lexem",
					sym->lexem ? gen->truedef : gen->falsedef, FALSE,
				GEN_WILD_PREFIX "is-whitespace",
					sym->whitespace ? gen->truedef : gen->falsedef, FALSE,
				GEN_WILD_PREFIX "is-greedy",
					sym->greedy ? gen->truedef : gen->falsedef, FALSE,

				(char*)NULL ), TRUE );

		if( max_symbol_name < (int)strlen( sym->name ) )
			max_symbol_name = (int)strlen( sym->name );

		if( plist_next( e ) )
		{
			symbols = pstrcatstr( symbols,
				gen->symbols.col_sep, FALSE );
		}
	}

	/* Type definition union */
	if( list_count( parser->vtypes ) == 1 )
	{
		vt = (VTYPE*)( parser->vtypes->pptr );
		type_def = pstrrender( gen->vstack_single,
				GEN_WILD_PREFIX "value-type", vt->real_def, FALSE,
					(char*)NULL );
	}
	else
	{
		type_def = pstrrender( gen->vstack_union_start,
				GEN_WILD_PREFIX "number-of-value-types",
					int_to_str( list_count( parser->vtypes ) ),
						TRUE, (char*)NULL );

		for( l = parser->vtypes; l; l = l->next )
		{
			vt = (VTYPE*)(l->pptr);

			type_def = pstrcatstr( type_def,
				pstrrender( gen->vstack_union_def,
					GEN_WILD_PREFIX "value-type", vt->real_def, FALSE,
						GEN_WILD_PREFIX "attribute",
								pstrrender( gen->vstack_union_att,
									GEN_WILD_PREFIX "value-type-id",
											int_to_str( vt->id ), TRUE,
												(char*)NULL ), TRUE,
						GEN_WILD_PREFIX "value-type-id",
							int_to_str( vt->id ), TRUE,
						(char*)NULL ), TRUE );
		}

		type_def = pstrcatstr( type_def,
					pstrrender( gen->vstack_union_end,
						GEN_WILD_PREFIX "number-of-value-types",
							int_to_str( list_count( parser->vtypes ) ),
								TRUE, (char*)NULL ), TRUE );
	}

	/* Reduction action code and production definition table */
	row = 0;

	plist_for( parser->productions, e )
	{
		p = (PROD*)plist_access( e );

		/* Select the semantic code to be processed! */
		act = (char*)NULL;

		is_default_code = FALSE;

		if( p->code )
			act = p->code;
		else if( plist_count( p->rhs ) == 0 )
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
				( parser->error && plist_get_by_ptr( p->rhs,
										parser->error ) ) ) )
		{
			act = (char*)NULL;
		}

		if( act && *act )
		{
			/* Generate action start */
			actions = pstrcatstr( actions, pstrrender( gen->action_start,
				GEN_WILD_PREFIX "production-number", int_to_str( p->id ), TRUE,
					(char*)NULL ), TRUE );

			/* Generate code localization */
			if( gen->code_localization && p->code_at > 0 )
			{
				actions = pstrcatstr( actions,
					pstrrender( gen->code_localization,
						GEN_WILD_PREFIX "line",
							int_to_str( p->code_at ), TRUE,
						(char*)NULL ),
					TRUE );
			}

			/* Generate the action code */
			act = build_action( parser, gen, p, act, is_default_code );
			actions = pstrcatstr( actions, act, TRUE );

			/* Generate the action end */
			actions = pstrcatstr( actions, pstrrender( gen->action_end,
				GEN_WILD_PREFIX "production-number", int_to_str( p->id ), TRUE,
					(char*)NULL ), TRUE );
		}

		/* Generate production information table */
		productions = pstrcatstr( productions, pstrrender(
			gen->productions.col,

				GEN_WILD_PREFIX "production-number",
					int_to_str( p->id ), TRUE,
				GEN_WILD_PREFIX "production",
					escape_for_target( gen, mkproduction_str( p ), TRUE ),
						TRUE,
				GEN_WILD_PREFIX "emit",
					escape_for_target( gen, p->emit, TRUE ), TRUE,
				GEN_WILD_PREFIX "length",
					int_to_str( plist_count( p->rhs ) ), TRUE,
				GEN_WILD_PREFIX "lhs",
					int_to_str( p->lhs->id ), TRUE,

			(char*)NULL ), TRUE );

		if( plist_next( e ) )
			productions = pstrcatstr( productions,
				gen->productions.col_sep, FALSE );

		row++;
	}

	/* Scanner action code */
	plist_for( parser->symbols, e )
	{
		sym = (SYMBOL*)plist_access( e );
		if( sym->keyword )
			continue;

		/* Select the semantic code to be processed! */
		if( ( act = sym->code ) )
		{
			/* Code localization features */
			if( gen->code_localization && sym->code_at > 0 )
			{
				scan_actions = pstrcatstr( scan_actions,
					pstrrender( gen->code_localization,
						GEN_WILD_PREFIX "line",
							int_to_str( sym->code_at ), TRUE,
								(char*)NULL ), TRUE );
			}

			scan_actions = pstrcatstr( scan_actions,
				pstrrender( gen->scan_action_start,
					GEN_WILD_PREFIX "symbol-number",
							int_to_str( sym->id ), TRUE,
								(char*)NULL ), TRUE );

			act = build_scan_action( parser, gen, sym, act );
			scan_actions = pstrcatstr( scan_actions, act, TRUE );

			scan_actions = pstrcatstr( scan_actions,
				pstrrender( gen->scan_action_end,
					GEN_WILD_PREFIX "symbol-number",
						int_to_str( sym->id ), TRUE,
							(char*)NULL ), TRUE );
		}
	}

	/* Get the goal production */
	goalprod = (PROD*)plist_access( plist_first( parser->goal->productions ) );

	/* Generate basename - parser->p_basename may contain directory path */
	basename = pstrdup( pbasename( parser->p_basename ) );

	/* Construct the output files */
	for( file = xml_child( gen->xml, "file" );
			file; file = xml_next( file ) )
	{
		/* Make filename */
		if( !parser->to_stdout )
		{
			if( ( filename = (char*)xml_attr( file, "filename" ) ) )
				filename = pstrrender( filename,
					/*
						Here we have to submit the original basename
						to construct the final output filename using the
						full output path.
					*/
					GEN_WILD_PREFIX "basename", parser->p_basename, FALSE,
					GEN_WILD_PREFIX "Cbasename", c_identifier(
						parser->p_basename, FALSE ), TRUE,
					GEN_WILD_PREFIX "CBASENAME", c_identifier(
						parser->p_basename, TRUE ), TRUE,
					GEN_WILD_PREFIX "prefix", parser->p_prefix, FALSE,
						(char*)NULL );
		}

		/* Assembling all together - Warning, this is
			ONE single function call! */

		all = pstrrender( xml_txt( file ),

			/* Lengths of names and Prologue/Epilogue codes */
			GEN_WILD_PREFIX "prologue" LEN_EXT,
				long_to_str( (long)pstrlen( parser->p_header ) ), TRUE,
			GEN_WILD_PREFIX "epilogue" LEN_EXT,
				long_to_str( (long)pstrlen( parser->p_footer ) ), TRUE,
			GEN_WILD_PREFIX "pcb" LEN_EXT,
				long_to_str( (long)pstrlen( parser->p_pcb ) ), TRUE,

			/* Names and Prologue/Epilogue codes */
			GEN_WILD_PREFIX "prologue", parser->p_header, FALSE,
			GEN_WILD_PREFIX "epilogue", parser->p_footer, FALSE,
			GEN_WILD_PREFIX "pcb", parser->p_pcb, FALSE,

			/* Limits and sizes, parse tables */
			GEN_WILD_PREFIX "number-of-symbols",
				int_to_str( plist_count( parser->symbols ) ), TRUE,
			GEN_WILD_PREFIX "number-of-states",
				int_to_str( parray_count( parser->states ) ), TRUE,
			GEN_WILD_PREFIX "number-of-productions",
				int_to_str( plist_count( parser->productions ) ), TRUE,
			GEN_WILD_PREFIX "number-of-dfa-machines",
				int_to_str( list_count( parser->dfas ) ), TRUE,
			GEN_WILD_PREFIX "deepest-action-row",
				int_to_str( max_action ), TRUE,
			GEN_WILD_PREFIX "deepest-goto-row",
				int_to_str( max_goto ), TRUE,
			GEN_WILD_PREFIX "deepest-dfa-index-row",
				int_to_str( max_dfa_idx ), TRUE,
			GEN_WILD_PREFIX "deepest-dfa-accept-row",
				int_to_str( max_dfa_accept ), TRUE,
			GEN_WILD_PREFIX "size-of-dfa-characters",
				int_to_str( column ), TRUE,
			GEN_WILD_PREFIX "number-of-character-map",
				int_to_str( charmap_count ), TRUE,
			GEN_WILD_PREFIX "action-table", action_table, FALSE,
			GEN_WILD_PREFIX "goto-table", goto_table, FALSE,
			GEN_WILD_PREFIX "default-productions", def_prod, FALSE,
			GEN_WILD_PREFIX "character-map-symbols", char_map_sym, FALSE,
			GEN_WILD_PREFIX "character-map", char_map, FALSE,
			GEN_WILD_PREFIX "character-universe",
				int_to_str( parser->p_universe ), TRUE,
			GEN_WILD_PREFIX "symbols", symbols, FALSE,
			GEN_WILD_PREFIX "productions", productions, FALSE,
			GEN_WILD_PREFIX "max-symbol-name-length",
				int_to_str( max_symbol_name ), TRUE,
			GEN_WILD_PREFIX "dfa-select", dfa_select, FALSE,
			GEN_WILD_PREFIX "dfa-index", dfa_idx, FALSE,
			GEN_WILD_PREFIX "dfa-char", dfa_char, FALSE,
			GEN_WILD_PREFIX "dfa-trans", dfa_trans, FALSE,
			GEN_WILD_PREFIX "dfa-accept", dfa_accept, FALSE,
			GEN_WILD_PREFIX "value-type-definition", type_def, FALSE,
			GEN_WILD_PREFIX "actions", actions, FALSE,
			GEN_WILD_PREFIX "scan_actions", scan_actions, FALSE,
			GEN_WILD_PREFIX "top-value", top_value, FALSE,
			GEN_WILD_PREFIX "goal-value", goal_value, FALSE,
			GEN_WILD_PREFIX "goal-type", parser->goal->vtype ?
											parser->goal->vtype->real_def : "",
												FALSE,
			GEN_WILD_PREFIX "mode", int_to_str( parser->p_mode ), TRUE,
			GEN_WILD_PREFIX "error",
				( parser->error ? int_to_str( parser->error->id ) :
							int_to_str( -1 ) ), TRUE,
			GEN_WILD_PREFIX "eof",
				( parser->end_of_input ?
					int_to_str( parser->end_of_input->id ) :
						int_to_str( -1 ) ), TRUE,
			GEN_WILD_PREFIX "goal-production",
				int_to_str( goalprod->id ), TRUE,
			GEN_WILD_PREFIX "goal",
				int_to_str( parser->goal->id ), TRUE,

			(char*)NULL
		);

		/* Replace all top-level options */
		plist_for( parser->options, e )
		{
			if( !( option = pasprintf( "%s%s",
					GEN_WILD_PREFIX, plist_key( e ) ) ) )
				OUTOFMEM;

			if( !( complete = pstrrender( all,
								option, (char*)plist_access( e ), FALSE,
								(char*)NULL ) ) )
				OUTOFMEM;

			pfree( all );
			pfree( option );
			all = complete;
		}

		/* Perform line number updating on this file */
		/*
		if( gen->code_localization )
			build_code_localizations( &all, gen );
		*/

		/* Now replace all prefixes */
		complete = pstrrender( all,
					GEN_WILD_PREFIX "prefix",
						parser->p_prefix, FALSE,
					GEN_WILD_PREFIX "basename",
						basename, FALSE,
					GEN_WILD_PREFIX "Cbasename",
						c_identifier( basename, FALSE ), TRUE,
					GEN_WILD_PREFIX "CBASENAME",
						c_identifier( basename, TRUE ), TRUE,
					GEN_WILD_PREFIX "filename" LEN_EXT,
						long_to_str(
							(long)pstrlen( parser->filename ) ), TRUE,
					GEN_WILD_PREFIX "filename", parser->filename, FALSE,

					(char*)NULL );

		pfree( all );

		/* Open output file */
		if( filename )
		{
			if( !( stream = fopen( filename, "wt" ) ) )
			{
				print_error( parser, ERR_OPEN_OUTPUT_FILE,
					ERRSTYLE_FATAL, filename );

				pfree( filename );
				filename = (char*)NULL;
			}
		}

		if( !filename )
		{
			stream = stdout;

			if( parser->files_count > 0 )
				fprintf( stdout, "%c", EOF );
		}

		parser->files_count++;
		fprintf( stream, "%s", complete );
		pfree( complete );

		if( filename )
		{
			fclose( stream );
			pfree( filename );
		}
	}

	MSG( "Freeing used memory" );

	pfree( basename );

	/* Freeing generated content */
	pfree( action_table );
	pfree( goto_table );
	pfree( def_prod );
	pfree( char_map );
	pfree( char_map_sym );
	pfree( symbols );
	pfree( productions );
	pfree( dfa_select );
	pfree( dfa_idx );
	pfree( dfa_char );
	pfree( dfa_trans );
	pfree( dfa_accept );
	pfree( type_def );
	pfree( actions );
	pfree( scan_actions );
	pfree( top_value );
	pfree( goal_value );

	/* Freeing the generator's structure */
	pfree( gen->for_sequences );
	pfree( gen->do_sequences );
	xml_free( gen->xml );

	/* Free local lexers */
	plex_free( action_lex );
	plex_free( scan_lex );

	VOIDRET;
}
