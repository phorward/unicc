/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator 
Copyright (C) 2006-2009 by Phorward Software Technologies, Jan Max Meyer
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
uchar* error_txt[128] =
{
	"Memory allocation error (out of memory?) in %s, line %d",
	"Expecting \"%s\" but found \"%s\"",
	"Multiple goal symbol defined; \"%s\" is already the goal symbol",
	"Invalid right-hand side definition for goal symbol",
	"No goal symbol defined",
	"Multiple definition for terminal \"%s\"",
	"Unknown parser configuration directive \"%s\"",
	"Call to whitespace token \"%s\" not allowed",
	"Non-terminal \"%s\" is used but never defined",
	"Terminal-symbol \"%s\" is used but never defined",
	"Non-terminal \"%s\" is defined but never used",
	"Terminal-symbol \"%s\" is defined but never used",
	"Reduce-reduce conflict on lookahead: ",
	"Shift-reduce conflict on lookahead: ",
	"Nonassociativity conflict on lookahead: ",
	"Keyword ambiguity at shift on \'%s\' and reduce on \"%s\"",
	"Unimplemented target language \"%s\" for code generator",
	"Undefined value type for \"%s\" in reduction code of rule %d, \"%.*s\"",
	"No end of input token defined, assuming \"%s\"",
	"Parameter missing behind \"%s\"",
	"Unable to open output file \"%s\"",
	"Generator definition file \"%s\" was not found!",
	"Can't find tag <%s> in %s",
	"XML-format errors %s:\n\t%s",
	"In %s: tag <%s> is incomplete, and requires for %s-attribute",
	"Duplicate escape sequence definition for \"%s\" in %s",
	"Circular definition",
	"Empty recursion",
	"Useless production, no terminals in expansion",
	/* semantic warnings and errors */
	"Directive \"%s\" takes no effect in context-free model",
	"Nonterminal whitespace \"%s\" is not allowed in context-free model",
	"Invalid character universe",
	"Character-class overlap in context-free model with \"%s\"",
};

int					error_protocol	= -1;
int					error_count		= 0;
int					warning_count	= 0;
extern	BOOLEAN		first_progress;
extern	BOOLEAN		no_warnings;
uchar*				progname;

/*
 * Functions
 */

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_error()
	
	Author:			Jan Max Meyer
	
	Usage:			Prints an error message.
					
	Parameters:		int			err_id				Error ID
					int			err_style			Error message behavior
					...								Additional parameters depending
													on error message text and
													error message style.

	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_error( int err_id, int err_style, ... )
{
	va_list		params;
	uchar*		filename 	= (uchar*)NULL;
	int			line 		= 0;
	STATE*		state;
	PROD*		p;
	SYMBOL*		s;
	BOOLEAN		do_print	= TRUE;
	
	if( error_protocol == -1 )
		return;

	va_start( params, err_style );

	if( err_style & ERRSTYLE_WARNING && no_warnings )
		do_print = FALSE;
	
	if( err_style & ERRSTYLE_FILEINFO )
	{
		filename = va_arg( params, uchar* );		
		line = va_arg( params, int );
	}
	else if( err_style & ERRSTYLE_STATEINFO )
		state = va_arg( params, STATE* );
	else if( err_style & ERRSTYLE_PRODUCTION )
		p = va_arg( params, PROD* );
	
	/* NO ELSE IF!! */
	if( err_style & ERRSTYLE_SYMBOL )
		s = va_arg( params, SYMBOL* );

	if( error_protocol == ERR_PROT_CONSOLE )
	{
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
	}
	else if( error_protocol == ERR_PROT_XML )
	{
		if( !( error_count && warning_count ) )
			fprintf( stderr, "<pcc-error-log>" );
		if( err_style & ERRSTYLE_FATAL )
		{
			fprintf( stderr, "\t<error" );
			error_count++;
		}
		else if( err_style & ERRSTYLE_WARNING )
		{
			fprintf( stderr, "\t<warning" );
			warning_count++;
		}

		if( err_style & ERRSTYLE_FILEINFO )
			fprintf( stderr, "file=\"%s\" line=\"%d\" ", filename, line );
		else if( err_style & ERRSTYLE_STATEINFO )
			fprintf( stderr, "state=\"%d\"", state->state_id );

		fprintf( stderr, ">" );
	}

	if( do_print )
	{
		vfprintf( stderr, error_txt[ err_id ], params );

		if( err_style & ERRSTYLE_SYMBOL )
			p_print_symbol( stderr, s );

		if( error_protocol == ERR_PROT_CONSOLE )
			fprintf( stderr, "\n" );

		if( err_style & ERRSTYLE_STATEINFO )
		{
			p_dump_item_set( stderr, (char*)NULL, state->kernel );
			p_dump_item_set( stderr, (char*)NULL, state->epsilon );
			fprintf( stderr, "\n" );
		}
		else if( err_style & ERRSTYLE_PRODUCTION )
		{
			fprintf( stderr, "  " );
			p_dump_production( stderr, p, TRUE, FALSE );
		}
	}

	if( error_protocol == ERR_PROT_XML )
		fprintf( stderr, "</%s>", ( ( err_style & ERRSTYLE_FATAL ) ? "error" : "warning" ) );

	va_end( params );

	if( do_print )
		first_progress = FALSE;
}

