/* -MODULE----------------------------------------------------------------------
UniCC² Parser Generator
Copyright (C) 2006-2020 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	parse.c
Usage:	Parser maintenance object.
----------------------------------------------------------------------------- */

#include "unicc.h"

/* -----------------------------------------------------------------------------
	Abstract Syntax Tree
----------------------------------------------------------------------------- */

/** Creates a new abstract syntax tree node //emit// associated with //sym//. */
AST_node* ast_create( char* emit, Symbol* sym, Production* prod, Token* tok,
						AST_node* child )
{
	AST_node*	node;

	PROC( "ast_create" );
	PARMS( "emit", "%s", emit );
	PARMS( "child", "%p", child );

	if( !( emit ) )
	{
		WRONGPARAM;
		RETURN( (AST_node*)NULL );
	}

	node = (AST_node*)pmalloc( sizeof( AST_node ) );

	node->emit = emit;
	node->child = child;
	node->sym = sym;
	node->prod = prod;

	if( tok )
		node->token = *tok;

	while( child )
	{
		child->parent = node;
		child = child->next;
	}

	RETURN( node );
}

/** Frees entire //ast// structure and subsequent links.

Always returns (AST_node*)NULL. */
AST_node* ast_free( AST_node* node )
{
	AST_node*	child;
	AST_node*	next;

	if( !node )
		return (AST_node*)NULL;

	next = node->child;

	while( next )
	{
		child = next;
		next = child->next;

		ast_free( child );
	}

	return (AST_node*)pfree( node );
}

/** Returns length of //node// chain. */
int ast_len( AST_node* node )
{
	int		step	= 0;

	while( node )
	{
		node = node->next;
		step++;
	}

	return step;
}

/** Returns the //n//th element of //node//. */
AST_node* ast_get( AST_node* node, int n )
{
	if( !( node && n >= 0 ) )
	{
		WRONGPARAM;
		return (AST_node*)NULL;
	}

	while( node && n-- > 0 )
		node = node->next;

	return node;
}

/** Returns the //n//th element matching emit //emit// starting at //node//. */
AST_node* ast_select( AST_node* node, char* emit, int n )
{
	if( !( node && n >= 0 ) )
	{
		WRONGPARAM;
		return (AST_node*)NULL;
	}

	while( node )
	{
		if( ( !*pstrget( emit ) || strcmp( node->emit, emit ) == 0 )
			&& n-- == 0 )
			break;

		node = node->next;
	}

	return node;
}

/** Evaluate //ast// using evaluation function //func//.

The evaluation function has the prototype

``` void (*Ast_evalfn)( Ast_eval type, AST_node* node )

and retrieves a //type// regarding the position where the evaluation currently
is positioned, and the node pointer.
*/
void ast_eval( AST_node* ast, Ast_evalfn func )
{
	AST_node* 	node;

	while( ast )
	{
		func( AST_EVAL_TOPDOWN, ast );

		ast_eval( ast->child, func );

		for( node = ast->child; node; node = node->next )
			func( AST_EVAL_PASSOVER, node );

		func( AST_EVAL_BOTTOMUP, ast );

		ast = ast->next;
	}
}

/** Dump //ast// in a human-readable, full-featured format to //stream//. */
void ast_dump( FILE* stream, AST_node* ast )
{
	static int lev		= 0;
	int	i;

	while( ast )
	{
		for( i = 0; i < lev; i++ )
			fprintf( stream, " " );

		if( ast->child )
			fprintf( stream, "%s {", ast->emit );
		else
		{
			fprintf( stream, "%s ", ast->emit );

			if( ast->val )
				fprintf( stream, " (%p)", ast->val );
			else if( ast->token.start && ast->token.len )
				fprintf( stream, ">%.*s<", (int)ast->token.len,
						ast->token.start );
		}

		fprintf( stream, "\n" );

		if( ast->child )
		{
			lev++;
			ast_dump( stream, ast->child );
			lev--;
		}

		if( ast->child )
		{

			for( i = 0; i < lev; i++ )
				fprintf( stream, " " );

			fprintf( stream, "} %s\n", ast->emit );
		}

		ast = ast->next;
	}
}

