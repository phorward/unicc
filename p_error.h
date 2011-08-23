/* -HEADER----------------------------------------------------------------------
UniCC LALR(1) Parser Generator 
Copyright (C) 2006-2011 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com/ ++ unicc<<AT>>phorward-software<<DOT>>com

File:	p_error.h
Author:	Jan Max Meyer
Usage:	Error message defines

You may use, modify and distribute this software under the terms and conditions
of the Artistic License, version 2. Please see LICENSE for more information.
----------------------------------------------------------------------------- */

#ifndef P_ERROR_H
#define P_ERROR_H

#define ERRSTYLE_NONE				0
#define ERRSTYLE_FATAL				1
#define ERRSTYLE_WARNING			2
#define ERRSTYLE_FILEINFO			4
#define ERRSTYLE_STATEINFO			8
#define ERRSTYLE_LINEINFO			16
#define ERRSTYLE_PRODUCTION			32
#define ERRSTYLE_SYMBOL				64

#define ERR_MEMORY_ERROR			0
#define ERR_CMD_LINE				1
#define ERR_CMD_OPT					2
#define ERR_PARSE_ERROR				3
#define ERR_PARSE_ERROR_EXPECT		4
#define ERR_MULTIPLE_GOAL_DEF		5
#define ERR_GOAL_ONE_RHS			6
#define ERR_NO_GOAL_SYMBOL			7
#define	ERR_DOUBLE_TERMINAL_DEF		8
#define ERR_UNKNOWN_DIRECTIVE		9
#define ERR_WHITESPACE_TOKEN		10
#define ERR_UNDEFINED_NONTERM		11
#define ERR_UNDEFINED_TERM			12
#define ERR_UNUSED_NONTERM			13
#define ERR_UNUSED_TERM				14
#define ERR_REDUCE_REDUCE			15
#define ERR_SHIFT_REDUCE			16
#define ERR_KEYWORD_ANOMALY			17
#define ERR_UNKNOWN_TARGET_LANG		18
#define ERR_NO_VALUE_TYPE			19
#define ERR_ASSUMING_DEF_EOF		20
#define ERR_OPEN_OUTPUT_FILE		21
#define ERR_OPEN_INPUT_FILE			22
#define ERR_NO_GENERATOR_FILE		23
#define ERR_TAG_NOT_FOUND			24
#define ERR_XML_ERROR				25
#define ERR_XML_INCOMPLETE			26
#define ERR_DUPLICATE_ESCAPE_SEQ	27
#define ERR_CIRCULAR_DEFINITION		28
#define ERR_EMPTY_RECURSION			29
#define ERR_USELESS_RULE			30
#define ERR_NO_EFFECT_IN_MODE		31
#define ERR_NONTERM_WS_NOT_ALLOWED	32
#define ERR_INVALID_CHAR_UNIVERSE	33
#define ERR_CHARCLASS_OVERLAP		34
#define ERR_UNDEFINED_SYMREF		35
#define ERR_UNDEFINED_LHS			36
#define ERR_UNDEFINED_TERMINAL		37
#define ERR_NO_TARGET_LANG_SUPPLY	38
#define ERR_DIRECTIVE_ALREADY_USED	39

#endif
