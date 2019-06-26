/* -MODULE----------------------------------------------------------------------
UniCC² Parser Generator
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
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
AST_node* ast_create( char* emit, Symbol* sym,
						Production* prod, AST_node* child )
{
	AST_node*	node;

	PROC( "ast_create" );
	PARMS( "emit", "%s", emit );
	PARMS( "sym", "%p", sym );
	PARMS( "prod", "%p", prod );
	PARMS( "child", "%p", child );

	if( !( emit && sym ) )
	{
		WRONGPARAM;
		RETURN( (AST_node*)NULL );
	}

	node = (AST_node*)pmalloc( sizeof( AST_node ) );

	node->emit = emit;

	node->sym = sym;
	node->prod = prod;

	node->child = child;

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

/** Dump detailed //ast// to //stream//. */
void ast_dump( FILE* stream, AST_node* ast )
{
	static int lev		= 0;
	int	i;

	while( ast )
	{
		for( i = 0; i < lev; i++ )
			fprintf( stream, " " );

		fprintf( stream, "{ %s ", ast->emit );

		if( ast->val )
			fprintf( stream, " (%p)", ast->val );
		else if( ast->start && ast->len )
			fprintf( stream, ">%.*s<", (int)ast->len, ast->start );

		fprintf( stream, "\n" );

		if( ast->child )
		{
			lev++;
			ast_dump( stream, ast->child );
			lev--;
		}

		for( i = 0; i < lev; i++ )
			fprintf( stream, " " );

		fprintf( stream, "{ %s ", ast->emit );

		if( ast->val )
			fprintf( stream, " (%p)", ast->val );
		else if( ast->start && ast->len )
			fprintf( stream, ">%.*s<", (int)ast->len, ast->start );

		fprintf( stream, "\n" );

		ast = ast->next;
	}
}

/** Dump simplified //ast// to //stream//.

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

		if( SYM_IS_TERMINAL( ast->sym ) || ast->sym->flags.lexem )
		{
			if( ast->val )
				fprintf( stream, " (%p)", ast->val );
			else if( ast->start && ast->len )
				fprintf( stream, " >%.*s<", (int)ast->len, ast->start );
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
	if( !_dbg_trace_enabled( file, function ) )
		return;

	_dbg_trace( file, line, "AST", function, "%s = {", name );
	ast_dump_short( stdout, ast );
	_dbg_trace( file, line, "AST", function, "}" );
}

/** Dump //ast// to //stream// as JSON-formatted string.

Only opening matches are printed. */
void ast_dump_json( FILE* stream, AST_node* ast )
{
	char*	ptr;
	char*	eptr;
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
		if( SYM_IS_TERMINAL( ast->sym ) || node->sym->flags.lexem )
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
			else if( node->start && node->end )
			{
				ptr = node->start;
				eptr = node->end;
			}
			else
				ptr = eptr = (char*)NULL;

			if( ptr && eptr && ptr < eptr )
			{
				for( ; *ptr && ptr < eptr; ptr++ )
					if( *ptr == '\"' )
						fprintf( stream, "\\\"" );
					else
						fputc( *ptr, stream );
			}

			fputc( '"', stream );
		}

		/* Position */
		fprintf( stream, ",\"row\":%ld,\"column\":%ld", node->row, node->col );

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
		else if( ast->start )
			fprintf( stream, "'%.*s' ", (int)ast->len, ast->start );
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
	if( ast->len < BUFSIZ )
		sprintf( buf, "%.*s", (int)ast->len, ast->start );
	else
		ptr = pstrndup( ast->start, ast->len );

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
	Symbol*			sym;			/* Symbol */

	char*			start;			/* Start */

	AST_node*		node;			/* AST construction */

	int				row;			/* Positioning in source */
	int				col;			/* Positioning in source */
} pplrse;


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

	if( !gram_prepare( g ) )
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

	GRAMMAR_DUMP( g );
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
				(int)sym->idx, PREGEX_COMP_STATIC );
		}
	}

	VARS( "lex", "%p", lex );
	RETURN( lex );
}

