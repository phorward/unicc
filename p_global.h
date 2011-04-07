/* -HEADER----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2009 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ mail@phorward-software.com

File:	p_global.h
Author:	Jan Max Meyer
Usage:	Global declarations, structures and includes
----------------------------------------------------------------------------- */

#ifndef P_GLOBAL_H
#define P_GLOBAL_H
#pragma warning( disable: 4996 )

/*
 * Includes
 */

/* Standard Library */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* Phorward C Library */
#include <uchar.h>
#undef uchar
#define uchar char
#include <boolean.h>
#include <dmem.h>
#include <bitset.h>
#include <llist.h>
#include <re.h>
#include <hashtab.h>
#include <xml.h>

/* Internal includes */
#include "p_defs.h"

/*
 * Defines
 */

/* Symbol types */
#define SYM_UNDEFINED			-1
#define SYM_NON_TERMINAL		0
#define SYM_CCL_TERMINAL		1
#define SYM_KW_TERMINAL			2
#define SYM_REGEX_TERMINAL		3
#define SYM_EXTERN_TERMINAL		4
#define SYM_ERROR_RESYNC		5


/* Parser generation models */
#define MODEL_CONTEXT_SENSITIVE	0	/* Character-based parsing (default) */
#define MODEL_CONTEXT_FREE		1	/* Scanner-based parsing */

/* Macro to verify terminals */
#define IS_TERMINAL( s ) \
	( ((s)->type) > SYM_NON_TERMINAL )

/* Associativity */
#define ASSOC_NONE				0
#define ASSOC_LEFT				1
#define ASSOC_RIGHT				2
#define ASSOC_NOASSOC			3

/* Parser actions */
#define REDUCE					1 /* 0 1 */
#define SHIFT					2 /* 1 0 */
#define SHIFT_REDUCE			3 /* 1 1 */

/* Number of hash-table buckets */
#define BUCKET_COUNT			64

/* Length of a string that is always long enough (sorry, insider!) */
#define ONE_LINE				80

/* Generator insertion wildcards */
#define GEN_WILD_PREFIX			"@@"

/* Phorward UniCC parser generator version number */
#define PHORWARD_VERSION		"0.19"
#define PHORWARD_DEFAULT_LNG	"C"

/* Phorward UniCC parser generator error protocols */
#define ERR_PROT_CONSOLE		0
#define ERR_PROT_XML			1

/*
 * Macros
 */

/* Memory handling */
#define p_malloc( size )			MALLOC( size )
#define p_realloc( ptr, size )		REALLOC( ptr, size )
#define p_free( ptr )				FREE( ptr )
#define p_strdup( ptr )				STRDUP( ptr )
#define p_strlen( ptr )				( ptr ? strlen( ptr ) : 0 )
#define p_strzero( ptr )			( ptr ? ( *ptr ? 1 : 0 ) : 0 )

/*
 * Type definitions
 */
typedef struct _symbol 				SYMBOL;
typedef struct _def					DEF;
typedef struct _prod 				PROD;
typedef struct _item 				ITEM;
typedef struct _state 				STATE;
typedef struct _tabcol 				TABCOL;
typedef struct _vtype				VTYPE;
typedef struct _parser				PARSER;
typedef struct _generator			GENERATOR;
typedef struct _generator_1d_tab	_1D_TABLE;
typedef struct _generator_1d_btab	_1D_BOOL_TABLE;
typedef struct _generator_2d_tab	_2D_TABLE;

/*
 * Structure declarations
 */

/* Symbol structure */
struct _symbol
{
	int			id;				/* Symbol ID */
	int			type;			/* Symbol type */
	
	uchar*		name;			/* Symbol name */

	int*		char_map;		/* Compressed character map (terminals) */
	int			char_map_size;	/* Size of character map */

	LIST*		productions;	/* List of productions attached to a
									non-terminal symbol */
	LIST*		first;			/* The symbol's first set */

	LIST*		nfa_def;		/* Start node of nondeterministic finite
									automata of regular expression terminals
										(expanded "KEYWORD" feature!) */

	BOOLEAN		fixated;		/* Flag, if fixated symbols (do always shift!) */
	BOOLEAN		goal;			/* Flag, if goal non-terminal */
	BOOLEAN		nullable;		/* Flag, if nullable */
	BOOLEAN		defined;		/* Flag, if defined */
	BOOLEAN		used;			/* Flag, if used */
	BOOLEAN		lexem;			/* Flag, if distinguished as lexem */
	BOOLEAN		keyword;		/* Flag, if it is a keyword
									(or if it has been derived from a
										keyword) */
	BOOLEAN		whitespace;		/* Flag, if it is a whitespace */
	BOOLEAN		extern_token;	/* Flag, if it is an external token not	
									recognized by Phorward */
	BOOLEAN		generated;		/* Flag, if automatically generated
									symbol */