/** Dump //ast// in a human-readable, simplified format to //stream//.

Only opening matches are printed. */
void ast_dump_short( FILE* stream, AST_node* ast )
{
	static int lev		= 0;
	int	i;

	while( ast )
	{
		for( i = 0; i < lev; i++ )
			fprintf( stream, " " );

		fprintf( stream, "%s", ast->emit );

		if( ast->token.start && ast->token.len )
		{
			if( !memchr( ast->token.start, '\n', ast->token.len ) )
			{
				fprintf( stream, " >%.*s<",
						(int)ast->token.len, ast->token.start );
			}
		}

		fprintf( stream, "\n" );

		if( ast->child )
		{
			lev++;
			ast_dump_short( stream, ast->child );
			lev--;
		}

		ast = ast->next;
	}
}

/** Dump //ast// in human-readable YAML-format to //stream//. */
void ast_dump_yaml( FILE* stream, AST_node* ast, size_t indent )
{
	size_t	i;
	char*	ptr;

	while( ast )
	{
		/*
		for( i = 0; i < indent; i++ )
			fprintf( stream, "  " );

		fprintf( stream, "node:\n" );
		*/

		for( i = 0; i < indent; i++ )
			fprintf( stream, "  " );

		fprintf( stream, "- node: %s\n", ast->emit );

		if( ( SYM_IS_TERMINAL( ast->sym )
				|| ast->sym->flags.lexem
					|| !ast->child )
			&& ast->token.start
				&& ast->token.len )
		{
			for( i = 0; i < indent; i++ )
				fprintf( stream, "  " );

			if( !memchr( ast->token.start, '\n', ast->token.len ) )
				fprintf( stream, "  match: %.*s\n",
					(int)ast->token.len, ast->token.start );
			else
			{
				fprintf( stream, "  match: |" );
				for( ptr = ast->token.start;
						*ptr && ptr < ast->token.start + ast->token.len;
							ptr++ )
				{
					if( ptr == ast->token.start || *ptr == '\n' )
					{
						fprintf( stream, "\n" );

						for( i = 0; i < indent + 2; i++ )
							fprintf( stream, "  " );
					}

					if( *ptr != '\n' )
						fputc( *ptr, stream );
				}

				fprintf( stream, "\n" );
			}
		}

		if( ast->child )
		{
			for( i = 0; i < indent; i++ )
				fprintf( stream, "  " );

			fprintf( stream, "  child:\n" );
			ast_dump_yaml( stream, ast->child, indent + 2 );
		}

		ast = ast->next;
	}
}

/** Dump AST to trace.

The AST_DUMP-macro is used to dump all relevant contents of a AST_node object
into the program trace, for debugging purposes.

AST_DUMP() can only be used when the function was trace-enabled by PROC()
before.
*/
/*MACRO:AST_DUMP( AST_node* ast )*/
void __dbg_ast_dump( char* file, int line, char* function,
					 char* name, AST_node* ast )
{
	if( !_dbg_trace_enabled( file, function, "AST" ) )
		return;

	_dbg_trace( file, line, "AST", function, "%s = {", name );
	ast_dump_short( stdout, ast );
	_dbg_trace( file, line, "AST", function, "}" );
}

