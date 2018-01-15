@@epilogue

/* Create Main? */
#if @@epilogue_len == 0
	#ifndef UNICC_MAIN
	#define UNICC_MAIN 	1
	#endif
#else
	#ifndef UNICC_MAIN
	#define UNICC_MAIN 	0
	#endif
#endif

#if UNICC_MAIN
int main( int argc, char** argv )
{
#define UNICCMAIN_SILENT		1
#define UNICCMAIN_ENDLESS		2
#define UNICCMAIN_LINEMODE		4
#define UNICCMAIN_SYNTAXTREE	8
#define UNICCMAIN_AUGSYNTAXTREE	16

	char*				opt;
	int					flags	= 0;
	@@prefix_parser*	parser = new @@prefix_parser;

#ifdef LC_ALL
	setlocale( LC_ALL, "" );
#endif

	/* Get command-line options */
	for( int i = 1; i < argc; i++ )
	{
		if( *(argv[i]) == '-' )
		{
			opt = argv[i] + 1;

			/* Long option coming? */
			if( *opt == '-' )
			{
				opt++;

				if( !strcmp( opt, "silent" ) )
					flags |= UNICCMAIN_SILENT;
				else if( !strcmp( opt, "endless" ) )
					flags |= UNICCMAIN_ENDLESS;
				else if( !strcmp( opt, "line-mode" ) )
					flags |= UNICCMAIN_LINEMODE;
				else
				{
					fprintf( stderr, "Unknown option '--%s'\n", argv[i] );
					return 1;
				}
			}

			for( ; *opt; opt++ )
			{
				if( *opt == 's' )
					flags |= UNICCMAIN_SILENT;
				else if( *opt == 'e' )
					flags |= UNICCMAIN_ENDLESS;
				else if( *opt == 'l' )
					flags |= UNICCMAIN_LINEMODE;
				else
				{
					fprintf( stderr, "Unknown option '-%c'\n", *opt );
					return 1;
				}
			}
		}
	}

	if( flags & UNICCMAIN_LINEMODE )
		parser->eof = '\n';
	else
		parser->eof = EOF;

	/* Parser invocation loop */
	do
	{
		if( !( flags & UNICCMAIN_SILENT ) )
			printf( "\nok\n" );

		parser->parse();

		/* Print AST */
		if( parser->ast )
		{
			parser->ast_print( stderr, parser->ast );
			parser->ast = parser->ast_free( parser->ast );
		}
	}
	while( flags & UNICCMAIN_ENDLESS );

	return 0;
}
#endif
