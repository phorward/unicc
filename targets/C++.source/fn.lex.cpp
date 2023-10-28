#if @@number-of-dfa-machines
void @@prefix_parser::lex( void )
{
    int 		state	= 0;
    size_t		len		= 0;
    int			chr;
    UNICC_CHAR	next;
#if !@@mode
    int			machine	= this->dfa_select[ this->tos->state ];
#else
    int			machine	= 0;
#endif

    next = this->get_input( len );
#if UNICC_DEBUG	> 1
fprintf( stderr, "%s: lex: next = %d\n", UNICC_PARSER, next );
#endif

    if( next == this->eof )
    {
        this->sym = @@eof;
        return;
    }

    do
    {
#if UNICC_DEBUG	> 1
fprintf( stderr, "%s: lex: next = %d\n", UNICC_PARSER, next );
#endif

        chr = this->dfa_idx[ machine ][ state ];
#if UNICC_DEBUG	> 1
fprintf( stderr, "%s: lex: chr = %d\n", UNICC_PARSER, chr );
#endif

        state = -1;
#if UNICC_DEBUG	> 1
fprintf( stderr, "%s: lex: FIRST next = %d this->dfa_chars[ chr ] = %d, "
            "this->dfa_chars[ chr+1 ] = %d\n", UNICC_PARSER, next,
                this->dfa_chars[ chr ], this->dfa_chars[ chr + 1 ] );
#endif
        while( this->dfa_chars[ chr ] > -1 )
        {
#if UNICC_DEBUG	> 1
fprintf( stderr, "%s: lex: next = %d this->dfa_chars[ chr ] = %d, "
        "this->dfa_chars[ chr+1 ] = %d\n", UNICC_PARSER, next,
            this->dfa_chars[ chr ], this->dfa_chars[ chr + 1 ] );
#endif
            if( next >= this->dfa_chars[ chr ] &&
                next <= this->dfa_chars[ chr+1 ] )
            {
                state = *( this->dfa_trans + ( chr / 2 ) );
#if UNICC_DEBUG	> 1
fprintf( stderr, "%s: lex: state = %d\n", UNICC_PARSER, state );
#endif
                if( this->dfa_accept[ machine ][ state ] > 0 )
                {
                    this->len = len + 1;
                    this->sym = this->dfa_accept[ machine ][ state ] - 1;

#if UNICC_DEBUG	> 1
fprintf( stderr, "%s: lex: new accepting symbol this->sym = %d greedy = %d\n",
            UNICC_PARSER, this->sym, this->symbols[ this->sym ].greedy );
#endif
                    if( this->sym == @@eof )
                    {
                        state = -1; /* test! */
                        break;
                    }

                    /* Stop if matched symbol should be parsed nongreedy */
                    if( !this->symbols[ this->sym ].greedy )
                    {
                        state = -1;
                        break;
                    }
                }

                next = this->get_input( ++len );
                break;
            }

            chr += 2;
        }
    }
    while( state > -1 && next != this->eof );

    if( this->sym > -1 )
    {
#if UNICC_SEMANTIC_TERM_SEL
        /*
            Execute scanner actions, if existing, but with
            UNICC_ON_SHIFT = 0, so that no memory allocation
            should be performed. This actions should only be
            handled if there are semantic-code dependent
            terminal symbol selections.

            tos is incremented here, if the semantic code
            stores data for the symbol. It won't get lost
            in case of a shift.
        */
        this->alloc_stack();
        this->tos++;

        next = this->buf[ this->len ];
        this->buf[ this->len ] = 0;

#define UNICC_ON_SHIFT 	0
        switch( this->sym )
        {
@@scan_actions

            default:
                break;
        }
#undef UNICC_ON_SHIFT

        this->buf[ this->len ] = next;

        this->tos--;
#endif /* UNICC_SEMANTIC_TERM_SEL */
    }

#if UNICC_DEBUG	> 1
fprintf( stderr, "%s: lex: function exits, this->sym = %d, this->len = %d\n",
            UNICC_PARSER, this->sym, this->len );
#endif
}
#endif