/** Dump //ast// to //stream// as JSON-formatted string.

Only opening matches are printed. */
void ast_dump_json( FILE* stream, AST_node* ast )
{
	char*		ptr;
	char*		eptr;
	AST_node*	node	= ast;

	if( ast && ast->next )
		fprintf( stream, "[" );

	while( node )
	{
		fputc( '{', stream );
		fprintf( stream, "\"emit\":" );

		/* Emit */
		fputc( '"', stream );

		for( ptr = node->emit; *ptr; ptr++ )
		{
			if( *ptr == '\"' )
				fputc( '\\', stream );

			fputc( *ptr, stream );
		}

		fputc( '"', stream );

		/* Match */
		if( SYM_IS_TERMINAL( node->sym )
			|| node->sym->flags.lexem
			|| !node->child )
		{
			fprintf( stream, ",\"match\":" );

			/* Matched string */
			fputc( '"', stream );

			if( node->val )
			{
				/* fixme
				ptr = pany_get_str( node->val );
				eptr = ptr + strlen( ptr );
				*/
			}
			else if( node->token.start && node->token.end )
			{
				ptr = node->token.start;
				eptr = node->token.end;
			}
			else
				ptr = eptr = (char*)NULL;

			if( ptr && eptr && ptr < eptr )
			{
				for( ; *ptr && ptr < eptr; ptr++ )
					switch( *ptr )
					{
						case '\"':
							fprintf( stream, "\\\"" );
							break;
						case '\n':
							fprintf( stream, "\\n" );
							break;
						case '\r':
							fprintf( stream, "\\r" );
							break;
						case '\t':
							fprintf( stream, "\\t" );
							break;
						case '\v':
							fprintf( stream, "\\v" );
							break;

						default:
							if( *ptr >= 0 && *ptr <= 31 )
								fprintf( stream, "\\u%04x", *ptr );
							else
								fputc( *ptr, stream );
					}
			}

			fputc( '"', stream );
		}

		/* Position */
		/*
		fprintf( stream, ",\"row\":%ld,\"column\":%ld",
				node->token.row, node->token.col );
		*/

		/* Children */
		if( node->child )
		{
			fprintf( stream, ",\"child\":" );
			ast_dump_json( stream, node->child );
		}

		fputc( '}', stream );

		if( ( node = node->next ) )
			fputc( ',', stream );
	}

	if( ast && ast->next )
		fprintf( stream, "]" );
}

/** Dump //ast// in notation for the tree2svg tool that generates a
graphical view of the parse tree. */
void ast_dump_tree2svg( FILE* stream, AST_node* ast )
{
	while( ast )
	{
		if( !SYM_IS_TERMINAL( ast->sym ) )
		{
			fprintf( stream, "'%s' [ ", ast->emit );
			ast_dump_tree2svg( stream, ast->child );
			fprintf( stream, "] " );
		}
		else if( ast->token.start )
			fprintf( stream, "'%.*s' ", (int)ast->token.len, ast->token.start );
		/* else if( ast->val ) fixme
			fprintf( stream, "'%s' ", pany_get_str( ast->val ) ); */
		else
			fprintf( stream, "'%s' ", ast->emit );

		ast = ast->next;
	}
}

#if 0
static void eval_emit( pvmprog* prog, char* emit, AST_node* ast )
{
	static char				buf		[ BUFSIZ + 1 ];
	char*					ptr;
	char*					code;

	PROC( "eval_emit" );
	PARMS( "prog", "%p", prog );
	PARMS( "emit", "%s", emit );

	ptr = buf;
	if( ast->token.len < BUFSIZ )
		sprintf( buf, "%.*s", (int)ast->token.len, ast->token.start );
	else
		ptr = pstrndup( ast->token.start, ast->token.len );

	if( strstr( emit, "$0" ) )
		code = pstrreplace( emit, "$0", ptr );
	else
		code = emit;

	VARS( "code", "%s", code );

	pvm_prog_compile( prog, code );

	if( code != ast->emit )
		pfree( code );

	if( ptr != buf )
		pfree( ptr );

	VOIDRET;
}

/** Dump //ast// into pvm program */
void ast_dump_pvm( pvmprog* prog, AST_node* ast )
{
	PROC( "ast_dump_pvm" );
	PARMS( "prog", "%p", prog );
	PARMS( "ast", "%p", ast );

	if( !( prog && ast ) )
	{
		WRONGPARAM;
		VOIDRET;
	}

	while( ast )
	{
		/*
		if( ast->emit && pstrncasecmp( ast->emit, "<<" ) )
			eval_emit( prog, ast->emit + 2 );
		*/

		if( ast->child )
		{
			ast_dump_pvm( prog, ast->child );

			/*
			if( ast->emit && pstrncasecmp( ast->emit, "!!" ) )
				eval_emit( prog, ast->emit + 2 );
			*/
		}

		if( ast->emit /* && (pstrncasecmp( ast->emit, ">>" ) */ )
		{
			eval_emit( prog, ast->emit, ast );
		}

		ast = ast->next;
	}

	VOIDRET;
}
#endif

/* -----------------------------------------------------------------------------
	Parser
----------------------------------------------------------------------------- */

/* LR-Stackitem */
typedef struct
{
	int				state;			/* State */
	int				dstate;			/* Diagnose state */
	Symbol*			sym;			/* Symbol */

	char*			start;			/* Start */

	AST_node*		node;			/* AST construction */

	Token			token;			/* Token */
} LRstackitem;


