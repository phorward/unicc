/* -HEADER----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2011 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com/ ++ unicc<<AT>>phorward-software<<DOT>>com

File:	p_global.h
Author:	Jan Max Meyer
Usage:	Global declarations, structures and includes

You may use, modify and distribute this software under the terms and conditions
of the Artistic License, version 2. Please see LICENSE for more information.
----------------------------------------------------------------------------- */

#ifndef P_GLOBAL_H
#define P_GLOBAL_H

#ifdef _WIN32
#pragma warning( disable: 4996 )
#endif

/*
 * Includes
 */

/* Standard Includes */
#include <pbasis.h>
#include <pregex.h>

#undef uchar
#define uchar char

/* Internal includes */
#include "p_defs.h"

/*
 * Defines
 */

/* Symbol types */
#define SYM_UNDEFINED			-1
#define SYM_NON_TERMINAL		0
#define SYM_CCL_TERMINAL		1
#define SYM_REGEX_TERMINAL		2
#define SYM_SYSTEM_TERMINAL		3

/* Parser construction modes */
#define MODE_SENSITIVE			0	/* Sensitive parser construction mode */
#define MODE_INSENSITIVE		1	/* Insensitive parser construction mode */

/* Macro to verify terminals */
#define IS_TERMINAL( s ) \
	( ((s)->type) > SYM_NON_TERMINAL )

/* Associativity */
#define ASSOC_NONE				0
#define ASSOC_LEFT				1
#define ASSOC_RIGHT				2
#define ASSOC_NOASSOC			3

/* Parser actions */
#define ERROR					0	/* Force parse error */
#define REDUCE					1 	/* Reduce 			0 1 */
#define SHIFT					2 	/* Shift 			1 0 */
#define SHIFT_REDUCE			3 	/* Shift-Reduce 	1 1 */

/* Number of hash-table buckets */
#define BUCKET_COUNT			64

/* Length of a string that is always long enough (sorry, insider!) */
#define ONE_LINE				80

/* Generator insertion wildcards */
#define GEN_WILD_PREFIX			"@@"

/* Phorward UniCC parser generator version number */
#define UNICC_VERSION			"0.27.31"

/* Default target language */
#define UNICC_DEFAULT_LNG		"C"

/* File extensions */
#define UNICC_TLT_EXTENSION		".tlt"
#define UNICC_XML_EXTENSION		".xml"

/*
 * Macros
 */ 
#define OUTOFMEM				fprintf( stderr, \
									"%s, %d: Memory allocation failure; " \
										"UniCC possibly ran out of memory!\n", \
											__FILE__, __LINE__ ), \
								p_error( (PARSER*)NULL, ERR_MEMORY_ERROR,\
									ERRSTYLE_FATAL, __FILE__, __LINE__ ), \
								exit( EXIT_FAILURE )
									
#define MISS_MSG( txt )			fprintf( stderr, "%s, %d: %s\n", \
										__FILE__, __LINE__, txt )

