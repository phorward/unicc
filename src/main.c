/* UniCC program entry / main function */

#include "unicc.h"

FILE*			status;
BOOLEAN			first_progress		= FALSE;
BOOLEAN 		no_warnings			= TRUE;

extern int		error_count;
extern int		warning_count;
extern char*	progname;

char* 			pmod[] =
{
    "scannerless",
    "using scanner"
};

/* Verbose Macros (Main only) */
#define PROGRESS( txt )		if( parser->verbose ) \
                            { \
                                fprintf( status, "%s...", (txt) ); \
                                fflush( status ); \
                                first_progress = TRUE; \
                            } \
                            else \
                                first_progress = FALSE;
#define DONE()				print_status( parser, "Done\n", (char*)NULL );
#define FAIL()				print_status( parser, "Failed\n", (char*)NULL );
#define SUCCESS()			print_status( parser, "Succeeded\n", (char*)NULL );
#define SKIPPED( why )		print_status( parser, "Skipped: %s\n", why );


/** Internal function to print verbose status messages.

//parser// is the parser info struct.
//status_msg// is the status info, respective format string.
//reason// is one parameter to be inserted.
*/
static void print_status( PARSER* parser, char* status_msg, char* reason )
{
    if( !parser->verbose )
        return;

    if( first_progress )
        fprintf( status, status_msg, reason );
    else
        fprintf( status, "\n" );

    first_progress = FALSE;
}

/** Generates and returns the UniCC version number string.

//long_version// is the If TRUE, prints a long version string with patchlevel
information. */
char* print_version( BOOLEAN long_version )
{
    static char	version [ ONE_LINE + 1 ];

    if( long_version )
        sprintf( version, "%d.%d.%d%s",
                UNICC_VER_MAJOR, UNICC_VER_MINOR, UNICC_VER_PATCH,
                    UNICC_VER_EXTSTR );
    else
        sprintf( version, "%d.%d", UNICC_VER_MAJOR, UNICC_VER_MINOR );

    return version;
}

/** Prints a program copyright info message to a desired file or stream.

//stream// is the stream where the message is printed to.
If this is (FILE*)NULL, stdout will be used. */
void print_copyright( FILE* stream )
{
    if( !stream )
        stream = stdout;

    fprintf( stream, "UniCC %s\n", print_version( TRUE ) );
    fprintf( stream, "The Universal LALR(1) parser generator.\n\n" );

    fprintf( stream, "Copyright (C) 2006-2023 by "
                        "Phorward Software Technologies, Jan Max Meyer\n" );
    fprintf( stream, "All rights reserved. "
                        "See LICENSE for more information.\n" );

}

/** Prints the program usage message.

//stream// is the stream where the message is printed to. If this is
(FILE*)NULL, stdout will be used. //progname// is the name of the executable. */
void print_usage( FILE* stream, char* progname )
{
    if( !stream )
        stream = stdout;

    fprintf( stream, "Usage: %s [OPTION]... FILE\n\n"
        "  -a    --all             Print all warnings\n"
        "  -b/-o --basename NAME   Use basename NAME for output files\n"
        "  -G    --grammar         Dump final (rewritten) grammar\n"
        "  -h    --help            Print this help and exit\n"
        "  -l    --language TARGET Specify target language (default: %s)\n"
        "  -n    --no-opt          Disables state optimization\n"
        "                          (this will cause more states)\n"
        "  -P    --productions     Dump final productions\n"
        "  -s    --stats           Print statistics message\n"
        "  -S    --states          Dump LALR(1) states\n"
        "  -t    --stdout          Print output to stdout instead of files\n"
        "  -T    --symbols         Dump symbols\n"
        "  -v    --verbose         Print progress messages\n"
        "  -V    --version         Print version and copyright and exit\n"
        "  -w    --warnings        Print warnings\n"
        "\n"
        "Errors and warnings are printed to stderr, "
            "everything else to stdout.\n"

        "", progname, UNICC_DEFAULT_TARGET );
}