/** Creates a new parser object using the underlying grammar //g//.

The grammar must either be parsed first via one of the BNF parsers
(gram_from_pbnf(), gram_from_ebnf(), gram_from_bnf()) or it
must be hand-crafted.

The provided grammar gets "frozen" when a parser is created from it.
Modification of the grammar with a parser based on an older grammar state may
cause memory corruption and crashes.

The function returns a valid parser object on success, or (Parser*)NULL in case
the grammar is invalid. */
Parser* par_create( Grammar* g )
{
	Parser*	p;

	PROC( "par_create" );
	PARMS( "g", "%p", g );

	if( !g )
	{
		WRONGPARAM;
		RETURN( (Parser*)NULL );
	}

	if( !g->flags.finalized && !gram_prepare( g ) )
	{
		MSG( "The provided grammar object is invalid" );
		RETURN( (Parser*)NULL );
	}

	MSG( "Grammar prepared" );

	p = (Parser*)pmalloc( sizeof( Parser ) );

	/* Grammar */
	p->gram = g;
	g->flags.frozen = TRUE;

	if( !lr_build( &p->states, &p->dfa, p->gram ) )
	{
		MSG( "Bulding parse tables failed" );

		par_free( p );
		RETURN( (Parser*)NULL );
	}

	MSG( "LR constructed" );

	/* GRAMMAR_DUMP( g ); */
	RETURN( p );
}

/** Frees the parser object //par//. */
Parser* par_free( Parser* par )
{
	if( !par )
		return (Parser*)NULL;

	pfree( par );

	return (Parser*)NULL;
}

/** Automatically generate lexical analyzer from terminal symbols
defined in the grammar.

The lexical analyzer is constructed as a plex scanner and must be freed
after usage with plex_free(). */
plex* par_autolex( Parser* p )
{
	Symbol*		sym;
	plistel*	e;
	plex*		lex				= (plex*)NULL;

	PROC( "par_autolex" );
	PARMS( "p", "%p", p );

	if( !( p ) )
	{
		WRONGPARAM;
		RETURN( 0 );
	}

	plist_for( p->gram->symbols, e )
	{
		sym = (Symbol*)plist_access( e );

		if( !SYM_IS_TERMINAL( sym ) || sym->flags.special )
			continue;

		VARS( "sym->name", "%s", sym->name ? sym->name : "(null)" );
		VARS( "sym->idx", "%d", sym->idx );

		if( !lex )
		{
			MSG( "OK, its time to create a lexer" );
			lex = plex_create( PREGEX_FLAG_NONE );
		}

		if( sym->ptn )
		{
			VARS( "sym->ptn", "%p", sym->ptn );
			plex_define( lex, (char*)sym->ptn, (int)sym->idx, PREGEX_COMP_PTN );
		}
		else
		{
			VARS( "sym->str", "%s", sym->str ? sym->str : "(null)" );
			VARS( "sym->name", "%s", sym->name ? sym->name : "(null)" );

			plex_define( lex, sym->str ? sym->str : sym->name,
				(int)sym->idx, sym->str ? PREGEX_COMP_STATIC : 0 );
		}
	}

	VARS( "lex", "%p", lex );
	RETURN( lex );
}

