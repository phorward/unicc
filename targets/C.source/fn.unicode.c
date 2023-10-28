UNICC_STATIC UNICC_SCHAR* @@prefix_lexem( @@prefix_pcb* pcb )
{
#if UNICC_WCHAR || !UNICC_UTF8
    pcb->lexem = pcb->buf;
#else
    size_t		size;

    size = wcstombs( (char*)NULL, pcb->buf, 0 );

    free( pcb->lexem );

    if( !( pcb->lexem = (UNICC_SCHAR*)malloc(
            ( size + 1 ) * sizeof( UNICC_SCHAR ) ) ) )
    {
        UNICC_OUTOFMEM( pcb );
        return (UNICC_SCHAR*)NULL;
    }

    wcstombs( pcb->lexem, pcb->buf, size + 1 );
#endif

#if UNICC_DEBUG	> 2
    fprintf( stderr, "%s: lexem: pcb->lexem = >" UNICC_SCHAR_FORMAT "<\n",
                        UNICC_PARSER, pcb->lexem );
#endif
    return pcb->lexem;
}