/** Analyzes the command line parameters passed to the parser generator.

//argc// is the argument count from main().
//argv// is the argument values from main().
//filename// is the return pointer for the name of the parser source file.
//output// is the return pointer for the name of a possible output file.
//parser// is the parser structure.

Returns a TRUE, if command-line parameters are correct, FALSE otherwise. */
BOOLEAN get_command_line( int argc, char** argv, char** filename,
        char** output, PARSER* parser )
{
    int		i;
    int		rc;
    int		next;
    char	opt		[ ONE_LINE + 1 ];
    char*	param;

    progname = *argv;

    for( i = 0;
            ( rc = pgetopt( opt, &param, &next, argc, argv,
                        "ab:Ghl:no:PsStTvVw",
                        "all grammar help language: no-opt output: basename: "
                            "productions stats states stdout symbols verbose "
                                "version warnings", i ) ) == 0; i++ )
    {
        if( !strcmp( opt, "output" ) || !strcmp( opt, "o" )
            || !strcmp( opt, "basename" ) || !strcmp( opt, "b" ) )
        {
            if( !param )
                print_error( parser, ERR_CMD_LINE, ERRSTYLE_FATAL, opt );
            else
                *output = param;
        }
        else if( !strcmp( opt, "language" ) || !strcmp( opt, "l" ) )
        {
            if( !param )
                print_error( parser, ERR_CMD_LINE, ERRSTYLE_FATAL, opt );
            else
                parser->target = param;
        }
        else if( !strcmp( opt, "verbose" ) || !strcmp( opt, "v" ) )
        {
            parser->verbose = TRUE;
            parser->stats = TRUE;
        }
        else if( !strcmp( opt, "stats" ) || !strcmp( opt, "s" ) )
            parser->stats = TRUE;
        else if( !strcmp( opt, "warnings" ) || !strcmp( opt, "w" ) )
            no_warnings = FALSE;
        else if( !strcmp( opt, "grammar" ) || !strcmp( opt, "G" ) )
            parser->show_grammar = TRUE;
        else if( !strcmp( opt, "states" ) || !strcmp( opt, "S" ) )
            parser->show_states = TRUE;
        else if( !strcmp( opt, "symbols" ) || !strcmp( opt, "T" ) )
            parser->show_symbols = TRUE;
        else if( !strcmp( opt, "stdout" ) || !strcmp( opt, "t" ) )
        {
            parser->to_stdout = TRUE;
            status = stderr;
        }
        else if( !strcmp( opt, "productions" ) || !strcmp( opt, "P" ) )
            parser->show_productions = TRUE;
        else if( !strcmp( opt, "no-opt" ) || !strcmp( opt, "n" ) )
            parser->optimize_states = FALSE;
        else if( !strcmp( opt, "all" ) || !strcmp( opt, "a" ) )
        {
            parser->all_warnings = TRUE;
            no_warnings = FALSE;
        }
        else if( !strcmp( opt, "version" ) || !strcmp( opt, "V" ) )
        {
            print_copyright( stdout );
            exit( EXIT_SUCCESS );
        }
        else if( !strcmp( opt, "help" ) || !strcmp( opt, "h" ) )
        {
            print_usage( stdout, *argv );
            exit( EXIT_SUCCESS );
        }
    }

    if( rc == 1 )
        *filename = param;
    else if( rc < 0 && param )
        print_error( parser, ERR_CMD_OPT, ERRSTYLE_FATAL, param );

    return ( *filename ? TRUE : FALSE );
}

