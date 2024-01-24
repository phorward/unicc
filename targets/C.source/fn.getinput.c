#if UNICC_UTF8
UNICC_STATIC UNICC_CHAR _get_char( _pcb* pcb )
{
    unsigned char first = UNICC_GETCHAR( pcb );

    if ((first & 0x80) == 0)
    {
        // Single-byte ASCII character
        return first;
    }
    else if ((first & 0xE0) == 0xC0)
    {
        // Two-byte sequence (110xxxxx 10xxxxxx)
        unsigned char second = UNICC_GETCHAR( pcb );
        return ((first & 0x1F) << 6) | (second & 0x3F);
    }
    else if ((first & 0xF0) == 0xE0)
    {
        // Three-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
        unsigned char bytes[2];

        bytes[0] = UNICC_GETCHAR( pcb );
        bytes[1] = UNICC_GETCHAR( pcb );

        return
            ((first & 0x0F) << 12)
            | ((bytes[0] & 0x3F) << 6)
            | (bytes[1] & 0x3F)
        ;
    }
    else if ((first & 0xF8) == 0xF0)
    {
        // Four-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        unsigned char bytes[3];

        bytes[0] = UNICC_GETCHAR( pcb );
        bytes[1] = UNICC_GETCHAR( pcb );
        bytes[2] = UNICC_GETCHAR( pcb );

        return
            ((first & 0x07) << 18)
            | ((bytes[0] & 0x3F) << 12)
            | ((bytes[1] & 0x3F) << 6)
            | (bytes[2] & 0x3F)
        ;
    }

    return -1; // Invalid UTF-8 sequence
}
#else
#define @@prefix_get_char( pcb )  UNICC_GETCHAR( pcb )
#endif

UNICC_STATIC UNICC_CHAR @@prefix_get_input( @@prefix_pcb* pcb, unsigned int offset )
{
#if UNICC_DEBUG	> 2
    fprintf( stderr, "%s: get input: pcb->buf + offset = %p pcb->bufend = %p\n",
                UNICC_PARSER, pcb->buf + offset, pcb->bufend );
#endif

    while( pcb->buf + offset >= pcb->bufend )
    {
#if UNICC_DEBUG	> 2
            fprintf( stderr, "%s: get input: requiring more input\n",
                    UNICC_PARSER );
#endif
        if( !pcb->buf )
        {
            pcb->bufend = pcb->buf = (UNICC_CHAR*)malloc(
                ( UNICC_MALLOCSTEP + 1 ) * sizeof( UNICC_CHAR ) );

            if( !pcb->buf )
            {
                UNICC_OUTOFMEM( pcb );
                return 0;
            }

            *pcb->buf = 0;
        }
        else if( *pcb->buf && !( ( pcb->bufend - pcb->buf ) %
                    UNICC_MALLOCSTEP ) )
        {
            unsigned int 	size	= (unsigned int)( pcb->bufend - pcb->buf );
            UNICC_CHAR*		buf;

            if( !( buf = (UNICC_CHAR*)realloc( pcb->buf,
                        ( size + UNICC_MALLOCSTEP + 1 )
                            * sizeof( UNICC_CHAR ) ) ) )
            {
                UNICC_OUTOFMEM( pcb );
                free( pcb->buf );

                return 0;
            }

            pcb->buf = buf;
            pcb->bufend = pcb->buf + size;
        }

        if( pcb->is_eof || ( *( pcb->bufend ) = @@prefix_get_char( pcb ) )
                                    == pcb->eof )
        {
#if UNICC_DEBUG	> 2
            fprintf( stderr, "%s: get input: can't get more input, "
                        "end-of-file reached\n", UNICC_PARSER );
#endif
            pcb->is_eof = 1;
            return pcb->eof;
        }
#if UNICC_DEBUG	> 2
        fprintf( stderr, "%s: get input: read char >%c< %d\n",
                    UNICC_PARSER, (char)*( pcb->bufend ), *( pcb->bufend ) );
#endif

#if UNICC_DEBUG	> 2
        fprintf( stderr, "%s: get input: reading character >%c< %d\n",
                    UNICC_PARSER, (char)*( pcb->bufend ), *( pcb->bufend ) );
#endif

        *( ++pcb->bufend ) = 0;
    }

#if UNICC_DEBUG	> 2
    {
        UNICC_CHAR*		chptr;

        fprintf( stderr, "%s: get input: offset = %d\n",
                    UNICC_PARSER, offset );
        fprintf( stderr, "%s: get input: buf = >" UNICC_CHAR_FORMAT "<\n",
                    UNICC_PARSER, pcb->buf );
        fprintf( stderr, "%s: get input: returning %d\n",
                    UNICC_PARSER, *( pcb->buf + offset ) );
    }
#endif

    return pcb->buf[ offset ];
}