/** Dumps parser to JSON */
pboolean par_dump_json( FILE* stream, Parser* par )
{
	size_t	i;
	size_t	j;
	size_t	k;
	plex*	lex;

	PROC( "par_dump_json" );
	PARMS( "stream", "%p", stream );
	PARMS( "par", "%p", par );

	if( !par )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	if( !stream )
		stream = stdout;

	/* Grammar */
	fprintf( stream, "{\n\t\"grammar\":\n" );
	gram_dump_json( stream, par->gram, "\t" );

	/* LR parser */
	fprintf( stream, ",\n\t\"states\": [" );

	for( i = 0; i < par->states; i++ )
	{
		fprintf( stream, "\n\t\t{\n" );
		fprintf( stream, "\t\t\t\"state\": %ld,\n", i );

		if( par->dfa[ 1 ] )
			fprintf( stream, "\t\t\t\"reduce-default\": %ld,\n",
				( par->dfa[ i ][ 1 ] - 1 ) );

		else
			fprintf( stream, "\t\t\t\"reduce-default\": null,\n" );

		fprintf( stream, "\t\t\t\"transitions\": [" );

		/* Actions */
		for( j = 2; j < par->dfa[ i ][ 0 ]; j += 3 )
		{
			fprintf( stream, "\n\t\t\t\t{\"symbol\": %ld, ", par->dfa[ i ][ j ] - 1 );

			if( par->dfa[ i ][ j + 1 ] & LR_REDUCE )
				fprintf( stream, "\"action\": \"%s\", \"production\": %d",
					par->dfa[ i ][ j + 1 ] == ( LR_SHIFT | LR_REDUCE ) ?
						"shift&reduce" : "reduce",
					(int)( par->dfa[ i ][ j + 2 ] - 1 ) );
			else if( par->dfa[ i ][ j + 1 ] & LR_SHIFT )
				fprintf( stream, "\"action\": \"shift\", \"state\": %d",
					(int)( par->dfa[ i ][ j + 2 ] - 1 ) );

			fprintf( stream, "}%s", j + 3 < par->dfa[ i ][ 0 ] ? "," : "" );
		}

		fprintf( stream, "\n\t\t\t]\n\t\t}%s", i + 1 < par->states ? "," : "" );
	}

	fprintf( stream, "\n\t]" );

	/* Lexer */
	fprintf( stream, ",\n\t\"lexers\": [" );
	
	/* For now there exists only one lexer. */
	lex = par_autolex( par );
	plex_prepare( lex );

	fprintf( stream, "\n\t\t[\n" );

	for( j = 0; j < lex->trans_cnt; j++ )
	{
		fprintf( stream, "\n\t\t\t{" );
		fprintf( stream, "\n\t\t\t\t\"state\": %ld,", j );
		fprintf( stream, "\n\t\t\t\t\"accept\": %d", lex->trans[j][1] );
		fprintf( stream, ",\n\t\t\t\t\"ref\": %d", lex->trans[j][3] );

		/* Default goto */
		if( par->dfa[ 1 ] )
			fprintf( stream, ",\n\t\t\t\t\"goto-default\": %d", lex->trans[j][4] );
		else
			fprintf( stream, ",\n\t\t\t\t\"goto-default\": null" );

		/* Flags */
		fprintf( stream, ",\n\t\t\t\t\"flags\": [" );

		if( lex->trans[j][2] )
			fprintf( stream, "\n\t\t\t\t\t\"greedy\"" );

		fprintf( stream, "\n\t\t\t\t]" );

		/* Transitions */
		fprintf( stream, ",\n\t\t\t\t\"transitions\": [" );

		for( k = 5; k < lex->trans[j][0]; k += 3 )
		{
			fprintf( stream, "\n\t\t\t\t\t\t{\"character-from\": %d", lex->trans[j][k] );
			fprintf( stream, ", \"character-until\": %d", lex->trans[j][k + 1] );
			fprintf( stream, ", \"goto-state\": %d", lex->trans[j][k + 2] );
			fprintf( stream, "}%s", k + 3 < lex->trans[j][0] ? ",": "" );
		}

		fprintf( stream, "\n\t\t\t\t]\n\t\t\t}%s", j + 1 < lex->trans_cnt ? ",": "" );
	}
	fprintf( stream, "\n\t\t]\n" );

	plex_free( lex );

	fprintf( stream, "\t]\n}\n" );

	RETURN( TRUE );
}


/** Initializes a parser context //ctx// for parser //par//.

Parser contexts are objects holding state and semantics information on a
current parsing process. */
Parser_ctx* parctx_init( Parser_ctx* ctx, Parser* par )
{
	PROC( "parctx_init" );
	PARMS( "ctx", "%p", ctx );
	PARMS( "par", "%p", par );

	if( !( ctx && par ) )
	{
		WRONGPARAM;
		RETURN( (Parser_ctx*)NULL );
	}

	memset( ctx, 0, sizeof( Parser_ctx ) );

	ctx->par = par;
	ctx->state = STAT_INITIAL;

	parray_init( &ctx->stack, sizeof( LRstackitem ), 0 );

	RETURN( ctx );
}

