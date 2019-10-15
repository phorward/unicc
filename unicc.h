/* -HEADER----------------------------------------------------------------------
UniCC² Parser Generator
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	parse.h
Usage:	Phorward parsing library
----------------------------------------------------------------------------- */

#include "lib/phorward.h"

#define UNICC_VERSION		"2.0.0-dev"

#define NAMELEN		        256                        /* fixme: make dynamic */
#define DERIVCHAR		    '\''

typedef struct _Symbol		Symbol;
typedef struct _Production	Production;
typedef struct _Grammar		Grammar;

typedef struct _Token		Token;
typedef struct _AST_node	AST_node;
typedef struct _Parser_ctx	Parser_ctx;

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

	struct
	{
		pboolean			leftrec		:1;	/* Left-recursive */
		pboolean			nullable	:1;

		pboolean			freeemit	:1;
	}						flags;

	Assoc					assoc;		/* LR associativity flag */
	unsigned int			prec;		/* LR precedence level */

	char*					emit;		/* AST emitting node */

	char*					strval;		/* String representation */
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
#ifndef SYM_T_WHITESPACE
#define SYM_T_WHITESPACE	"&whitespace"
#endif

	struct
	{
		pboolean			terminal	:1;
		pboolean			nameless	:1;
		pboolean			defined		:1;
		pboolean			nullable	:1;
		pboolean			lexem		:1;
		pboolean			whitespace	:1;
		pboolean			generated	:1;
		pboolean			special		:1;
		pboolean			leftrec		:1;	/* Left-recursive */

		pboolean			freename	:1;
		pboolean			freeemit	:1;

	}						flags;

	int						usages;		/* Number of usages of this symbol */

	Symbol*					origin;		/* Origin of generated symbol */

	Assoc					assoc;		/* LR associativity flag */
	unsigned int			prec;		/* LR precedence level */

	parray					first;		/* Set of FIRST() symbols */
	plist					prods;		/* Productions associated with symbol */

	char*					emit;		/* AST emitting node */

	pccl*					ccl;		/* Terminal: Character-class */
	char*					str;		/* Terminal: Static string */
	pregex_ptn*				ptn;		/* Terminal: Pattern definition */

	char*					strval;		/* String representation */
};

#ifndef SYM_IS_TERMINAL
#define SYM_IS_TERMINAL( sym )	\
	( sym->flags.terminal || sym->ccl || sym->str || sym->ptn ) /* fixme! */
#endif

/* Grammar */
struct _Grammar
{
	plist*					symbols;	/* Symbols (both non-terminals
														and terminals) */
	plist*					prods;		/* Productions */

	Symbol*					goal;		/* The start/goal symbol */
	Symbol*					eof;		/* End-of-input symbol */

	struct
	{
		pboolean			preventlrec	:1;	/* Prevent left-recursions */
		pboolean			finalized	:1; /* Grammar is finalized */
		pboolean			frozen		:1;	/* Grammar is in use, e.g. by
												a parser - no changes to its
												structure are allowed! */
		pboolean			debug		:1;	/* Debug behavor */
	}						flags;


	char*					strval;		/* String representation */
};

/* Token */
struct _Token
{
	Symbol*					symbol;

	char*					start;		/* Begin of fragment */
	char*					end;		/* End of fragment */
	size_t					len;		/* Fragment length */

	/* Source */
	unsigned long			row;		/* Appearance in row */
	unsigned long			col;		/* Appearance in column */
};

/* AST */
struct _AST_node
{
	char*					emit;		/* AST node name */

	Symbol*					sym;		/* Emitting symbol */
	Production*				prod;		/* Emitting production */

	/* Semantics */
	void*					val;		/* User-defined pointer */

	/* Match */
	Token					token;		/* Token information */

	/* AST */
	AST_node*				parent;		/* Parent element */
	AST_node*				child;		/* First child element */
	AST_node*				prev;		/* Previous element in current scope */
	AST_node*				next;		/* Next element in current scope */

	/* Traversal */
	void*					ptr;		/* User pointer to smth. useful */
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

	AST_node*				ast;		/* AST root node */
	AST_node*				last;		/* AST last node */

	Parser_reducefn			reducefn;	/* Reduce function */
};

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
