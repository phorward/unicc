/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator 
Copyright (C) 2006-2009 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com/ ++ unicc<<AT>>phorward-software<<DOT>>com

File:	p_main.c (created on 28.01.2007)
Author:	Jan Max Meyer
Usage:	Phorward program entry / main function

You may use, modify and distribute this software under the terms and conditions
of the Artistic License, version 2. Please see LICENSE for more information.
----------------------------------------------------------------------------- */

/*
 * Includes
 */
#include "p_global.h"
#include "p_error.h"
#include "p_proto.h"

/*
 * Global variables
 */
BOOLEAN	first_progress		= FALSE;
BOOLEAN no_warnings			= TRUE;
extern int error_count;
extern int warning_count;
extern int error_protocol;
extern uchar* progname;

uchar* pmod[] = 
{
	"context-sensitive",
	"context-free"
};

/*
 * Defines
 */

/*
 * Macros (Main only)
 */
#define PROGRESS( txt )		if( parser->verbose ) { fprintf( stderr, "%s...", (txt ) ), first_progress = TRUE; } else first_progress = FALSE ;
#define DONE()				if( parser->verbose ) { if( first_progress ) fprintf( stderr, "Done\n" ); }\
							first_progress = FALSE;
#define FAIL()				if( parser->verbose ) { if( first_progress ) fprintf( stderr, "Failed\n" ); else fprintf( stderr, "\n" ); }\
							first_progress = FALSE;
#define SUCCESS()			if( parser->verbose ) { if( first_progress ) fprintf( stderr, "Succeeded\n" ); else fprintf( stderr, "\n" ); }\
							first_progress = FALSE;
#define SKIPPED( why )		if( parser->verbose ) { if( first_progress ) fprintf( stderr, "Skipped: %s\n", why ); else fprintf( stderr, "\n" ); }\
							first_progress = FALSE;

