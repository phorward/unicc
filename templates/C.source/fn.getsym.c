UNICC_STATIC int @@prefix_get_sym( @@prefix_pcb* pcb )
{
	pcb->sym = -1;
	pcb->len = 0;

#if @@mode
	do
	{
#endif
#if !@@mode

#if UNICC_DEBUG > 2
		fprintf( stderr, "%s: get sym: state = %d dfa_select = %d\n",
					UNICC_PARSER, pcb->tos->state,
						@@prefix_dfa_select[ pcb->tos->state ] );
#endif

		if( @@prefix_dfa_select[ pcb->tos->state ] > -1 )
			@@prefix_lex( pcb );
		/* 
		 * If there is no DFA state machine,
		 * try to identify the end-of-file symbol.
		 * If this also fails, a parse error will
		 * raise.
		 */
		else if( @@prefix_get_input( pcb, 0 ) == pcb->eof )
			pcb->sym = @@eof;
#else
		@@prefix_lex( pcb );
#endif /* !@@mode */

#if @@mode

		if( pcb->sym > -1 && @@prefix_symbols[ pcb->sym ].whitespace )
		{
			UNICC_CLEARIN( pcb );
			continue;
		}
		
		break;
	}
	while( 1 );
#endif /* @@mode */

	return ( pcb->sym > -1 ) ? 1 : 0;
}
