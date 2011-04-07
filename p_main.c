/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator 
Copyright (C) 2006-2011 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com/ ++ unicc<<AT>>phorward-software<<DOT>>com

File:	p_main.c (created on 28.01.2007)
Author:	Jan Max Meyer
Usage:	UniCC program entry / main function

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
extern uchar* progname;

uchar* pmod[] = 
{
	"sensitive",
	"insensitive"
};

/*
 * Defines
 */

/*
 * Verbose Macros (Main only)
 */
#define PROGRESS( txt )		if( parser->verbose ) \
							{ \
								fprintf( stderr, "%s...", (txt ) ); \
								first_progress = TRUE; \
							} \
							else \
								first_progress = FALSE;
#define DONE()				p_status( parser, "Done\n", (uchar*)NULL );
#define FAIL()				p_status( parser, "Failed\n", (uchar*)NULL );
#define SUCCESS()			p_status( parser, "Succeeded\n", (uchar*)NULL );
#define SKIPPED( why )		p_status( parser, "Skipped: %s\n", why );

/*
 * Functions
 */

/* -FUNCTION--------------------------------------------------------------------
	Function:		p_status()
	
	Author:			Jan Max Meyer
	
	Usage:			Internal function to print verbose status messages.
					
	Parameters:		PARSER* 		parser			Parser info struct
					uchar*			status			Status info/Format string
					uchar*			reason			One parameter to be inserted
													by the fprintf( status ... )
													have fun!

	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
static void p_status( PARSER* parser, uchar* status, uchar* reason )
{
	if( !parser->verbose )
		return;
		
	if( first_progress )
		fprintf( stderr, status, reason );
	else
		fprintf( stderr, "\n" );
	
	first_progress = FALSE;
}

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
			UNICC_VERSION, __DATE__, __TIME__ );
	fprintf( stream, "Copyright (C) 2006-2011 by "
						"Phorward Software Technologies, Jan Max Meyer\n" );
	fprintf( stream, "http://www.phorward-software.com ++ "
						"contact<at>phorward<dash>software<dot>com\n\n" );
						
	fprintf( stream, "You may use, modify and distribute this software under "
				"the terms and\n" );
	fprintf( stream, "conditions of the Artistic License, version 2.\n"
				"Please see LICENSE for more information.\n" );
	
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
		"\t-all --all-warnings    Show all warnings\n"
		"\t-gr  --grammar         Dump final grammar to stderr\n"
		"\t-h   --help            Print this help\n"
		"\t-V   --version         Print version and copyright\n"
		"\t-no  --no-opt          No state optimization (causes more states)\n"
		"\t-b   --basename <name> Use basename <name> for output\n"
		"\t-pr  --productions     Dump final productions to stderr\n"		
		"\t-s   --stats           Print statistics message\n"
		"\t-st  --states          Dump LALR(1) states to stderr\n"
		"\t-sym --symbols         Dump symbols to stderr\n"
		"\t-v   --verbose         Print progress messages\n"
		"\t-w   --warnings        Print warnings\n"
		"\t-x   --xml             Build XML output (XML code generator)\n",

		progname );
}

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
			
			if( !strcmp( opt, "output" ) || !strcmp( opt, "o" ) 
				|| !strcmp( opt, "basename" ) || !strcmp( opt, "b" ) )
			{
				if( ++i < argc && !( *output ) )
					*output = argv[i];
				else
					p_error( parser, ERR_CMD_LINE, ERRSTYLE_WARNING,
								argv[ i - 1 ] );
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
			else if( !strcmp( opt, "symbols" ) || !strcmp( opt, "sym" ) )
				parser->show_symbols = TRUE;
			else if( !strcmp( opt, "productions" ) || !strcmp( opt, "pr" ) )
				parser->show_productions = TRUE;
			else if( !strcmp( opt, "no-opt" ) || !strcmp( opt, "no" ) )
				parser->optimize_states = FALSE;
			else if( !strcmp( opt, "all-warnings" ) || !strcmp( opt, "all" ) )
			{
				parser->all_warnings = TRUE;
				no_warnings = FALSE;
			}
			else if( !strcmp( opt, "version" ) || !strcmp( opt, "V" ) )
			{
				p_copyright( stderr );
				exit( EXIT_SUCCESS );
			}
			else if( !strcmp( opt, "help" ) || !strcmp( opt, "h" ) )
			{
				p_usage( stderr, *argv );
				exit( EXIT_SUCCESS );
			}
			else if( !strcmp( opt, "xml" ) || !strcmp( opt, "x" ) )
				parser->gen_xml = TRUE;
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
					char**		argv				Argument values
	
	Returns:		int								Number of errors count,
													0 = all right :D
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
int main( int argc, char** argv )
{
	uchar*	filename	= (uchar*)NULL;
	uchar*	base_name	= (uchar*)NULL;
	uchar*	mbase_name	= (uchar*)NULL;
	PARSER*	parser;
	BOOLEAN	recursions	= FALSE;
	BOOLEAN	def_lang	= FALSE;

	parser = p_create_parser();

#ifdef UNICC_BOOTSTRAP
	/* On bootstrap build, print a warning message */
	printf( "*** WARNING: YOU'RE RUNNING A BOOTSTRAP BUILD OF UNICC!\n" );
	printf( "*** Some features may not work as you would expect.\n\n" );
