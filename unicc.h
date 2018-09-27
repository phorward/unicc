/* -HEADER----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2018 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com ++ unicc<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	unicc.h
Author:	Jan Max Meyer
Usage:	Global declarations, structures and includes
----------------------------------------------------------------------------- */

#ifdef _WIN32
#pragma warning( disable: 4996 )
#endif

/*
 * Includes
 */

/* Including the Phorward Foundation Library */
#include <phorward.h>

/* ezXML Library */
#include "xml.h"

/*
 * Defines
 */

/* Special symbol names */
#define P_WHITESPACE			"&whitespace"
#define P_ERROR_RESYNC			"&error"
#define P_END_OF_FILE			"&eof"
#define P_EMBEDDED				"&embedded_%d"

/* Characters for virtual nonterminal names */
#define P_POSITIVE_CLOSURE		'+'
#define P_KLEENE_CLOSURE		'*'
#define P_OPTIONAL_CLOSURE		'?'

/* Character for rewritten virtual nonterminal */
#define P_REWRITTEN_TOKEN		"\'"
#define P_REWRITTEN_CCL			"#"
#define P_REWRITTEN_KW			"~"

/* Regular expression terminals */
#define P_PREGEX_AUTO_NAME		"regex"

/* Default End-of-Input string */
#define P_DEF_EOF_SYMBOL		"\\0"

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
#define IS_TERMINAL( s )		( ((s)->type) > SYM_NON_TERMINAL )

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

/* Length of a string that is always long enough (sorry, insider!) */
#define ONE_LINE				80

/* Generator insertion wildcards */
#define GEN_WILD_PREFIX			"@@"

/* UniCC version number */
#define UNICC_VER_MAJOR			1
#define UNICC_VER_MINOR			5
#define UNICC_VER_PATCH			0
#define UNICC_VER_EXTSTR		"-develop"

/* Default target language */
#define UNICC_DEFAULT_LNG		"C"

/* File extensions */
#define UNICC_TLT_EXTENSION		".tlt"
#define UNICC_XML_EXTENSION		".xml"

/*
 * Macros
 */
#ifdef OUTOFMEM
#undef OUTOFMEM
#endif

#define OUTOFMEM				fprintf( stderr, \
									"%s, %d: Memory allocation failure; " \
										"UniCC possibly ran out of memory!\n", \
											__FILE__, __LINE__ ), \
								print_error( (PARSER*)NULL, ERR_MEMORY_ERROR,\
									ERRSTYLE_FATAL, __FILE__, __LINE__ ), \
								exit( EXIT_FAILURE )

#define MISS_MSG( txt )			fprintf( stderr, "%s, %d: %s\n", \
										__FILE__, __LINE__, txt )

/*
 * Type definitions
 */
typedef struct _list				LIST;
typedef struct _symbol 				SYMBOL;
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

/* Simple linked list */
struct _list
{
	void*		pptr;
	LIST*		next;
};

#define list_access( ll )		( (ll) ? (ll)->pptr : (void*)NULL )
#define list_next( ll )			( (ll) ? (ll)->next : (LIST*)NULL )

#define LISTFOR( ll, cc )		for( (cc) = (ll); (cc); (cc) = list_next(cc) )

/* Symbol structure */
struct _symbol
{
	int			id;				/* Symbol ID */
	int			type;			/* Symbol type */

	char*		keyname;		/* Key name */
	char*		name;			/* Symbol name */
	char*		emit;

	pccl*		ccl;			/* Character-class definition */

	plist*		productions;	/* List of productions attached to a
									non-terminal symbol */
	plist*		first;			/* The symbol's first set */

	plist*		all_sym;		/* List of all possible terminal
									definitions, for multiple-terminals.
									This list will only be set in the
									primary symbol.
								*/

	pregex_ptn*	ptn;			/* Regular expression pattern */

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

	plist*		options;		/* Options hash table */

	SYMBOL*		derived_from;	/* Pointer to symbol from which the
									current has been derived */

	VTYPE*		vtype;			/* Pointer to value type the symbol
									is associated with */