/* Now uses pbasis library */
#define p_malloc( size )		pmalloc( size )
#define p_realloc( ptr, size )	prealloc( ptr, size )
#define p_free( ptr )			pfree( ptr )
#define p_strdup( ptr )			pstrdup( ptr )
#define p_strlen( ptr )			pstrlen( ptr )
#define p_strzero( ptr )		pstrzero( ptr )
#define p_tpl_insert			pstr_render
#define p_str_append			pstr_append_str

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
typedef struct _option				OPT;
typedef struct _parser				PARSER;
typedef struct _generator			GENERATOR;
typedef struct _generator_1d_tab	_1D_TABLE;
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

	CCL			ccl;			/* Character-class definition */

	LIST*		productions;	/* List of productions attached to a
									non-terminal symbol */
	LIST*		first;			/* The symbol's first set */
	
	LIST*		all_sym;		/* List of all possible terminal
									definitions, for multiple-terminals.
									This list will only be set in the
									primary symbol.
								*/

	pregex_nfa	nfa;			/* Regular expression-based terminal */

	BOOLEAN		fixated;		/* Flags, if fixated symbols
									(do always shift!) */
	BOOLEAN		goal;			/* Flags, if goal non-terminal */
	BOOLEAN		nullable;		/* Flags, if nullable */
	BOOLEAN		defined;		/* Flags, if defined */
	BOOLEAN		used;			/* Flags, if used */
	BOOLEAN		lexem;			/* Flags, if distinguished as lexem */
	BOOLEAN		keyword;		/* Flags, if it is a keyword
									(or if it has been derived from a
										keyword) */
	BOOLEAN		whitespace;		/* Flags, if it is a whitespace */
	BOOLEAN		generated;		/* Flags, if automatically generated
									symbol */
	BOOLEAN		greedy;			/* Flags if this is a greedy or nongreedy
									nonterminal */
									
	HASHTAB		options;		/* Options hash table */

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

	SYMBOL*		lhs;			/* Primary left-hand side symbol */
	LIST*		all_lhs;		/* All possible left-hand sides */
	
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
	
	HASHTAB		options;		/* Options hash table */
	
	int			line;			/* Line of definition */
	
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

	pregex_dfa*	dfa;			/* DFA machine for regex recognition
									in this state */
									
	STATE*		derived_from;	/* Previous state */
};

/* Action/Goto table column */
struct _tabcol
{
	SYMBOL*		symbol;			/* Symbol */
	short		action;			/* Action on this symbol */
	int			index;			/* Action-index on this symbol */
	
	ITEM*		derived_from;	/* List of items that caused the
									derivation of this column */
};

/* Value stack type */
struct _vtype
{
	int			id;				/* Value type ID */
	uchar*		int_name;		/* Internal name for verification */
	uchar*		real_def;		/* Definition by user */
};

/* Parser option */
struct _option
{
	int			line;			/* Line of option definition */
	uchar*		opt;			/* Option name */
	uchar*		def;			/* Option content */
};

/* Parser information structure */
struct _parser
{
	HASHTAB		definitions;	/* Symbol hash table for faster
									symbol access */
	LIST*		symbols;		/* Linked list for sequencial
									symbol management */
	LIST*		productions;	/* Linked list of productions */
	LIST*		lalr_states;	/* Linked list of LALR(1) states */
	LIST*		dfa;			/* List containing the DFA for
									regex terminal recognition */
	
	SYMBOL*		goal;			/* Pointer to the goal non-terminal */
	SYMBOL*		end_of_input;	/* End of input symbol */
	SYMBOL*		error;			/* Error token */
	
	LIST*		kw;				/* Keyword recognition machines */
	LIST*		vtypes;			/* Value stack types */

	short		p_mode;		/* Parser model */
	uchar*		p_name;			/* Parser name */
	uchar*		p_desc;			/* Parser description */
	uchar*		p_language;		/* Parser target programming language */
	uchar*		p_copyright;	/* Parser copyright notice */
	uchar*		p_version;		/* Parser version */
	uchar*		p_prefix;		/* Parser symbol prefix */
	uchar*		p_basename;		/* Parser file basename */
	uchar*		p_def_action;	/* Default reduce action */
	uchar*		p_def_action_e;	/* Default reduce action for
									epsilon-productions */

	BOOLEAN		p_lexem_sep;	/* Flag, if lexem separation is switched
									ON or not */
	BOOLEAN		p_cis_strings;	/* Flag, if case-insensitive strings */
	BOOLEAN		p_extern_tokens;/* Flag if parser uses external tokens */
	BOOLEAN		p_reserve_regex;/* Flag, if regex'es are reserved */
	int			p_universe;		/* Maximum of the character universe */

	uchar*		p_header;		/* Header/Prologue program code of the parser */
	uchar*		p_footer;		/* Footer/Epilogue embedded program code
									of the parser */
	uchar*		p_pcb;			/* Parser control block: Individual
									code segment */
	
