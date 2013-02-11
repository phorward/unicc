/*
 * File:        p_parse.h
 * Parser:		UniCC Grammar Definition Language
 * Version:		1.5.1
 * Copyright:	Copyright (C) 2008-2013 by Jan Max Meyer, Phorward Software Technologies
 * Description:	Parser for UniCC parser definitions
 *
 * UniCC Parser Template for C - Version 1.0
 * Copyright (C) 2006-2013 by Phorward Software Technologies, Jan Max Meyer
 */

#ifndef P_PARSE_H
#define P_PARSE_H

/* Wide character processing enabled? */
#ifndef UNICC_WCHAR
#define UNICC_WCHAR					0
#endif

/* UTF-8 processing enabled? */
#if !UNICC_WCHAR
#ifndef UNICC_UTF8
#	define UNICC_UTF8				1
#endif
#else
#	ifdef UNICC_UTF8
#	undef UNICC_UTF8
#	endif
#	define UNICC_UTF8				0
#endif

/* UNICC_CHAR is used as character type for internal processing */
#ifndef UNICC_CHAR
#if UNICC_UTF8 || UNICC_WCHAR
#	define UNICC_CHAR				wchar_t
#	define UNICC_CHAR_FORMAT		"%S"
#else
#	define UNICC_CHAR				char
#	define UNICC_CHAR_FORMAT		"%s"
#endif
#endif /* UNICC_CHAR */

/* UNICC_SCHAR defines the character type for semantic action procession */
#ifndef UNICC_SCHAR
#if UNICC_WCHAR
#	define UNICC_SCHAR				wchar_t
#	define UNICC_SCHAR_FORMAT		"%S"
#else
#	define UNICC_SCHAR				char
#	define UNICC_SCHAR_FORMAT		"%s"
#endif
#endif /* UNICC_SCHAR */

/* Boolean */
#ifndef UNICC_BOOLEAN
#define UNICC_BOOLEAN			short
#endif

/* Debug level */
#ifndef UNICC_DEBUG
#define UNICC_DEBUG				0
#endif

/* Stack debug switch */
#ifndef UNICC_STACKDEBUG
#define UNICC_STACKDEBUG		0
#endif

/* Parse error macro */
#ifndef UNICC_PARSE_ERROR
#define UNICC_PARSE_ERROR( pcb ) \
	fprintf( stderr, "line %d, column %d: syntax error on symbol %d, token '" \
		UNICC_SCHAR_FORMAT "'\n", \
	( pcb )->line, ( pcb )->column, pcb->sym, _lexem( pcb ) )
#endif

/* Input buffering clean-up */
#ifndef UNICC_CLEARIN
#define UNICC_CLEARIN( pcb )		_clear_input( pcb )
#endif

/*TODO:*/
#ifndef UNICC_NO_INPUT_BUFFER
#define UNICC_NO_INPUT_BUFFER	0
#endif

/* Memory allocation step size for dynamic stack- and buffer allocation */
#ifndef UNICC_MALLOCSTEP
#define UNICC_MALLOCSTEP		128
#endif

/* Call this when running out of memory during memory allocation */
#ifndef UNICC_OUTOFMEM
#define UNICC_OUTOFMEM			fprintf( stderr, \
									"Fatal error, ran out of memory\n" ), \
								exit( 1 )
#endif

/* Static switch */
#ifndef UNICC_STATIC
#define UNICC_STATIC			static
#endif

#ifdef UNICC_PARSER
#undef UNICC_PARSER
#endif
#define UNICC_PARSER			"" "debug"

/* Don't change next three defines below! */
#ifndef UNICC_ERROR
#define UNICC_ERROR				0
#endif
#ifndef UNICC_SHIFT
#define UNICC_SHIFT				2
#endif
#ifndef UNICC_REDUCE
#define UNICC_REDUCE			1
#endif

/* Error delay after recovery */
#ifndef UNICC_ERROR_DELAY
#define UNICC_ERROR_DELAY		3
#endif

/* Syntax tree construction */
#ifndef UNICC_SYNTAXTREE
#define UNICC_SYNTAXTREE		0
#endif

/* Enable/Disable terminal selection in semantic actions */
#ifndef UNICC_SEMANTIC_TERM_SEL
#define UNICC_SEMANTIC_TERM_SEL	0
#endif

/* Value Types */
typedef union _VTYPE
{
	uchar* value_0;
	BOOLEAN value_1;
	LIST* value_2;
	SYMBOL* value_3;
	PROD* value_4;
	int value_5;
	pregex_ptn* value_6;
} _vtype;



/* Typedef for symbol information table */
typedef struct
{
	char*			name;
	short			type;
	UNICC_BOOLEAN	lexem;
	UNICC_BOOLEAN	whitespace;
	UNICC_BOOLEAN	greedy;
} _syminfo;

/* Typedef for production information table */
typedef struct
{
	char*	definition;
	int		length;
	int		lhs;
} _prodinfo;


/* Stack Token */
typedef struct
{
	_vtype		value;

	_syminfo*	symbol;
	int					state;
	unsigned int		line;
	unsigned int		column;
} _tok;


#if UNICC_SYNTAXTREE
/* Parse tree node */
typedef struct _SYNTREE _syntree;

struct _SYNTREE
{
	_tok		symbol;
	UNICC_SCHAR*		token;

	_syntree*	parent;
	_syntree*	child;
	_syntree*	prev;
	_syntree*	next;
}; 
#endif

/* Parser Control Block */
typedef struct
{
	/* Stack */
	_tok*		stack;
	_tok*		tos;

	/* Stack size */
	unsigned int		stacksize;
	
	/* Values */
	_vtype		ret;
	_vtype		test;
	
	/* State */
	int					act;
	int					idx;
	int					lhs;
	
	/* Lookahead */
	int					sym;
	int					old_sym;
	unsigned int		len;

	/* Input buffering */
	UNICC_SCHAR*		lexem;
	UNICC_CHAR*			buf;
	UNICC_CHAR*			bufend;
	UNICC_CHAR*			bufsize;

	/* Lexical analysis */
	UNICC_CHAR			next;
	UNICC_CHAR			eof;
	UNICC_BOOLEAN		is_eof;
	
	/* Error handling */
	int					error_delay;
	int					error_count;
	
	unsigned int		line;
	unsigned int		column;

#if UNICC_SYNTAXTREE
	/* Syntax tree */
	_syntree*	syntax_tree;
#endif
	
	/* User-defined components */
	

} _pcb;

#endif /* P_PARSE_H */