/*
 * Functions
 */

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_copyright()
	
	Author:			Jan Max Meyer
	
	Usage:			Prints a program copyright info message to a desired file
					or stream.
					
	Parameters:		FILE*		stream				Stream where the message is
													printed to. If this is
													(FILE*)NULL, stdout will be
													used.
	
	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_copyright( FILE* stream )
{
	if( !stream )
		stream = stdout;

	fprintf( stream, "UniCC LALR(1) Parser Generator v%s [build %s %s]\n",
			PHORWARD_VERSION, __DATE__, __TIME__ );
	fprintf( stream, "Copyright (C) 2006-2009 by Phorward Software, Jan Max Meyer\n" );
	fprintf( stream, "http://www.phorward-software.com ++ contact@phorward-software.com\n" );

	fprintf( stream, "!!!THIS IS A DEVELOPMENT VERSION: NOT FOR PUBLIC RELEASE!!!\n" );
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_usage()
	
	Author:			Jan Max Meyer
	
	Usage:			Prints the program usage message.
					
	Parameters:		FILE*		stream				Stream where the message is
													printed to. If this is
													(FILE*)NULL, stdout will be
													used.
					uchar*		progname			Name of the executable.
	
	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void p_usage( FILE* stream, uchar* progname )
{
	if( !stream )
		stream = stdout;

	fprintf( stream, "usage: %s [options] filename\n\n"
		"\t-all --all-warnings  Show all warnings\n"
		"\t-gr  --grammar       Dump final grammar to stderr\n"
		"\t-h   --help          Print this help\n"
		"\t-i   --version       Print version and copyright\n"
		"\t-no  --no-opt        No state optimization (causes more states)\n"
		"\t-o   --output <file> Write parser to <file>\n"				
		"\t-pr  --productions   Dump final productions to stderr\n"		
		"\t-s   --stats         Print statistics message\n"
		"\t-st  --states        Dump LALR(1) states to stderr\n"
		"\t-v   --verbose       Print progress messages\n"
		"\t-w   --warnings      Print warnings\n",

		progname );
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_unicc_home()
	
	Author:			Jan Max Meyer
	
	Usage:			Extracts the unicc home directory from the unicc program
					filename.
					
	Parameters:		???
	
	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
/* TODO */

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_get_command_line()
	
	Author:			Jan Max Meyer
	
	Usage:			Analyzes the command line parameters passed to the
					parser generator.
					
	Parameters:		int			argc		Argument count from main()
					uchar**		argv		Argument values from main()
					uchar**		filename	Name of the parser source file
					uchar**		output		Name of a possible output file
					PARSER*		parser		Parser structure
	
	Returns:		BOOLEAN					TRUE, if command-line parameters
											are correct, FALSE else.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
BOOLEAN p_get_command_line( int argc, char** argv, char** filename,
		char** output, PARSER* parser )
{
	int		i;
	uchar*	opt;
	
	progname = *argv;

	for( i = 1; i < argc; i++ )
	{
		if( *(argv[i]) == '-' )
		{
			opt = argv[i] + 1;
			if( *opt == '-' )
				opt++;
			
			if( !strcmp( opt, "output" ) || !strcmp( opt, "o" ) )
			{
				if( ++i < argc && !( *output ) )
					*output = argv[i];
				else
					p_error( ERR_CMD_LINE, ERRSTYLE_WARNING, argv[i-1] );
			}
			else if( !strcmp( opt, "verbose" ) || !strcmp( opt, "v" ) )
				parser->verbose = TRUE;
			else if( !strcmp( opt, "stats" ) || !strcmp( opt, "s" ) )
				parser->stats = TRUE;
			else if( !strcmp( opt, "warnings" ) || !strcmp( opt, "w" ) )
				no_warnings = FALSE;
			else if( !strcmp( opt, "grammar" ) || !strcmp( opt, "gr" ) )
				parser->show_grammar = TRUE;
			else if( !strcmp( opt, "states" ) || !strcmp( opt, "st" ) )
				parser->show_states = TRUE;
			else if( !strcmp( opt, "productions" ) || !strcmp( opt, "pr" ) )
				parser->show_productions = TRUE;
			else if( !strcmp( opt, "no-opt" ) || !strcmp( opt, "no" ) )
				parser->optimize_states = FALSE;
			else if( !strcmp( opt, "all-warnings" ) || !strcmp( opt, "all" ) )
				parser->all_warnings = TRUE;
			else if( !strcmp( opt, "version" ) || !strcmp( opt, "i" ) )
			{
				p_copyright( stderr );
				exit( EXIT_SUCCESS );
			}
			else if( !strcmp( opt, "help" ) || !strcmp( opt, "h" ) )
			{
				p_usage( stderr, *argv );
				exit( EXIT_SUCCESS );
			}
		}
		else if( !( *filename ) )
			*filename = argv[i];
	}

	return ( *filename ? TRUE : FALSE );
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		main()
	
	Author:			Jan Max Meyer
	
	Usage:			Global program entry
					
	Parameters:		int			argc				Argument count
					char**		argv				Argument values array
	
	Returns:		int								Error code (0 = no error)
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
int main( int argc, char** argv )
{
	uchar*	file		= (uchar*)NULL;
	uchar*	filename	= (uchar*)NULL;
	uchar*	output		= (uchar*)NULL;
	uchar*	tmp			= (uchar*)NULL;
	FILE*	out			= (FILE*)NULL;
	SYMBOL*	goal		= (SYMBOL*)NULL;
	PARSER*	parser;
	BOOLEAN	recursions	= FALSE;
	BOOLEAN	def_lang	= FALSE;

	error_protocol = ERR_PROT_CONSOLE;
	parser = p_create_parser();

	if( p_get_command_line( argc, argv, &filename, &output, parser ) )
	{
		parser->filename = filename;
		parser->source = p_mapfile( filename );

		/* Hack for the parser for comments on the end of line ;) */
		if( parser->source )
			parser->source = p_str_append( parser->source, "\n", FALSE );

		if( output && !( out = fopen( output, "wt" ) ) )
		{
			p_error( ERR_OPEN_OUTPUT_FILE, ERRSTYLE_FATAL, output );
			return error_count;
		}

		if( !out )
			out = stdout;

		PROGRESS( "Parsing source grammar" )
		/* Parse grammar structure */
		if( p_parse( parser, parser->source ) == 0 )
		{
			DONE()

			if( parser->verbose )
				fprintf( stderr, "Detected parser model: %s\n",
					pmod[ parser->p_model ] );

			PROGRESS( "Goal symbol detection" )
			if( parser->goal )
			{
				SUCCESS()

				/* Single goal revision, if necessary */
				PROGRESS( "Setting up single goal symbol" )
				p_setup_single_goal( parser );
				DONE()
					
				/* Rewrite the grammar, if required */
				PROGRESS( "Rewriting grammar" )
				if( parser->p_model == MODEL_CONTEXT_SENSITIVE )
					p_rewrite_grammar( parser );

				p_unique_charsets( parser );
				
				if( parser->p_model == MODEL_CONTEXT_SENSITIVE )
					p_inherit_fixiations( parser );
				DONE()

				/* Precedence fixup */
				PROGRESS( "Fixing precedences" )
				p_fix_precedences( parser );
				DONE()
				
				/* FIRST-set computation */
				PROGRESS( "Computing FIRST-sets" )
				p_first( parser->symbols );
				DONE()

				if( parser->show_grammar )
					p_dump_grammar( stderr, parser );

				if( parser->show_productions )
					p_dump_productions( stderr, parser );

				/* Stupid production recognition */
				PROGRESS( "Validating rule integrity" )

				p_undef_or_unused( parser );
					
				if( p_stupid_productions( parser ) )
					recursions = TRUE;

				DONE()

				/* Parse table generator */
				PROGRESS( "Building parse tables" )
				p_generate_tables( parser );

				if( parser->show_states )
					p_dump_lalr_states( stderr, parser );

				DONE()

				/* Keyword anomaly recognition */
				PROGRESS( "Performing keyword ambiguity detection" )
				if( !recursions )
				{
					p_keyword_anomalies( parser );
					DONE()
				}
				else
				{
					SKIPPED( "Recursions detected" );
				}

				/* Lexical analyzer generator */
				PROGRESS( "Constructing lexical analyzer" )

				if( parser->p_model == MODEL_CONTEXT_SENSITIVE )
					p_keywords_to_dfa( parser );
				else if( parser->p_model == MODEL_CONTEXT_FREE )
					p_single_lexer( parser );

				DONE()
				
				/* Default production detection */
				PROGRESS( "Detecting default actions" )
				p_detect_default_productions( parser );
				DONE()

				/* Code generator */
				if( !( parser->p_language ) )
				{
					parser->p_language = p_strdup( PHORWARD_DEFAULT_LNG );
					def_lang = TRUE;
				}

				if( parser->verbose )
					fprintf( stderr, "Code generation target: %s%s\n", parser->p_language,
					( def_lang ? " (default)" : "" ) );


				PROGRESS( "Invoking code generator" )
				p_build_code( out, parser );
				DONE()

				if( output )
					fclose( out );
			}
			else
			{
				FAIL();
				p_error( ERR_NO_GOAL_SYMBOL, ERRSTYLE_FATAL );
			}
		}
		else
		{
			if( parser->verbose )
				fprintf( stderr, "\nFundamental errors in definition, unable to continue.\n" );
		}

		if( parser->stats )
			fprintf( stderr, "\n%s produced %d states (%d error%s, %d warning%s)\n",
				filename, list_count( parser->lalr_states ),
					error_count, ( error_count == 1 ) ? "" : "s",
						warning_count, ( warning_count == 1 ) ? "" : "s" );

		p_free_parser( parser );
	}
	else
	{
		FAIL()
		p_usage( stderr, *argv );
	}

	return error_count;
}