/** Dumps parser to JSON */
pboolean par_dump_json( FILE* stream, Parser* par )
{
	int		i;
	int		j;

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

	fprintf( stream, "{\n\t\"grammar\":\n" );
	gram_dump_json( stream, par->gram );

	/* LR parser */
	fprintf( stream, ",\n\t\"states\": [\n" );

	for( i = 0; i < par->states; i++ )
	{
		fprintf( stream, "\t\t{\n" );

		if( par->dfa[ 1 ] )
			fprintf( stream, "\t\t\t\"reduce-default\": %d,\n",
				( par->dfa[ i ][ 1 ] - 1 ) );

		else
			fprintf( stream, "\t\t\t\"reduce-default\": null,\n" );

		fprintf( stream, "\t\t\t\"actions\": [\n" );

		/* Actions */
		for( j = 2; j < par->dfa[ i ][ 0 ]; j += 3 )
		{
			fprintf( stream, "\t\t\t\t{ \"symbol\": %d, ", par->dfa[ i ][ j ] - 1 );

			if( par->dfa[ i ][ j + 1 ] & LR_REDUCE )
				fprintf( stream, "\"action\": \"%s\", \"production\": %d",
					par->dfa[ i ][ j + 1 ] == ( LR_SHIFT | LR_REDUCE ) ?
						"shift&reduce" : "reduce",
					(int)( par->dfa[ i ][ j + 2 ] - 1 ) );
			else if( par->dfa[ i ][ j + 1 ] & LR_SHIFT )
				fprintf( stream, "\"action\": \"shift\", \"state\": %d",
					(int)( par->dfa[ i ][ j + 2 ] - 1 ) );

			fprintf( stream, "}%s\n", j + 3 < par->dfa[ i ][ 0 ] ? "," : "" );
		}

		fprintf( stream, "\t\t\t]\n\t\t}%s\n", i + 1 < par->states ? "," : "" );
	}

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

	parray_init( &ctx->stack, sizeof( pplrse ), 0 );

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

#if 0
/* Function to dump the parse stack content */
static void print_stack( char* title, parray* stack )
{
	pplrse*	e;
	int		i;

	fprintf( stderr, "STACK DUMP %s\n", title );

	for( i = 0; i < parray_count( stack ); i++ )
	{
		e = (pplrse*)parray_get( stack, i );
		fprintf( stderr, "%02d: %s %d >%.*s<\n",
			i, e->sym->name, e->state
				e->end - e->start, e->start );
	}
}
#endif


/** Let parser and context //ctx// run on next token //sym//.

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
Parser_stat parctx_next( Parser_ctx* ctx, Symbol* sym )
{
	int			i;
	pplrse*		tos;
	int			shift;
	int			reduce;
	Parser*		par;

	PROC( "parctx_next" );

	if( !( ctx && sym ) )
	{
		WRONGPARAM;
		RETURN( STAT_ERROR );
	}

	PARMS( "ctx", "%p", ctx );
	PARMS( "sym", "%p", sym );

	par = ctx->par;

	/* Initialize parser on first call */
	if( ctx->state == STAT_INITIAL )
	{
		MSG( "Initial call" );
		parctx_reset( ctx );

		tos = (pplrse*)parray_malloc( &ctx->stack );
		tos->sym = par->gram->goal;
	}
	else
		tos = (pplrse*)parray_last( &ctx->stack );

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

			for( i = 0; i < plist_count( ctx->reduce->rhs ); i++ )
			{
				tos = (pplrse*)parray_pop( &ctx->stack );

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
			}

			tos = (pplrse*)parray_last( &ctx->stack );

			/* Construction of AST node */
			if( ctx->reduce->emit || ctx->reduce->lhs->emit )
				ctx->last = ast_create( ctx->reduce->emit ? ctx->reduce->emit
												: ctx->reduce->lhs->emit,
								ctx->reduce->lhs, ctx->reduce, ctx->last );

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

			tos = (pplrse*)parray_malloc( &ctx->stack );

			tos->sym = ctx->reduce->lhs;
			tos->state = shift - 1;
			tos->node = ctx->last;

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
				if( par->dfa[tos->state][i] == sym->idx + 1 )
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

		VARS( "shift", "%d", shift );
		VARS( "ctx->reduce", "%d", ctx->reduce );

		if( !shift && !ctx->reduce )
		{
			/* Parse Error */
			/* TODO: Error Recovery */
			fprintf( stderr, "Parse Error @ %s\n", sym->name );

			for( i = 2; i < par->dfa[tos->state][0]; i += 3 )
			{
				Symbol*	sym;

				sym = sym_get( par->gram, par->dfa[tos->state][i] - 1 );
				fprintf( stderr, "state %d, expecting '%s'\n",
					tos->state, sym->name );
			}

			MSG( "Parsing failed" );
			RETURN( ( ctx->state = STAT_ERROR ) );
		}
	}
	while( !shift && ctx->reduce );

	if( ctx->reduce )
		LOG( "shift on %s and reduce by production %d\n",
					sym->name, ctx->reduce->idx );
	else
		LOG( "shift on %s to state %d\n", sym->name, shift - 1 );

	tos = (pplrse*)parray_malloc( &ctx->stack );

	tos->sym = sym;
	tos->state = ctx->reduce ? 0 : shift - 1;

	/* Shifted symbol becomes AST node? */
	if( sym->emit )
		ctx->last = tos->node = ast_create(
			sym->emit, sym, (Production*)NULL, (AST_node*)NULL );

	MSG( "Next token required" );
	RETURN( ( ctx->state = STAT_NEXT ) );
}

