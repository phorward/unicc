
UNICC_STATIC @@prefix_ast* @@prefix_ast_create( @@prefix_pcb* pcb, char* emit,
                                                    UNICC_SCHAR* token )
{
    @@prefix_ast*	node;

    if( !( node = (@@prefix_ast*)malloc( sizeof( @@prefix_ast ) ) ) )
    {
        UNICC_OUTOFMEM( pcb );
        return node;
    }

    memset( node, 0, sizeof( @@prefix_ast ) );

    node->emit = emit;

    if( token )
    {
        #if !UNICC_WCHAR
        if( !( node->token = strdup( token ) ) )
        {
            UNICC_OUTOFMEM( pcb );
            free( node );
            return (@@prefix_ast*)NULL;
        }
        #else
        if( !( node->token = wcsdup( token ) ) )
        {
            UNICC_OUTOFMEM( pcb );
            free( node );
            return (@@prefix_ast*)NULL;
        }
        #endif
    }

    return node;
}

/* Don't report on unused @@prefix_ast_free or @@prefix_ast_print */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

UNICC_STATIC @@prefix_ast* @@prefix_ast_free( @@prefix_ast* node )
{
    if( !node )
        return (@@prefix_ast*)NULL;

    @@prefix_ast_free( node->child );
    @@prefix_ast_free( node->next );

    if( node->token )
        free( node->token );

    free( node );
    return (@@prefix_ast*)NULL;
}

UNICC_STATIC void @@prefix_ast_print( FILE* stream, @@prefix_ast* node )
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
            fprintf( stream,  " " );

        fprintf( stream, "%s", node->emit );

        if( node->token && strcmp( node->emit, node->token ) != 0 )
            fprintf( stream, " (%s)", node->token );

        fprintf( stream, "\n" );

        rec++;
        @@prefix_ast_print( stream, node->child );
        rec--;

        node = node->next;
    }
}

#pragma GCC diagnostic pop