	int			prec;			/* Level of precedence */
	int			assoc;			/* Associativity */

	int			line;			/* Line of definition */

	char*		code;			/* Code for regex terminals */
	int			code_at;		/* Beginning line of code-segment
									in source file */
};

/* Production structure */
struct _prod
{
	int			id;				/* Production ID */

	SYMBOL*		lhs;			/* Primary left-hand side symbol */
	plist*		all_lhs;		/* List of all possible left-hand sides */

	char*		emit;			/* AST node generation */

	plist*		rhs;			/* Right-hand side symbols */
	plist*		sem_rhs;		/* Semantic right-hand side; This
									may differ from the right hand side
										in case of embedded productions,
											to allow a mixture of the
												outer- and inner-production */

	int			prec;			/* Precedence level */
	int			assoc;			/* Associativity flag */

	plist*		options;		/* Options hash table */

	int			line;			/* Line of definition */

	char*		code;			/* Semantic reduction action template */
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
	plist*		lookahead;		/* Set of lookahead-symbols */
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

	int			derived_from;	/* Previous state */
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
	char*		int_name;		/* Internal name for verification */
	char*		real_def;		/* Definition by user */
};

/* Parser option */
struct _option
{
	int			line;			/* Line of option definition */
	char*		opt;			/* Option name */
	char*		def;			/* Option content */
};

/* Parser information structure */
struct _parser
{
	plist*		symbols;		/* Symbol table */
	plist*		productions;	/* Productions */
	parray*		states;			/* LALR(1) states */
	LIST*		dfa;			/* List containing the DFA for
									regex terminal recognition */

	SYMBOL*		goal;			/* Pointer to the goal non-terminal */
	SYMBOL*		end_of_input;	/* End of input symbol */
	SYMBOL*		error;			/* Error token */

	LIST*		dfas;			/* Lexers */
	LIST*		vtypes;			/* Value stack types */

	short		p_mode;			/* Parser model */
	char*		p_name;			/* Parser name */
	char*		p_desc;			/* Parser description */
	char*		p_template;		/* Parser target template */
	char*		p_copyright;	/* Parser copyright notice */
	char*		p_version;		/* Parser version */
	char*		p_prefix;		/* Parser symbol prefix */
	char*		p_basename;		/* Parser file basename */
	char*		p_def_action;	/* Default reduce action */
	char*		p_def_action_e;	/* Default reduce action for
									epsilon-productions */

	BOOLEAN		p_lexem_sep;	/* Flag, if lexem separation is switched
									ON or not */
	BOOLEAN		p_cis_strings;	/* Flag, if case-insensitive strings */
	BOOLEAN		p_extern_tokens;/* Flag if parser uses external tokens */
	BOOLEAN		p_reserve_regex;/* Flag, if regex'es are reserved */
	int			p_universe;		/* Maximum of the character universe */

	char*		p_header;		/* Header/Prologue program code of the parser */
	char*		p_footer;		/* Footer/Epilogue embedded program code
									of the parser */
	char*		p_pcb;			/* Parser control block: Individual
									code segment */

	VTYPE*		p_def_type;		/* Default value type */

	plist*		options;		/* Options parameter hash table */

	char*		source;			/* Parser definition source */

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
	BOOLEAN		to_stdout;
	char*		target;			/* Target language by command-line */
	int			files_count;

	/* Debug and maintainance */
	char*		filename;
	int			debug_level;

	/* XML-root node for XML-encoded error messages */
	XML_T		err_xml;
};

/* Generator 2D table structure */
struct _generator_2d_tab
{
	char*		row_start;
	char*		row_end;
	char*		col;
	char*		col_sep;
	char*		row_sep;
};

/* Generator 1D table structur */
struct _generator_1d_tab
{
	char*		col;
	char*		col_sep;
};