/** Creates a new parser context for parser //par//.

Parser contexts are objects holding state and semantics information on a
current parsing process. */
Parser_ctx* parctx_create( Parser* par )
{
	Parser_ctx*	ctx;

	PROC( "parctx_create" );
	PARMS( "par", "%p", par );

	if( !par )
	{
		WRONGPARAM;
		RETURN( (Parser_ctx*)NULL );
	}

	ctx = (Parser_ctx*)pmalloc( sizeof( Parser_ctx ) );
	parctx_init( ctx, par );

	RETURN( ctx );
}

/** Resets the parser context object //ctx//. */
Parser_ctx* parctx_reset( Parser_ctx* ctx )
{
	if( !ctx )
		return (Parser_ctx*)NULL;

	parray_erase( &ctx->stack );
	ctx->ast = ast_free( ctx->ast );

	ctx->state = STAT_INITIAL;

	return ctx;
}

/** Frees the parser context object //ctx//. */
Parser_ctx* parctx_free( Parser_ctx* ctx )
{
	if( !ctx )
		return (Parser_ctx*)NULL;

	parctx_reset( ctx );
	pfree( ctx );

	return (Parser_ctx*)NULL;
}

#define PRINT_TOKEN( title, token ) \
	fprintf( stderr, "%s >%.*s<%s", title ? title : "", \
		(token)->len ? (int)(token)->len : 6, \
			(token)->start ? (token)->start : "(null)", \
				title && *title ? "\n" : "" )

#if 0
/* Function to dump the parse stack content */
static void print_stack( char* title, parray* stack )
{
	LRstackitem*	e;
	int				i;

	fprintf( stderr, "STACK DUMP %s\n", title );

	for( i = 0; i < parray_count( stack ); i++ )
	{
		e = (LRstackitem*)parray_get( stack, i );
		fprintf( stderr, "%02d: %s %d",
			i, e->sym->name, e->state );

		PRINT_TOKEN( "", &e->token );

		if( e->node )
		{
			fprintf( stderr, " [%s]", e->node->emit );
			PRINT_TOKEN( "", &e->node->token );
		}

		fprintf( stderr, "\n" );
	}
}
#endif


