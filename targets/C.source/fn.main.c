@@epilogue

/* Create Main? */
%%%ifgen STDTPL
#if @@epilogue_len == 0
	#ifndef UNICC_MAIN
	#define UNICC_MAIN 	1
	#endif
#else
	#ifndef UNICC_MAIN
	#define UNICC_MAIN 	0
	#endif
#endif
%%%ifgen UNICC4C
#ifndef UNICC_MAIN
%%%code L( &out, "#define UNICC_MAIN %d", pstrlen( xml_txt(
%%%code					xml_child( parser,"epilogue" ) ) ) > 0 ? 1 : 0 );
#endif
%%%end

#if UNICC_MAIN

#if UNICC_SYNTAXTREE
UNICC_STATIC void @@prefix_syntree_print( FILE* stream,
		@@prefix_pcb* pcb, @@prefix_syntree* node )
{
	int 		i;
	static int 	rec;

	if( !node )
		return;

	if( !stream )
		stream = stderr;

	while( node )
	{
		for( i = 0; i < rec; i++ )
			fprintf( stream,  "." );

		fprintf( stream, "%s", node->symbol.symbol->name );

		if( node->token )
			fprintf( stream, ": '%s'", node->token );

		fprintf( stream, "\n" );

		rec++;
		@@prefix_syntree_print( stream, pcb, node->child );
		rec--;

		node = node->next;
	}
}
#endif

int main( int argc, char** argv )
{
#define UNICCMAIN_SILENT		1
#define UNICCMAIN_ENDLESS		2
#define UNICCMAIN_LINEMODE		4
#define UNICCMAIN_SYNTAXTREE	8
#define UNICCMAIN_AUGSYNTAXTREE	16

	char*			opt;
	int				flags	= 0;
	int				i;
	@@prefix_pcb	pcb;

#ifdef LC_ALL
	setlocale( LC_ALL, "" );
#endif

	/* Get command-line options */
	for( i = 1; i < argc; i++ )
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

	/* Parser info */
	if( !( flags & UNICCMAIN_SILENT ) )
	{
%%%ifgen STDTPL
		if( @@name_len > 0 && @@version_len > 0 )
			printf( "@@name v@@version\n" );

		if( @@copyright_len > 0 )
			printf( "@@copyright\n\n" );
%%%ifgen UNICC4C
%%%code{
	if( pstrlen( xml_attr( parser, "name" ) ) &&
			pstrlen( xml_attr( parser, "version" ) ) )
		TL( &out, 2 "printf( \"%s v%s\\n\" );",
			xml_attr( parser, "name" ),
				xml_attr( parser, "version" ) );

	if( pstrlen( xml_attr( parser, "copyright" ) ) )
		TL( &out, 2, "printf( \"%s\\n\\n\" );",
			xml_attr( parser, "copyright" ) );
%%%code}
%%%end
	}

	/* Parser invocation loop */
	do
	{
		if( !( flags & UNICCMAIN_SILENT ) )
			printf( "\nok\n" );

		/* Invoke parser */
		memset( &pcb, 0, sizeof( @@prefix_pcb ) );

		if( flags & UNICCMAIN_LINEMODE )
			pcb.eof = '\n';
		else
			pcb.eof = EOF;

		@@prefix_parse( &pcb );

#if UNICC_SYNTAXTREE
		/* Print syntax tree */
		if( !pcb.error_count )
			@@prefix_syntree_print( stderr, &pcb, pcb.syntax_tree );

		pcb.syntax_tree = @@prefix_syntree_free( pcb.syntax_tree );
#endif
	}
	while( flags & UNICCMAIN_ENDLESS );

	return 0;
}
#endif