	SYMBOL*		derived_from;	/* Pointer to symbol from which the
									current has been derived */

	VTYPE*		vtype;			/* Pointer to value type the symbol
									is associated with */

	int			prec;			/* Level of precedence */
	int			assoc;			/* Associativity */

	int			line;			/* Line of definition */

	uchar*		code;			/* Code for regex terminals */
	int			code_at;		/* Beginning line of code-segment
									in source file */
};

/* Production structure */
struct _prod
{
	int			id;				/* Production ID */

	SYMBOL*		lhs;			/* Left-hand side symbol */
	
	LIST*		rhs;			/* Right-hand side symbols */
	
	LIST*		rhs_idents;		/* List of offset identifiers for rhs items;
									This list is hold equivalent to the
										right-hand side symbol list */
	
	LIST*		sem_rhs;		/* Semantic right-hand side; This
									may differ from the right hand side
										in case of embedded productions,
											to allow a mixture of the
												outer- and inner-production */
	LIST*		sem_rhs_idents;	/* Semantic right-hand side identifiers;
									Must be equivalent to sem_rhs */

	int			prec;			/* Precedence level */
	int			assoc;			/* Associativity flag */
	
	uchar*		code;			/* Semantic reduction action template */
	int			code_at;		/* Beginning line of code-segment
									in source file */
};

/* Closure item */
struct _item
{
	PROD*		prod;			/* The associated production
									to this item */	
	int			dot_offset;		/* The dot's offset from the left
									of the right hand side */
	SYMBOL*		next_symbol;	/* Symbol following the dot */
	LIST*		lookahead;		/* Set of lookahead-symbols */
};

/* LALR(1) State */
struct _state
{
	int			state_id;		/* State ID */
	LIST*		kernel;			/* Kernel item set */
	LIST*		epsilon;		/* Epsilon item set */
	
	LIST*		actions;		/* Action table entries */
	LIST*		gotos;			/* Goto table entries */
	
	PROD*		def_prod;		/* Default production */
	
	BOOLEAN		done;			/* Done flag */
	BOOLEAN		closed;			/* Closed flag */

	LIST*		dfa;			/* DFA machine for keyword recognition
									in this state */
};

/* Action/Goto table column */
struct _tabcol
{
	SYMBOL*		symbol;			/* Symbol */
	short		action;			/* Action on this symbol */
	int			index;			/* Action-index on this symbol */
};

/* Value stack type */
struct _vtype
{
	int			id;				/* Value type ID */
	uchar*		int_name;		/* Internal name for verification */
	uchar*		real_def;		/* Definition by user */
};

/* Parser information structure */
struct _parser
{
	HASHTAB*	definitions;	/* Symbol hash table for faster symbol access */
	LIST*		symbols;		/* Linked list for sequencial symbol management */
	LIST*		productions;	/* Linked list of productions */
	LIST*		lalr_states;	/* Linked list of LALR(1) states */
	LIST*		dfa;			/* List containing the DFA for keyword recognition */
	
	SYMBOL*		goal;			/* Pointer to the goal non-terminal */
	SYMBOL*		end_of_input;	/* End of input symbol */
	SYMBOL*		error;			/* Error token */
	
	LIST*		kw;				/* Keyword recognition machines */
	LIST*		vtypes;			/* Value stack types */

	LIST*		nfa_m;			/* List containing all NFA-states of regular expression
									tokens */

	short		p_model;		/* Parser model */
	uchar*		p_name;			/* Parser name */
	uchar*		p_desc;			/* Parser description */
	uchar*		p_language;		/* Parser target programming language */
	uchar*		p_copyright;	/* Parser copyright notice */
	uchar*		p_version;		/* Parser version */
	uchar*		p_prefix;		/* Parser symbol prefix */
	uchar*		p_def_action;	/* Default reduce action */
	uchar*		p_def_action_e;	/* Default reduce action for epsilon-productions */