/** Let parser //ctx// run on next token identified by //name//.

The function is a wrapper for parctx_next() with same behavior.
*/
Parser_stat parctx_next_by_name( Parser_ctx* ctx, char* name )
{
	Symbol*		sym;

	PROC( "parctx_next_by_name" );
	PARMS( "ctx", "%p", ctx );
	PARMS( "name", "%s", name );

	if( !( ctx && name && *name ) )
	{
		WRONGPARAM;
		RETURN( STAT_ERROR );
	}

	if( !( sym = sym_get_by_name( ctx->par->gram, name ) ) )
	{
		WRONGPARAM;

		LOG( "Token named '%s' does not exist in the grammar", name );
		RETURN( STAT_ERROR );
	}

	if( !SYM_IS_TERMINAL( sym ) )
	{
		WRONGPARAM;

		LOG( "Symbol named '%s' is not a terminal symbol", name );
		RETURN( STAT_ERROR );
	}

	RETURN( parctx_next( ctx, sym ) );
}


/** Let parser //ctx// run on next token identified by //idx//.

The function is a wrapper for parctx_next() with same behavior.
*/
Parser_stat parctx_next_by_idx( Parser_ctx* ctx, unsigned int idx )
{
	Symbol*		sym;

	PROC( "parctx_next_by_idx" );
	PARMS( "ctx", "%p", ctx );
	PARMS( "idx", "%d", idx );

	if( !( ctx ) )
	{
		WRONGPARAM;
		RETURN( STAT_ERROR );
	}

	if( !( sym = sym_get( ctx->par->gram, idx ) ) )
	{
		WRONGPARAM;

		LOG( "Token with index %d does not exist in the grammar", idx );
		RETURN( STAT_ERROR );
	}

	LOG( "Token = %s", sym->name );

	if( !SYM_IS_TERMINAL( sym ) )
	{
		WRONGPARAM;

		LOG( "Symbol with index %d is not a terminal symbol", idx );
		RETURN( STAT_ERROR );
	}

	RETURN( parctx_next( ctx, sym ) );
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

	PROC( "par_parse" );
	PARMS( "root", "%p", root );
	PARMS( "par", "%p", par );

	if( !( par && start ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	if( !( lex = par_autolex( par ) ) )
	{
		MSG( "Unable to create a lexer from this parser" );
		WRONGPARAM;

		RETURN( FALSE );
	}

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

		LOG( "symbol %d, %s", sym->idx, sym->name );

		stat = parctx_next( &ctx, sym );

		if( end > start && ctx.last && stat != STAT_ERROR && !ctx.last->len )
		{
			ctx.last->start = start;
			ctx.last->len = end - start;
			ctx.last->end = end;
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

	RETURN( ret );
}
