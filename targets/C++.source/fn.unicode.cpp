UNICC_SCHAR* @@prefix_parser::get_lexem( void )
{
#if UNICC_WCHAR || !UNICC_UTF8
    this->lexem = this->buf;
#else
    size_t		size;

    size = wcstombs( (char*)NULL, this->buf, 0 );

    free( this->lexem );

    if( !( this->lexem = (UNICC_SCHAR*)malloc(
            ( size + 1 ) * sizeof( UNICC_SCHAR ) ) ) )
    {
        UNICC_OUTOFMEM( this );
        return NULL;
    }

    wcstombs( this->lexem, this->buf, size + 1 );
#endif

#if UNICC_DEBUG	> 2
    fprintf( stderr, "%s: lexem: this->lexem = >" UNICC_SCHAR_FORMAT "<\n",
                        UNICC_PARSER, this->lexem );
#endif
    return this->lexem;
}