#endif

	if( p_get_command_line( argc, argv, &filename, &base_name, parser ) )
	{
		parser->filename = filename;
		switch( map_file( &parser->source, filename ) )
		{
			case 1:
				p_error( parser, ERR_OPEN_INPUT_FILE,
					ERRSTYLE_FATAL, filename );
				return error_count;
				
			case ERR_OK:
				break;
				
			default:
				break;
		}

		/* Basename */
		if( !base_name )
		{
			parser->p_basename = mbase_name = p_strdup(
												pbasename( filename ) );
			if( ( base_name = strrchr( parser->p_basename, '.' ) ) )
				*base_name = '\0';
		}
		else
			parser->p_basename = base_name;

		if( parser->verbose )
			fprintf( stderr, "UniCC version: %s\n", UNICC_VERSION );

		PROGRESS( "Parsing grammar" )
		/* Parse grammar structure */
		if( p_parse( parser, parser->source ) == 0 )
		{
			DONE()

			if( parser->verbose )
				fprintf( stderr, "Parser construction mode: %s\n",
					pmod[ parser->p_mode ] );


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
				if( parser->p_mode == MODE_SENSITIVE )
					p_rewrite_grammar( parser );

				p_unique_charsets( parser );
				p_symbol_order( parser );
				p_charsets_to_nfa( parser );
				
				if( parser->p_mode == MODE_SENSITIVE )
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
				
				if( parser->show_symbols )
					p_dump_symbols( stderr, parser );

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

				/* Regex anomaly recognition */
				PROGRESS( "Regex-terminal anomaly detection" )
				if( parser->p_mode == MODE_SENSITIVE )
				{
					if( recursions )
					{
						SKIPPED( "Recursions detected" );
					}
					else if( parser->p_reserve_regex )
					{
						SKIPPED( "Tokens are reserved!" );
					}
					else
					{
						p_regex_anomalies( parser );
						DONE()
					}
				}
				else
				{
					SKIPPED( "Not required" );
				}

				/* Lexical analyzer generator */
				PROGRESS( "Constructing lexical analyzer" )

				if( parser->p_mode == MODE_SENSITIVE )
					p_keywords_to_dfa( parser );
				else if( parser->p_mode == MODE_INSENSITIVE )
					p_single_lexer( parser );

				DONE()
				
				/* Default production detection */
				PROGRESS( "Detecting default rules" )
				p_detect_default_productions( parser );
				DONE()

				/* Code generator */
				if( !( parser->p_language ) )
				{
					parser->p_language = p_strdup( UNICC_DEFAULT_LNG );
					def_lang = TRUE;
				}

				if( !parser->gen_xml )
				{
					if( parser->verbose )
						fprintf( stderr, "Code generation target: %s%s\n",
							parser->p_language,
								( def_lang ? " (default)" : "" ) );

					PROGRESS( "Invoking code generator" )
					p_build_code( parser );
					DONE()
				}
				else
				{
					PROGRESS( "Generating parser definition file" )
					p_build_xml( parser, TRUE );
					DONE()
				}
			}
			else
			{
				FAIL();
				p_error( parser, ERR_NO_GOAL_SYMBOL, ERRSTYLE_FATAL );
			}
		
			if( parser->stats )
				fprintf( stderr, "\n%s produced %d states "
							"(%d error%s, %d warning%s)\n",
					filename, list_count( parser->lalr_states ),
						error_count, ( error_count == 1 ) ? "" : "s",
						warning_count, ( warning_count == 1 ) ? "" : "s" );	
		}
		else
		{
			FAIL()
			error_count++;

			if( parser->gen_xml )
				p_build_xml( parser, FALSE );
		}

		p_free_parser( parser );
	}
	else
	{
		FAIL()
		p_usage( stderr, *argv );
		error_count++;
	}

	p_free( mbase_name );

	return error_count;
}

