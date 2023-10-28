#if @@number-of-dfa-machines
UNICC_STATIC void @@prefix_lex( @@prefix_pcb* pcb )
{
    int 			dfa_st	= 0;
    unsigned int	len		= 0;
    int				chr;
    UNICC_CHAR		next;
#if !@@mode
    int				mach	= @@prefix_dfa_select[ pcb->tos->state ];
#else
    int				mach	= 0;
#endif

    next = @@prefix_get_input( pcb, len );
#if UNICC_DEBUG	> 1
fprintf( stderr, "%s: lex: next = %d\n", UNICC_PARSER, next );
#endif

    if( next == pcb->eof )
    {
        pcb->sym = @@eof;
        return;
    }

    do
    {
#if UNICC_DEBUG	> 1
fprintf( stderr, "%s: lex: next = %d\n", UNICC_PARSER, next );
#endif

        chr = @@prefix_dfa_idx[ mach ][ dfa_st ];
#if UNICC_DEBUG	> 1
fprintf( stderr, "%s: lex: chr = %d\n", UNICC_PARSER, chr );
#endif

        dfa_st = -1;
#if UNICC_DEBUG	> 1
fprintf( stderr, "%s: lex: FIRST next = %d @@prefix_dfa_chars[ chr ] = %d, "
            "@@prefix_dfa_chars[ chr+1 ] = %d\n", UNICC_PARSER, next,
                @@prefix_dfa_chars[ chr ], @@prefix_dfa_chars[ chr + 1 ] );
#endif
        while( @@prefix_dfa_chars[ chr ] > -1 )
        {
#if UNICC_DEBUG	> 1
fprintf( stderr, "%s: lex: next = %d @@prefix_dfa_chars[ chr ] = %d, "
        "@@prefix_dfa_chars[ chr+1 ] = %d\n", UNICC_PARSER, next,
            @@prefix_dfa_chars[ chr ], @@prefix_dfa_chars[ chr + 1 ] );
#endif
            if( next >= @@prefix_dfa_chars[ chr ] &&
                next <= @@prefix_dfa_chars[ chr+1 ] )
            {
                dfa_st = *( @@prefix_dfa_trans + ( chr / 2 ) );
#if UNICC_DEBUG	> 1
fprintf( stderr, "%s: lex: dfa_st = %d\n", UNICC_PARSER, dfa_st );
#endif
                if( @@prefix_dfa_accept[ mach ][ dfa_st ] > 0 )
                {
                    pcb->len = len + 1;
                    pcb->sym = @@prefix_dfa_accept[ mach ][ dfa_st ] - 1;

#if UNICC_DEBUG	> 1
fprintf( stderr, "%s: lex: new accepting symbol pcb->sym = %d greedy = %d\n",
            UNICC_PARSER, pcb->sym, @@prefix_symbols[ pcb->sym ].greedy );
#endif
                    if( pcb->sym == @@eof )
                    {
                        dfa_st = -1; /* test! */
                        break;
                    }

                    /* Stop if matched symbol should be parsed nongreedy */
                    if( !@@prefix_symbols[ pcb->sym ].greedy )
                    {
                        dfa_st = -1;
                        break;
                    }
                }

                next = @@prefix_get_input( pcb, ++len );
                break;
            }

            chr += 2;
        }
    }
    while( dfa_st > -1 && next != pcb->eof );

    if( pcb->sym > -1 )
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
        @@prefix_alloc_stack( pcb );
        pcb->tos++;

        next = pcb->buf[ pcb->len ];
        pcb->buf[ pcb->len ] = '\0';

#define UNICC_ON_SHIFT 	0
        switch( pcb->sym )
        {
@@scan_actions

            default:
                break;
        }
#undef UNICC_ON_SHIFT

        pcb->buf[ pcb->len ] = next;

        pcb->tos--;
#endif /* UNICC_SEMANTIC_TERM_SEL */
    }

#if UNICC_DEBUG	> 1
fprintf( stderr, "%s: lex: function exits, pcb->sym = %d, pcb->len = %d\n",
            UNICC_PARSER, pcb->sym, pcb->len );
#endif
}
#endif
