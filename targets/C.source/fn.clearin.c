UNICC_STATIC void @@prefix_clear_input( @@prefix_pcb* pcb )
{
    int		i;

    if( pcb->buf )
    {
        if( pcb->len )
        {
            /* Update counters for line and column */
            for( i = 0; i < pcb->len; i++ )
            {
                if( (char)pcb->buf[i] == '\n' )
                {
                    pcb->line++;
                    pcb->column = 1;
                }
                else
                    pcb->column++;
            }

#if UNICC_DEBUG > 2
    fprintf( stderr, "%s: clear input: "
        "Clearing %d characters (%d bytes)\n",
            UNICC_PARSER, pcb->len, pcb->len * sizeof( UNICC_CHAR ) );
    fprintf( stderr, "%s: clear input: buf = >" UNICC_CHAR_FORMAT "<\n",
            UNICC_PARSER, pcb->buf, sizeof( UNICC_CHAR ) );
    fprintf( stderr, "%s: clear input: pcb->bufend >" UNICC_CHAR_FORMAT "<\n",
            UNICC_PARSER, pcb->bufend );
#endif

            memmove( pcb->buf, pcb->buf + pcb->len,
                        ( ( pcb->bufend - ( pcb->buf + pcb->len ) ) + 1 + 1 )
                            * sizeof( UNICC_CHAR ) );
            pcb->bufend = pcb->buf + ( pcb->bufend - ( pcb->buf + pcb->len ) );

#if UNICC_DEBUG	> 2
    fprintf( stderr, "%s: clear input: now buf = >" UNICC_CHAR_FORMAT "<\n",
                UNICC_PARSER, pcb->buf, sizeof( UNICC_CHAR ) );
    fprintf( stderr, "%s: clear input: now bufend = >" UNICC_CHAR_FORMAT "<\n",
                UNICC_PARSER, pcb->bufend, sizeof( UNICC_CHAR ) );
#endif
        }
        else
        {
            pcb->bufend = pcb->buf;
            *( pcb->buf ) = 0;
        }
    }

    pcb->len = 0;
    pcb->sym = -1;
#if UNICC_DEBUG	> 2
    fprintf( stderr, "%s: clear input: symbol cleared\n", UNICC_PARSER );
#endif
}