/** Let parser and context //ctx// run on next token //tok//.

//val// is an optional parameter that holds a semantic value. It will be
assigned to an AST tree node when provided. Set it NULL if the value is not
used or required.

This method is called a push-parsing algorithm, where the scanner calls the
parser to perform the next parsing steps. The used parsing method is LALR(1).

The function returns one of the following values:

- STAT_DONE when the parse succeeded,
- STAT_NEXT when the partial parse succeeded, and next token is wanted,
- STAT_ERROR when an error was encountered.
-
*/
Parser_stat parctx_next( Parser_ctx* ctx, Token* tok )
{
	int				i;
	LRstackitem*	tos;
	int				shift;
	int				reduce;
	Parser*			par;
	Token			range;

	PROC( "parctx_next" );

	if( !( ctx && tok ) )
	{
		WRONGPARAM;
		RETURN( STAT_ERROR );
	}

	PARMS( "ctx", "%p", ctx );
	PARMS( "sym", "%p", tok );

	par = ctx->par;

	/* Initialize parser on first call */
	if( ctx->state == STAT_INITIAL )
	{
		MSG( "Initial call" );
		parctx_reset( ctx );

		tos = (LRstackitem*)parray_malloc( &ctx->stack );
		tos->sym = par->gram->goal;
	}
	else
		tos = (LRstackitem*)parray_last( &ctx->stack );

	/* Until all reductions are performed... */
	do
	{
		/* Reduce */
		while( ctx->reduce )
		{
			LOG( "reduce by production '%s'", prod_to_str( ctx->reduce ) );

			if( ctx->reducefn )
			{
				LOG( "calling reduce function %p", ctx->reducefn );
				(*ctx->reducefn)( ctx );
			}

			LOG( "popping %d items off the stack, replacing by %s\n",
						plist_count( ctx->reduce->rhs ),
							ctx->reduce->lhs->name );

			ctx->last = (AST_node*)NULL;
			memset( &range, 0, sizeof( Token ) );

			for( i = 0; i < plist_count( ctx->reduce->rhs ); i++ )
			{
				tos = (LRstackitem*)parray_pop( &ctx->stack );

				/* Connecting nodes, remember last node. */
				if( tos->node )
				{
					if( ctx->last )
					{
						while( tos->node->next )
							tos->node = tos->node->next;

						tos->node->next = ctx->last;
						ctx->last->prev = tos->node;
					}

					ctx->last = tos->node;

					while( ctx->last->prev )
						ctx->last = ctx->last->prev;
				}

				if( tos->token.start )
				{
					if( !range.start )
						range = tos->token;
					else
					{
						range.start = tos->token.start;
						range.len = range.end - range.start;
					}
				}
			}

			tos = (LRstackitem*)parray_last( &ctx->stack );

			/* Construction of AST node */
			if( ctx->reduce->emit || ctx->reduce->lhs->emit )
			{
				ctx->last = ast_create(
						ctx->reduce->emit ? ctx->reduce->emit
							: ctx->reduce->lhs->emit, ctx->reduce->lhs,
								ctx->reduce, &range,
									ctx->last );
			}

			/* Goal symbol reduced? */
			if( ctx->reduce->lhs == par->gram->goal
					&& parray_count( &ctx->stack ) == 1 )
			{
				MSG( "Parsing succeeded!" );

				ctx->ast = ctx->last;

				RETURN( ( ctx->state = STAT_DONE ) );
			}

			/* Check for entries in the parse table */
			for( i = par->dfa[tos->state][0] - 3, shift = 0, reduce = 0;
					i >= 2; i -= 3 )
			{
				if( par->dfa[tos->state][i] == ctx->reduce->lhs->idx + 1 )
				{
					if( par->dfa[ tos->state ][ i + 1 ] & LR_SHIFT )
						shift = par->dfa[tos->state][ i + 2 ];

					if( par->dfa[ tos->state ][ i + 1 ] & LR_REDUCE )
						reduce = par->dfa[tos->state][ i + 2 ];

					break;
				}
			}

			tos = (LRstackitem*)parray_malloc( &ctx->stack );

			tos->sym = ctx->reduce->lhs;
			tos->state = shift - 1;

			if( !tos->sym->flags.whitespace )
				tos->dstate = tos->state;

			tos->node = ctx->last;
			tos->token = range;

			ctx->reduce = reduce ? prod_get( par->gram, reduce - 1 )
									: (Production*)NULL;

			LOG( "New top state is %d", tos->state );
		}

		VARS( "State", "%d", tos->state );

		/* Check for entries in the parse table */
		if( tos->state > -1 )
		{
			for( i = 2, shift = 0, reduce = 0;
					i < par->dfa[tos->state][0]; i += 3 )
			{
				if( par->dfa[tos->state][i] == tok->symbol->idx + 1 )
				{
					if( par->dfa[ tos->state ][ i + 1 ] & LR_SHIFT )
						shift = par->dfa[tos->state][ i + 2 ];

					if( par->dfa[ tos->state ][ i + 1 ] & LR_REDUCE )
						reduce = par->dfa[tos->state][ i + 2 ];

					break;
				}
			}

			if( !shift && !reduce )
				reduce = par->dfa[ tos->state ][ 1 ];

			ctx->reduce = reduce ? prod_get( par->gram, reduce - 1 )
									: (Production*)NULL;
		}

		LOG( "shift = %d, reduce = %d", shift, ctx->reduce );

		if( !shift && !ctx->reduce )
		{
			/* Parse Error */
			/* TODO: Error Recovery */
			fprintf( stderr, "Parse Error on %s at >%.*s<\n",
						tok->symbol->name, 30, tok->start );

			for( i = 2; i < par->dfa[tos->dstate][0]; i += 3 )
			{
				Symbol*	la;

				la = sym_get( par->gram, par->dfa[tos->dstate][i] - 1 );
				fprintf( stderr, "state %d, expecting '%s'\n",
					tos->dstate, la->name );
			}

			MSG( "Parsing failed" );
			RETURN( ( ctx->state = STAT_ERROR ) );
		}
	}
	while( !shift && ctx->reduce );

	if( ctx->reduce )
		LOG( "shift on %s and reduce by production %d\n",
					tok->symbol->name, ctx->reduce->idx );
	else
		LOG( "shift on %s to state %d\n", tok->symbol->name, shift - 1 );

	tos = (LRstackitem*)parray_malloc( &ctx->stack );

	tos->sym = tok->symbol;
	tos->state = ctx->reduce ? 0 : shift - 1;

	if( !tos->sym->flags.whitespace )
		tos->dstate = tos->state;

	tos->token = *tok;

	/* Shifted symbol becomes AST node? */
	if( tok->symbol->emit )
	{
		ctx->last = tos->node = ast_create(
				tok->symbol->emit, tok->symbol, NULL, tok, (AST_node*)NULL );
	}

	MSG( "Next token required" );
	RETURN( ( ctx->state = STAT_NEXT ) );
}


