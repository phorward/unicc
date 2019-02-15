/* -HEADER----------------------------------------------------------------------
UniCC² Parser Generator
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	parse.h
Usage:	Phorward parsing library
----------------------------------------------------------------------------- */

#include "phorward.h"

#define UNICC_VERSION		"2.0.0-dev"

typedef struct _Symbol		Symbol;
typedef struct _Production	Production;
typedef struct _Grammar		Grammar;
typedef struct _AST_node	AST_node;

typedef struct _Parser_ctx	Parser_ctx;

/* Flags for grammars and their objects */
#define FLAG_NONE			0x00
#define FLAG_CALLED			0x01
#define FLAG_DEFINED		0x02
#define FLAG_NULLABLE		0x04
#define FLAG_LEFTREC		0x08
#define FLAG_LEXEM			0x10
#define FLAG_WHITESPACE		0x20
#define FLAG_PREVENTLREC	0x40
#define FLAG_NAMELESS		0x80
#define FLAG_GENERATED		0x100
#define FLAG_FREENAME		0x200
#define FLAG_FREEEMIT		0x400
#define FLAG_SPECIAL		0x800
#define FLAG_FINALIZED		0x1000
#define FLAG_FROZEN			0x2000

#define MOD_OPTIONAL		'?'
#define MOD_POSITIVE		'+'
#define MOD_KLEENE			'*'

#define LR_SHIFT			1
#define LR_REDUCE			2

/* Associativity */
typedef enum
{
	ASSOC_NONE,
	ASSOC_NOT,
	ASSOC_LEFT,
	ASSOC_RIGHT
} Assoc;

/* Production */
struct _Production
{
	Grammar*				grm;		/* Grammar */

	unsigned int			idx;		/* Production index */
	Symbol*					lhs;		/* Left-hand side */
	plist*					rhs;		/* Left-hand side items */
	unsigned int			flags;		/* Configuration flags */

	Assoc					assoc;		/* LR associativity */
	unsigned int			prec;		/* LR precedence level */

	char*					emit;		/* AST emitting node */

	char*					strval;		/* String represenation */
};

/* Symbol (both terminal and nonterminal for easier access in productions) */
struct _Symbol
{
	Grammar*				grm;		/* Grammar */

	unsigned int			idx;		/* Symbol index */
	char*					name;		/* Unique name */
#ifndef SYM_T_EOF
#define SYM_T_EOF			"&eof"
#endif

	unsigned int			flags;		/* Configuration flags */

	Assoc					assoc;		/* LR associativity */
	unsigned int			prec;		/* LR precedence level */

	plist*					first;		/* Set of FIRST() symbols */

	char*					emit;		/* AST emitting node */
	pregex_ptn*				ptn;		/* Pattern definition (terminals!) */

	char*					strval;		/* String representation */
};

#ifndef SYM_IS_TERMINAL
#define SYM_IS_TERMINAL( sym )	( !sym->name || !( *sym->name ) \
										|| !islower( *( sym )->name ) )
#endif

/* Grammar */
struct _Grammar
{
	plist*					symbols;	/* Symbols (both nonterminals
														and terminals) */
	plist*					prods;		/* Productions */

	Symbol*					goal;		/* The start/goal symbol */
	Symbol*					eof;		/* End-of-input symbol */

	unsigned int			flags;		/* Configuration flags */

	char*					strval;		/* String representation */
};

/* AST */
struct _AST_node
{
	char*					emit;		/* AST node name */

	Symbol*					sym;		/* Emitting symbol */
	Production*				prod;		/* Emitting production */

	/* Semantics */
	pany*					val;		/* Value */

	/* Match */
	char*					start;		/* Begin of fragment */
	char*					end;		/* End of fragment */
	size_t					len;		/* Fragment length */

	/* Source */
	unsigned long			row;		/* Appearance in row */
	unsigned long			col;		/* Appearance in column */

	/* AST */
	AST_node*				child;		/* First child element */
	AST_node*				prev;		/* Previous element in current scope */
	AST_node*				next;		/* Next element in current scope */
};

/* AST traversal */
typedef enum
{
	AST_EVAL_TOPDOWN,
	AST_EVAL_PASSOVER,
	AST_EVAL_BOTTOMUP
} Ast_eval;

typedef void (*Ast_evalfn)( Ast_eval type, AST_node* node );

/* Parser states */
typedef enum
{
	STAT_INITIAL,
	STAT_DONE,
	STAT_NEXT,
	STAT_ERROR
} Parser_stat;

/* Parser */
typedef struct
{
	/* Grammar */
	Grammar*				gram;		/* Grammar */

	/* Parser */
	unsigned int			states;		/* States count */
	unsigned int**			dfa;		/* Parse table */

} Parser;

/* Parser context */
typedef void (*Parser_reducefn)( Parser_ctx* ctx );

struct _Parser_ctx
{
	Parser*					par;		/* Parser */

	Parser_stat				state;		/* State */
	Production*				reduce;		/* Reduce */
	parray					stack;		/* Stack */
	AST_node*				ast;		/* AST */

	Parser_reducefn			reducefn;	/* Reduce function */
} ;

/* Macro: GRAMMAR_DUMP */
#ifdef DEBUG
	#define GRAMMAR_DUMP( g ) \
		__dbg_gram_dump( __FILE__, __LINE__, _dbg_proc_name, #g, g )
#else
	#define GRAMMAR_DUMP( g )
#endif

/* Macro: AST_DUMP */
#ifdef DEBUG
	#define AST_DUMP( ast ) \
		__dbg_ast_dump( __FILE__, __LINE__, _dbg_proc_name, #ast, ast )
#else
	#define AST_DUMP( ast )
#endif

#include "proto.h"
