UNICC_CHAR @@prefix_parser::get_input( size_t offset )
{
#if UNICC_DEBUG	> 2
    fprintf( stderr, "%s: get input: this->buf + offset = %p this->bufend = %p\n",
                UNICC_PARSER, this->buf + offset, this->bufend );
#endif

    while( this->buf + offset >= this->bufend )
    {
#if UNICC_DEBUG	> 2
            fprintf( stderr, "%s: get input: requiring more input\n",
                    UNICC_PARSER );
#endif
        if( !this->buf )
        {
            this->bufend = this->buf = (UNICC_CHAR*)malloc(
                ( UNICC_MALLOCSTEP + 1 ) * sizeof( UNICC_CHAR ) );

            if( !this->buf )
            {
                UNICC_OUTOFMEM( this );
                return 0;
            }

            *this->buf = 0;
        }
        else if( *this->buf && !( ( this->bufend - this->buf ) %
                    UNICC_MALLOCSTEP ) )
        {
            size_t size	= this->bufend - this->buf;
            UNICC_CHAR*	buf;

            if( !( buf = (UNICC_CHAR*)realloc( this->buf,
                        ( size + UNICC_MALLOCSTEP + 1 )
                            * sizeof( UNICC_CHAR ) ) ) )
            {
                UNICC_OUTOFMEM( this );

                free( this->buf );
                this->buf = NULL;

                return 0;
            }

            this->buf = buf;
            this->bufend = this->buf + size;
        }

        if( this->is_eof || ( *( this->bufend ) = (UNICC_CHAR)UNICC_GETINPUT )
                                    == this->eof )
        {
#if UNICC_DEBUG	> 2
            fprintf( stderr, "%s: get input: can't get more input, "
                        "end-of-file reached\n", UNICC_PARSER );
#endif
            this->is_eof = true;
            return this->eof;
        }
#if UNICC_DEBUG	> 2
        fprintf( stderr, "%s: get input: read char >%c< %d\n",
                    UNICC_PARSER, (char)*( this->bufend ), *( this->bufend ) );
#endif

#if UNICC_DEBUG	> 2
        fprintf( stderr, "%s: get input: reading character >%c< %d\n",
                    UNICC_PARSER, (char)*( this->bufend ), *( this->bufend ) );
#endif

        *( ++this->bufend ) = 0;
    }

#if UNICC_DEBUG	> 2
    {
        UNICC_CHAR*		chptr;

        fprintf( stderr, "%s: get input: offset = %d\n",
                    UNICC_PARSER, offset );
        fprintf( stderr, "%s: get input: buf = >" UNICC_CHAR_FORMAT "<\n",
                    UNICC_PARSER, this->buf );
        fprintf( stderr, "%s: get input: returning %d\n",
                    UNICC_PARSER, *( this->buf + offset ) );
    }
#endif

    return this->buf[ offset ];
}