/** Global program entry.

//argc// is the argument count.
//argv// is the argument values.

Returns the number of errors count, 0 = all right :D
*/
int main( int argc, char** argv )
{
    char*	filename	= (char*)NULL;
    char*	base_name	= (char*)NULL;
    char*	mbase_name	= (char*)NULL;
    PARSER*	parser;
    BOOLEAN	recursions	= FALSE;

    status = stdout;
    parser = create_parser();

#ifdef UNICC_BOOTSTRAP
    /* On bootstrap build, print a warning message */
    printf( "*** WARNING: YOU'RE RUNNING A BOOTSTRAP BUILD OF UNICC!\n" );
    printf( "*** Some features may not work as you would expect them.\n\n" );
#endif

    if( get_command_line( argc, argv, &filename, &base_name, parser ) )
    {
        if( !pfiletostr( &parser->source, ( parser->filename = filename ) ) )
        {
            print_error( parser, ERR_OPEN_INPUT_FILE,
                            ERRSTYLE_FATAL, filename );
            free_parser( parser );

            return error_count;
        }

        /* Basename */
        if( !base_name )
        {
            parser->p_basename = mbase_name = pstrdup( pbasename( filename ) );
            if( ( base_name = strrchr( parser->p_basename, '.' ) ) )
                *base_name = '\0';
        }
        else
            parser->p_basename = base_name;

        if( parser->verbose )
            fprintf( status, "UniCC v%s\n", print_version( FALSE ) );

        PROGRESS( "Parsing grammar" )

        /* Parse grammar structure */
        if( parse_grammar( parser, parser->filename, parser->source ) == 0 )
        {
            DONE()

            if( parser->verbose )
                fprintf( status, "Parser construction mode: %s\n",
                    pmod[ parser->p_mode ] );


            PROGRESS( "Goal symbol detection" )
            if( parser->goal )
            {
                SUCCESS()

                /* Single goal revision, if necessary */
                PROGRESS( "Setting up single goal symbol" )
                setup_single_goal( parser );
                DONE()

                /* Rewrite the grammar, if required */
                PROGRESS( "Rewriting grammar" )
                if( parser->p_mode == MODE_SCANNERLESS )
                    rewrite_grammar( parser );

                unique_charsets( parser );
                symbol_orders( parser );
                charsets_to_ptn( parser );

                if( parser->p_mode == MODE_SCANNERLESS )
                    inherit_fixiations( parser );
                DONE()

                /* Precedence fixup */
                PROGRESS( "Fixing precedences" )
                fix_precedences( parser );
                DONE()

                /* FIRST-set computation */
                PROGRESS( "Computing FIRST-sets" )
                compute_first( parser );
                DONE()

                if( parser->show_grammar )
                    dump_grammar( status, parser );

                if( parser->show_symbols )
                    dump_symbols( status, parser );

                if( parser->show_productions )
                    dump_productions( status, parser );

                /* Stupid production recognition */
                PROGRESS( "Validating rule integrity" )

                if( !find_undef_or_unused( parser ) )
                {
                    if( check_stupid_productions( parser ) )
                        recursions = TRUE;

                    DONE()

                    /* Parse table generator */
                    PROGRESS( "Building parse tables" )
                    generate_tables( parser );

                    if( parser->show_states )
                        dump_lalr_states( status, parser );

                    DONE()

                    /* Terminal anomaly detection */
                    PROGRESS( "Terminal anomaly detection" )
                    if( parser->p_mode == MODE_SCANNERLESS )
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
                            check_regex_anomalies( parser );
                            DONE()
                        }
                    }
                    else
                    {
                        SKIPPED( "Not required" );
                    }

                    /* Lexical analyzer generator */
                    PROGRESS( "Constructing lexical analyzer" )

                    if( parser->p_mode == MODE_SCANNERLESS )
                        merge_symbols_to_dfa( parser );
                    else if( parser->p_mode == MODE_SCANNER )
                        construct_single_lexer( parser );

                    DONE()

                    /* Default production detection */
                    PROGRESS( "Detecting default rules" )
                    detect_default_productions( parser );
                    DONE()

                    /* Code generator */
                    if( !( parser->p_template ) )
                        parser->p_template = parser->target;

                    if( parser->gen_prog )
                    {
                        if( parser->verbose )
                            fprintf( status, "Code generation target: %s%s\n",
                                parser->p_template,
                                    ( parser->p_template == parser->target
                                        && strcmp( parser->target,
                                            UNICC_DEFAULT_TARGET ) == 0 ?
                                            " (default)" : "" ) );

                        PROGRESS( "Invoking code generator" )
                        build_code( parser );
                        DONE()
                    }
                }
                else
                {
                    FAIL()
                }
            }
            else
            {
                FAIL()
                print_error( parser, ERR_NO_GOAL_SYMBOL, ERRSTYLE_FATAL );
            }

            if( parser->stats )
                fprintf( status, "%s%s produced %ld states "
                            "(%d error%s, %d warning%s), %d file%s\n",
                    ( parser->verbose ? "\n" : "" ),
                    filename, parray_count( parser->states ),
                        error_count, ( error_count == 1 ) ? "" : "s",
                        warning_count, ( warning_count == 1 ) ? "" : "s",
                        parser->files_count,
                            ( parser->files_count == 1 ) ? "" : "s" );
        }
        else
        {
            FAIL()
            error_count++;
        }

        free_parser( parser );
    }
    else if( !error_count )
    {
        print_usage( status, *argv );
        error_count++;
    }

    pfree( mbase_name );

    return error_count;
}