/* Generator template structure */
struct _generator
{
	char*		name;						/* Target language name */
	char*		prefix;						/* Replacement variable prefix */
	char*		driver;						/* Driver source code */
	char*		vstack_def_type;			/* Default-type for nonterminals
												if no type is specified */
	char*		vstack_term_type;			/* Type for terminals
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

	char*		action_start;				/* Action code start */
	char*		action_end;					/* Action code end */
	char*		action_single;				/* Action vstack access */
	char*		action_union;				/* Action union access */
	char*		action_lhs_single;			/* Action left-hand side
												single access */
	char*		action_lhs_union;			/* Action left-hand side
												union access */
	char*		action_set_lhs;				/* Set a left-hand
												side within semantic
												action code */
	char*		vstack_single;				/* Single value stack type
												definition */
	char*		vstack_union_start;			/* Begin of value stack
												union definition */
	char*		vstack_union_end;			/* End of value stack
												union definition */
	char*		vstack_union_def;			/* Union inline type
												definition */
	char*		vstack_union_att;			/* Union attribute name */
	char*		scan_action_start;			/* Scanner action code start */
	char*		scan_action_end;			/* Scanner action code end */
	char*		scan_action_begin_offset;	/* Begin-offset of scanned token
												in source */
	char*		scan_action_end_offset;		/* End-offset of scanned token
												in source */
	char*		scan_action_ret_single;		/* Semantic value,
												single access */
	char*		scan_action_ret_union;		/* Semantic value,
												union access */
	char*		scan_action_set_symbol;		/* Set regex symbol depending
												on action code decision */

	char*		code_localization;			/* Code localization template */

	char**		for_sequences;				/* Dynamic array of string
												sequences to be replaced/es-
												caped in output strings */
	char**		do_sequences;				/* Dynamic array of escape-
												sequences for the particular
												string sequence in the above
												array */
	int			sequences_count;			/* Number of elements in the
												above array */

	XML_T		xml;						/* XML root node */
};

/* Error styles */

#define ERRSTYLE_NONE				0
#define ERRSTYLE_FATAL				1
#define ERRSTYLE_WARNING			2
#define ERRSTYLE_FILEINFO			4
#define ERRSTYLE_STATEINFO			8
#define ERRSTYLE_LINEINFO			16
#define ERRSTYLE_PRODUCTION			32
#define ERRSTYLE_SYMBOL				64

/* Error codes */

typedef enum
{
	ERR_MEMORY_ERROR,
	ERR_CMD_LINE,
	ERR_CMD_OPT,
	ERR_PARSE_ERROR,
	ERR_PARSE_ERROR_EXPECT,
	ERR_MULTIPLE_GOAL_DEF,
	ERR_GOAL_ONE_RHS,
	ERR_NO_GOAL_SYMBOL,
	ERR_DOUBLE_TERMINAL_DEF,
	ERR_UNKNOWN_DIRECTIVE,
	ERR_WHITESPACE_TOKEN,
	ERR_UNDEFINED_NONTERM,
	ERR_UNDEFINED_TERM,
	ERR_UNUSED_NONTERM,
	ERR_UNUSED_TERM,
	ERR_REDUCE_REDUCE,
	ERR_SHIFT_REDUCE,
	ERR_KEYWORD_ANOMALY,
	ERR_UNKNOWN_TARGET_LANG,
	ERR_NO_VALUE_TYPE,
	ERR_OPEN_OUTPUT_FILE,
	ERR_OPEN_INPUT_FILE,
	ERR_NO_GENERATOR_FILE,
	ERR_TAG_NOT_FOUND,
	ERR_XML_ERROR,
	ERR_XML_INCOMPLETE,
	ERR_DUPLICATE_ESCAPE_SEQ,
	ERR_CIRCULAR_DEFINITION,
	ERR_EMPTY_RECURSION,
	ERR_USELESS_RULE,
	ERR_NO_EFFECT_IN_MODE,
	ERR_NONTERM_WS_NOT_ALLOWED,
	ERR_INVALID_CHAR_UNIVERSE,
	ERR_CHARCLASS_OVERLAP,
	ERR_UNDEFINED_SYMREF,
	ERR_UNDEFINED_LHS,
	ERR_UNDEFINED_TERMINAL,
	ERR_NO_TARGET_TPL_SUPPLY,
	ERR_DIRECTIVE_ALREADY_USED
} ERRORCODE;

#include "proto.h"