	uchar*		p_invalid_suf;	/* Character class for invalid keyword suffix characters,
									if (char*)NULL, empty table */
	BOOLEAN		p_neg_inv_suf;	/* Defines if p_invalid_suf is negated or not. This must
									be explicitly stored because p_universe is possibly not
										known when this is read. */
	BOOLEAN		p_lexem_sep;	/* Flag, if lexem separation is switched ON or not */
	BOOLEAN		p_cis_keywords;	/* Flag, if case-insensitive keywords */
	BOOLEAN		p_cis_types;	/* Flag, if case-insenstivie value stack types */
	BOOLEAN		p_extern_tokens;/* Flag if parser uses external tokens */
	int			p_universe;		/* Maximum of the character universe */

	uchar*		p_header;		/* Header/Prologue program code of the parser */
	uchar*		p_footer;		/* Footer/Epilogue embedded program code of the parser */

	uchar*		source;			/* Parser definition source */

	/* Context-free model relevant */
	LIST*		lexer;

	/* Parser runtime switches */
	BOOLEAN		stats;
	BOOLEAN		verbose;
	BOOLEAN		show_states;
	BOOLEAN		show_grammar;
	BOOLEAN		show_productions;
	BOOLEAN		optimize_states;
	BOOLEAN		all_warnings;

	/* Debug and maintainance */
	char*		filename;
	int			debug_level;
};


/* Generator 2D table structure */
struct _generator_2d_tab
{
	uchar*	row_start;
	uchar*	row_end;
	uchar*	col;
	uchar*	col_sep;
	uchar*	row_sep;
};

/* Generator 1D table structur */
struct _generator_1d_tab
{
	uchar*	col;
	uchar*	col_sep;
};

/* Generator 1D boolean table structur */
struct _generator_1d_btab
{
	uchar*	col_true;
	uchar*	col_false;
	uchar*	col_sep;
};

/* Generator template structure */
struct _generator
{
	uchar*			name;						/* Target language name */
	uchar*			driver;						/* Driver source code */
	uchar*			vstack_def_type;			/* Default-type for nonterminals,
													if no type is specified */
	uchar*			vstack_term_type;			/* Type for terminals (characters)
													to be pushed on the value
														stack */
	_2D_TABLE		acttab;						/* Action table */
	_2D_TABLE		gotab;						/* Goto table */
	_1D_TABLE		prodlen;					/* Production lengths */
	_1D_TABLE		prodlhs;					/* Production's left-hand sides */
	_1D_TABLE		charmap;					/* Character-class validation map */
	_1D_TABLE		dfa_select;					/* DFA machine selection */
	_2D_TABLE		dfa_idx;					/* DFA state index */
	_1D_TABLE		dfa_char;					/* DFA transition characters */
	_1D_TABLE		dfa_trans;					/* DFA transitions */
	_2D_TABLE		dfa_accept;					/* DFA accepting states */
	_1D_BOOL_TABLE	kw_invalid_suffix;			/* Invalid keyword-suffix
														character-map */
	_1D_BOOL_TABLE	whitespace;					/* Whitespace identification table
													(used by context-free model */
	_1D_TABLE		symbols;					/* Symbol name table
													(for debug) */
	_1D_TABLE		productions;				/* Production definition table
													(for debug) */

	uchar*			action_start;				/* Action code start */
	uchar*			action_end;					/* Action code end */
	uchar*			action_single;				/* Action vstack access */
	uchar*			action_union;				/* Action union access */
	uchar*			action_lhs_single;			/* Action left-hand side single access */
	uchar*			action_lhs_union;			/* Action left-hand side union access */
	uchar*			vstack_single;				/* Single value stack type definition */
	uchar*			vstack_union_start;			/* Begin of value stack union definition */
	uchar*			vstack_union_end;			/* End of value stack union definition */
	uchar*			vstack_union_def;			/* Union inline type definition */
	uchar*			vstack_union_att;			/* Union attribute name */
	uchar*			scan_action_start;			/* Scanner action code start */
	uchar*			scan_action_end;			/* Scanner action code end */
	uchar*			scan_action_begin_offset;	/* Begin-offset of scanned token in source */
	uchar*			scan_action_end_offset;		/* End-offset of scanned token in source */
	uchar*			scan_action_ret_single;		/* Semantic value, single access */
	uchar*			scan_action_ret_union;		/* Semantic value, union access */
	
	uchar*			code_localization;			/* Code localization template */

	uchar**			for_sequences;				/* Dynamic array of string sequences
													to be replaced/escaped in output
														strings */
	uchar**			do_sequences;				/* Dynamic array of escape-sequences
													for the particular string
														sequence in the above array */
	int				sequences_count;			/* Number of elements in the above
													array */

	XML_T			xml;						/* XML root node */
};

#endif