	VTYPE*		p_def_type;		/* Default value type */
	
	HASHTAB		options;		/* Options parameter hash table */

	uchar*		source;			/* Parser definition source */

	/* Context-free model relevant */
	LIST*		lexer;

	/* Parser runtime switches */
	BOOLEAN		stats;
	BOOLEAN		verbose;
	BOOLEAN		show_states;
	BOOLEAN		show_grammar;
	BOOLEAN		show_productions;
	BOOLEAN		show_symbols;
	BOOLEAN		optimize_states;
	BOOLEAN		all_warnings;
	BOOLEAN		gen_prog;
	BOOLEAN		gen_xml;

	/* Debug and maintainance */
	uchar*		filename;
	int			debug_level;
	
	/* XML-root node for XML-encoded error messages */
	XML_T		err_xml;
};

/* Generator 2D table structure */
struct _generator_2d_tab
{
	uchar*		row_start;
	uchar*		row_end;
	uchar*		col;
	uchar*		col_sep;
	uchar*		row_sep;
};

/* Generator 1D table structur */
struct _generator_1d_tab
{
	uchar*		col;
	uchar*		col_sep;
};

/* Generator template structure */
struct _generator
{
	uchar*		name;						/* Target language name */
	uchar*		driver;						/* Driver source code */
	uchar*		vstack_def_type;			/* Default-type for nonterminals
												if no type is specified */
	uchar*		vstack_term_type;			/* Type for terminals
												(characters) to be pushed on
													the value stack */
	_2D_TABLE	acttab;						/* Action table */
	_2D_TABLE	gotab;						/* Goto table */
	_1D_TABLE	defprod;					/* Default production for
												each state */
	_1D_TABLE	symbols;					/* Symbol information table */
	_1D_TABLE	productions;				/* Production information table */
	_1D_TABLE	dfa_select;					/* DFA machine selection */
	_2D_TABLE	dfa_idx;					/* DFA state index */
	_1D_TABLE	dfa_char;					/* DFA transition characters */
	_1D_TABLE	dfa_trans;					/* DFA transitions */
	_2D_TABLE	dfa_accept;					/* DFA accepting states */

	uchar*		action_start;				/* Action code start */
	uchar*		action_end;					/* Action code end */
	uchar*		action_single;				/* Action vstack access */
	uchar*		action_union;				/* Action union access */
	uchar*		action_lhs_single;			/* Action left-hand side
												single access */
	uchar*		action_lhs_union;			/* Action left-hand side
												union access */
	uchar*		action_set_lhs;				/* Set a left-hand
												side within semantic
												action code */
	uchar*		vstack_single;				/* Single value stack type
												definition */
	uchar*		vstack_union_start;			/* Begin of value stack
												union definition */
	uchar*		vstack_union_end;			/* End of value stack
												union definition */
	uchar*		vstack_union_def;			/* Union inline type
												definition */
	uchar*		vstack_union_att;			/* Union attribute name */
	uchar*		scan_action_start;			/* Scanner action code start */
	uchar*		scan_action_end;			/* Scanner action code end */
	uchar*		scan_action_begin_offset;	/* Begin-offset of scanned token
												in source */
	uchar*		scan_action_end_offset;		/* End-offset of scanned token
												in source */
	uchar*		scan_action_ret_single;		/* Semantic value,
												single access */
	uchar*		scan_action_ret_union;		/* Semantic value,
												union access */
	uchar*		scan_action_set_symbol;		/* Set regex symbol depending
												on action code decision */

	uchar*		code_localization;			/* Code localization template */

	uchar**		for_sequences;				/* Dynamic array of string
												sequences to be replaced/es-
												caped in output strings */
	uchar**		do_sequences;				/* Dynamic array of escape-
												sequences for the particular
												string sequence in the above
												array */
	int			sequences_count;			/* Number of elements in the
												above array */

	XML_T		xml;						/* XML root node */
};

#endif

