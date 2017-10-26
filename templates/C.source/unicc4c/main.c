/* -MODULE----------------------------------------------------------------------
UniCC4C
Copyright (C) 2010 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<AT>phorward<DASH>software<DOT>com

File:	main.c
Author:	Jan Max Meyer
Usage:	Program entry and parameter parsing.
----------------------------------------------------------------------------- */

/*
 * Includes
 */
#include "unicc4c.h"

/*
 * Global variables
 */
char*		prefix;
XML_T		parser;

/*
 * Defines
 */

/*
 * Functions
 */
 
 /* -FUNCTION--------------------------------------------------------------------
	Function:		copyright()
	
	Author:			Jan Max Meyer
	
	Usage:			Prints a program copyright info message.
					
	Parameters:		void

	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void copyright( void )
{
	fprintf( stderr, "UniCC4C v%s [build %s %s]\n",
			UNICC4C_VERSION, __DATE__, __TIME__ );
	fprintf( stderr, "Copyright (C) 2010 by "
						"Phorward Software Technologies, Jan Max Meyer\n" );
	fprintf( stderr, "http://www.phorward-software.com ++ "
						"contact<at>phorward<dash>software<dot>com\n\n" );
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		usage()
	
	Author:			Jan Max Meyer
	
	Usage:			Prints the program usage message.
					
	Parameters:		uchar*		progname			Name of the executable.
	
	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
void usage( uchar* progname )
{
	fprintf( stderr, "usage: %s [options]\n\n"
		"\t-h   --help            Print this help\n"
		"\t-V   --version         Print version and copyright\n",

		progname );
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		get_command_line()
	
	Author:			Jan Max Meyer
	
	Usage:			Analyzes the command line parameters passed to this program.
					
	Parameters:		int			argc		Argument count from main()
					uchar**		argv		Argument values from main()
	
	Returns:		BOOLEAN					TRUE, if command-line parameters
											are correct, FALSE else.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
BOOLEAN get_command_line( int argc, char** argv )
{
	int		i;
	uchar*	opt;

	for( i = 1; i < argc; i++ )
	{
		if( *(argv[i]) == '-' )
		{
			opt = argv[i] + 1;
			if( *opt == '-' )
				opt++;

			if( !strcmp( opt, "version" ) || !strcmp( opt, "V" ) )
			{
				copyright();
				exit( EXIT_SUCCESS );
			}
			else if( !strcmp( opt, "help" ) || !strcmp( opt, "h" ) )
			{
				usage( *argv );
				exit( EXIT_SUCCESS );
			}
		}
	}

	return TRUE;
}

/* -FUNCTION--------------------------------------------------------------------
	Function:		main()
	
	Author:			Jan Max Meyer
	
	Usage:			Program entry
					
	Parameters:		int		argc				Argument count
					char**	argv				Argument values

	
	Returns:		int							Program return code
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
int main( int argc, char** argv )
{
	uchar*		code;

	PROC( "main" );
	
	if( !get_command_line( argc, argv ) )
		RETURN( EXIT_FAILURE );

	if( !( parser = xml_parse_file( argv[1] ) ) )
		RETURN( EXIT_FAILURE );

	if( !( prefix = xml_attr( parser, "prefix" ) ) )
		prefix = "";

	code = gen();
	printf( "%s\n", code );
	pfree( code );

	RETURN( EXIT_SUCCESS );
}