/* Helper for par_parse() */
static Symbol* par_scan( Parser* par, plex* lex,
							char** start, char** end, pboolean lazy )
{
	unsigned int	tok;
	Symbol*			sym;

	PROC( "par_scan" );


	while( TRUE )
	{
		if( ( !lazy && ( tok = plex_lex( lex, *start, end ) ) )
			|| ( lazy && ( *start = plex_next( lex, *start, &tok, end ) ) ) )
		{
			VARS( "tok", "%d", tok );

			if( !( sym = sym_get( par->gram, tok ) )
				|| sym->flags.whitespace )
			{
				/* fprintf( stderr, "w = >%.*s<\n", *end - *start, *start ); */
				*start = *end;
				continue;
			}
		}
		else
			sym	= par->gram->eof;

		break;
	}

	if( sym )
		LOG( "Next token '%s' @ >%.*s<\n", sym->name, *end - *start, *start );

	RETURN( sym );
}

/** Parse //start// with parser //par//.

If parsing succeeds, the function returns TRUE. And AST_node-pointer is then
saved to //root// if provided. */
pboolean par_parse( AST_node** root, Parser* par, char* start )
{
	plex*			lex;
	char*			end;
	pboolean		lazy	= TRUE;
	pboolean		ret		= FALSE;
	unsigned int	i;
	Symbol*			sym;
	Parser_ctx		ctx;
	Parser_stat		stat;
#if TIMEMEASURE
	clock_t			cstart;
	int				toks	= 0;
#endif

	Token			tok;

	PROC( "par_parse" );
	PARMS( "root", "%p", root );
	PARMS( "par", "%p", par );

	if( !( par && start ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

#if TIMEMEASURE
	cstart = clock();
#endif

	if( !( lex = par_autolex( par ) ) )
	{
		MSG( "Unable to create a lexer from this parser" );
		WRONGPARAM;

		RETURN( FALSE );
	}

	/* lex->flags |= PREGEX_RUN_DEBUG; */

	for( i = 0; ( sym = sym_get( par->gram, i ) ); i++ )
	{
		if( SYM_IS_TERMINAL( sym ) && sym->flags.whitespace )
		{
			lazy = FALSE;
			break;
		}
	}

	VARS( "lazy", "%s", BOOLEAN_STR( lazy ) );

	parctx_init( &ctx, par );

	do
	{
		sym = par_scan( par, lex, &start, &end, lazy );
#if TIMEMEASURE
		toks++;
#endif

		LOG( "symbol %d, %s", sym->idx, sym->name );

		tok.symbol = sym;
		tok.start = start;
		tok.end = end;
		tok.len = end - start;

		stat = parctx_next( &ctx, &tok );

		if( end > start && ctx.last
				&& stat != STAT_ERROR && !ctx.last->token.len )
		{
			ctx.last->token.start = start;
			ctx.last->token.len = end - start;
			ctx.last->token.end = end;
		}

		switch( stat )
		{
			case STAT_NEXT:
				MSG( "Next symbol requested" );
				start = end;
				break;

			case STAT_DONE:
				MSG( "We have a successful parse!" );

				if( root )
				{
					*root = ctx.ast;
					ctx.ast = (AST_node*)NULL;
				}

				sym = (Symbol*)NULL;
				ret = TRUE;

				break;

			default:
				sym = (Symbol*)NULL;
				break;
		}
	}
	while( sym );

	parctx_reset( &ctx );
	plex_free( lex );

#if TIMEMEASURE
	fprintf( stderr, "%d tokens, %lf secs\n", toks,
		(double)(clock() - cstart) / CLOCKS_PER_SEC );
#endif

	RETURN( ret );
}
