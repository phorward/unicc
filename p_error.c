/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2015 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com/ ++ unicc<<AT>>phorward-software<<DOT>>com

File:	p_error.c
Author:	Jan Max Meyer
Usage:	Error message handling

You may use, modify and distribute this software under the terms and conditions
of the Artistic License, version 2. Please see LICENSE for more information.
----------------------------------------------------------------------------- */

/*
 * Includes
 */
#include "p_global.h"
#include "p_proto.h"
#include "p_error.h"

/*
 * Global variables
 */
char* error_txt[128] =
{
	"Memory allocation error (out of memory?) in %s, line %d",
	"Parameter required behind command-line option \'%s\'",
	"Unknown command-line option \'%s\'",
	"Parse error: Invalid input \'%s\'",
	"Parse error: Found \'%s\', but expecting %s",
	"Multiple goal symbols defined; \'%s\' already defined as goal symbol",
	"Invalid right-hand side definition for goal symbol",
	"No goal symbol defined",
	"Multiple definition for terminal \'%s\'",
	"Unknown parser configuration directive \'%s\'",
	"Call to whitespace token \'%s\' not allowed",
	"Non-terminal \'%s\' is used but never defined",
	"Terminal-symbol \'%s\' is used but never defined",
	"Non-terminal \'%s\' is defined but never used",
	"Terminal-symbol \'%s\' is defined but never used",
	"Reduce-reduce conflict on lookahead: ",
	"Shift-reduce conflict on lookahead: ",
	/* "Nonassociativity conflict on lookahead: ", */
	"Terminal anomaly at shift on \'%s\' and reduce on \'%s\'",
	"Unimplemented template \'%s\' for code generator",
	"Undefined value type for \'%s\' in reduction code of rule %d, \'%.*s\'",
	"Unable to open output file \'%s\'",
	"Unable to open input file \'%s\'",
	"Can't find generator definition file \'%s\'",
	"Can't find tag <%s> in %s",
	"XML parse errors %s:\n\t%s",
	"In %s: tag <%s> is incomplete, and requires for %s-attribute",
	"Duplicate escape sequence definition for \'%s\' in %s",
	"Circular definition",
	"Empty recursion",
	"Useless production, no terminals in expansion",
	/* semantic warnings and errors */
	"Use of effectless directive \'%s\' in insensitive mode ignored",
	"Nonterminal whitespace \'%s\' is not allowed in insensitive mode",
	"Invalid value for character universe",
	"Character-class overlap in insensitive mode with \'%s\'",
	"Action references to undefined right-hand side symbol '%.*s'",
	"Left-hand side '%s' not known",
	"Terminal '%s' not known",
	"Semantic code will be ignored: No target specified.",
	"Multiple use of directive '#%s' ignored; It has already been defined."
};

int					error_count		= 0;
int					warning_count	= 0;
extern	BOOLEAN		first_progress;
extern	BOOLEAN		no_warnings;
char*				progname;

/*
 * Functions
 */

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_error()

	Author:			Jan Max Meyer

	Usage:			Prints an error message.

	Parameters:		PARSER*		parser			Parser information structure
					int			err_id			Error ID
					int			err_style		Error message behavior
					...							Additional parameters depending
												on error message text and
												error message style.

	Returns:		void

	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_error( PARSER* parser, int err_id, int err_style, ... )
{
	va_list		params;
	char*		filename 	= (char*)NULL;
	int			line 		= 0;
	STATE*		state		= (STATE*)NULL;
	PROD*		p			= (PROD*)NULL;
	SYMBOL*		s			= (SYMBOL*)NULL;
	XML_T		messages;
	XML_T		errmsg		= (XML_T)NULL;

	BOOLEAN		do_print	= TRUE;

	char*		tmp;

	va_start( params, err_style );

	if( err_style & ERRSTYLE_WARNING && no_warnings )
		do_print = FALSE;

	if( parser->gen_xml )
	{
		/*
			This XML library is a cramp...
			it's not possible to move a root-tag,
			so it's required to create a dummy-root
			that is never used.
		*/
		if( !parser->err_xml )
		{
			parser->err_xml = xml_new( "dummy" );
			messages = xml_add_child( parser->err_xml, "messages", 0 );
		}
		else
			messages = xml_child( parser->err_xml, "messages" );

		if( err_style & ERRSTYLE_FATAL )
			errmsg = xml_add_child( messages, "error", 0 );
		else
			errmsg = xml_add_child( messages, "warning", 0 );

		xml_set_int_attr( errmsg, "errorcode", err_id );
	}

	if( err_style & ERRSTYLE_FILEINFO )
	{
		filename = va_arg( params, char* );
		line = va_arg( params, int );

		if( errmsg )
		{
			xml_set_attr_d( errmsg, "filename", filename );
			xml_set_int_attr( errmsg, "line", line );
		}
	}

	if( err_style & ERRSTYLE_STATEINFO )
	{
		state = va_arg( params, STATE* );

		if( errmsg )
			xml_set_int_attr( errmsg, "state", state->state_id );
	}
	else if( err_style & ERRSTYLE_PRODUCTION )
	{
		p = va_arg( params, PROD* );

		if( errmsg )
			xml_set_int_attr( errmsg, "production", p->id );
	}

	/* NO ELSE IF!! */
	if( err_style & ERRSTYLE_SYMBOL )
	{
		s = va_arg( params, SYMBOL* );

		if( errmsg )
			xml_set_int_attr( errmsg, "symbol", s->id );
	}

	if( do_print )
	{
		if( first_progress )
			fprintf( stderr, "\n" );
	}

	if( err_style & ERRSTYLE_FATAL )
	{
		fprintf( stderr, "%s: error: ", progname );
		error_count++;
	}
	else if( err_style & ERRSTYLE_WARNING )
	{
		if( do_print )
			fprintf( stderr, "%s: warning: ", progname );

		warning_count++;
	}

	if( do_print )
	{
		if( err_style & ERRSTYLE_FILEINFO )
			fprintf( stderr, "%s(%d):\n    ", filename, line );
		else if( err_style & ERRSTYLE_STATEINFO )
			fprintf( stderr, "state %d: ", state->state_id );
	}

	if( do_print )
	{
		vfprintf( stderr, error_txt[ err_id ], params );

		if( err_style & ERRSTYLE_SYMBOL )
			p_print_symbol( stderr, s );

		fprintf( stderr, "\n" );

		if( err_style & ERRSTYLE_STATEINFO )
		{
			p_dump_item_set( stderr, (char*)NULL, state->kernel );
			p_dump_item_set( stderr, (char*)NULL, state->epsilon );
		}
		else if( err_style & ERRSTYLE_PRODUCTION )
		{
			fprintf( stderr, "  " );
			p_dump_production( stderr, p, TRUE, FALSE );
		}
	}

	/* Halt on memory error! */
	if( err_id == ERR_MEMORY_ERROR )
	{
		va_end( params );
		exit( 1 );
	}

	if( errmsg )
	{
		/* Unfortunatelly, this must be done this ugly way... */
		pvasprintf( &tmp, error_txt[ err_id ], params );
		xml_set_txt_d( errmsg, tmp );
		pfree( tmp );
	}

	va_end( params );

	if( do_print )
		first_progress = FALSE;

}

