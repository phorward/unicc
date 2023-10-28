void @@prefix_parser::clear_input( void )
{
    if( this->buf )
    {
        if( this->len )
        {
            /* Update counters for line and column */
            for( int i = 0; i < this->len; i++ )
            {
                if( (char)this->buf[i] == '\n' )
                {
                    this->line++;
                    this->column = 1;
                }
                else
                    this->column++;
            }

#if UNICC_DEBUG > 2
    fprintf( stderr, "%s: clear input: "
        "Clearing %d characters (%d bytes)\n",
            UNICC_PARSER, this->len, this->len * sizeof( UNICC_CHAR ) );
    fprintf( stderr, "%s: clear input: buf = >" UNICC_CHAR_FORMAT "<\n",
            UNICC_PARSER, this->buf, sizeof( UNICC_CHAR ) );
    fprintf( stderr, "%s: clear input: this->bufend >" UNICC_CHAR_FORMAT "<\n",
            UNICC_PARSER, this->bufend );
#endif

            memmove( this->buf, this->buf + this->len,
                        ( ( this->bufend - ( this->buf + this->len ) ) + 1 + 1 )
                            * sizeof( UNICC_CHAR ) );
            this->bufend = this->buf + ( this->bufend - ( this->buf + this->len ) );

#if UNICC_DEBUG	> 2
    fprintf( stderr, "%s: clear input: now buf = >" UNICC_CHAR_FORMAT "<\n",
                UNICC_PARSER, this->buf, sizeof( UNICC_CHAR ) );
    fprintf( stderr, "%s: clear input: now bufend = >" UNICC_CHAR_FORMAT "<\n",
                UNICC_PARSER, this->bufend, sizeof( UNICC_CHAR ) );
#endif
        }
        else
        {
            this->bufend = this->buf;
            *( this->buf ) = 0;
        }
    }

    this->len = 0;
    this->sym = -1;
#if UNICC_DEBUG	> 2
    fprintf( stderr, "%s: clear input: symbol cleared\n", UNICC_PARSER );
#endif
}
