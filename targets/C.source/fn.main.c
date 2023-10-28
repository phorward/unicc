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

        /* Print AST */
        if( pcb.ast )
        {
            @@prefix_ast_print( stderr, pcb.ast );
            @@prefix_ast_free( pcb.ast );
        }
    }
    while( flags & UNICCMAIN_ENDLESS );

    return 0;
}
#endif
