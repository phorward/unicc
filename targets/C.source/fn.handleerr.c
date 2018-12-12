UNICC_STATIC int @@prefix_handle_error( @@prefix_pcb* pcb, FILE* @@prefix_dbg )
{
#if UNICC_DEBUG
	int					i;
#endif

	if( !pcb->error_delay )
	{
#if UNICC_DEBUG
		fprintf( @@prefix_dbg, "%s: !!!PARSE ERROR!!!\n"
				"%s: error recovery: current token %d (%s)\n",
					UNICC_PARSER, UNICC_PARSER, pcb->sym,
						( ( pcb->sym >= 0 ) ?
							@@prefix_symbols[ pcb->sym ].name :
								"(null)" ) );

%%%ifgen STDTPL /* TODO:UNICC4C */
		/*
		fprintf( @@prefix_dbg,
				"%s: error recovery: expecting ", UNICC_PARSER );
				
		for( i = 1; i < @@prefix_act[ pcb->tos->state ][0] * 3; i += 3 )
		{
			fprintf( @@prefix_dbg, "%d (%s)%s",
				@@prefix_act[ pcb->tos->state ][i],
				@@prefix_symbols[ @@prefix_act[ pcb->tos->state ][i] ].name,
				( i == @@prefix_act[ pcb->tos->state ][0] * 3 - 3 ) ?
						"\n" : ", " );
		}
		*/
%%%end
		
		fprintf( @@prefix_dbg, "\n%s: error recovery: error_delay is %d, %s\n",
					UNICC_PARSER, pcb->error_delay,
					( pcb->error_delay ? "error recovery runs silently" :
						"error is reported before its recover!" ) );
#endif
	}

%%%ifgen STDTPL
#if @@error < 0
	/* No error token defined? Then exit here... */
%%%ifgen UNICC4C
%%%code if( error_sym < 0 )
%%%code {
%%%end

#if UNICC_DEBUG
	fprintf( @@prefix_dbg,
		"%s: error recovery: No error resync token used by grammar, "
			"exiting parser.\n", UNICC_PARSER );
#endif

	UNICC_PARSE_ERROR( pcb );
	pcb->error_count++;

	return 1;
%%%ifgen STDTPL
#else
%%%ifgen UNICC4C
%%%code }
%%%code else
%%%code {
%%%end
	/*
	@@prefix_pcb 		org_pcb;
	*/

#if UNICC_DEBUG
	fprintf( @@prefix_dbg, "%s: error recovery: "
		"trying to recover...\n", UNICC_PARSER );
#if UNICC_STACKDEBUG
	@@prefix_dbg_stack( @@prefix_dbg, pcb->stack, pcb->tos );
#endif
#endif

	#if 0
	/* Create a copy of original parser control block (maybe later...) */
	memcpy( &org_pcb, pcb, sizeof( @@prefix_pcb ) );
	org_pcb.buf = (char*)malloc( ( strlen( pcb->buf ) / UNICC_MALLOCSTEP +
					( !( strlen( pcb->buf ) % UNICC_MALLOCSTEP ) ? 1 : 0 ) )
							* UNICC_MALLOCSTEP );
	strcpy( org_pcb.buf, pcb->buf );
	#endif

	/* Remember previous symbol, or discard it */
	if( pcb->error_delay != UNICC_ERROR_DELAY )
		pcb->old_sym = pcb->sym;
	else
	{
		pcb->old_sym = -1;
		pcb->len = 1;
		UNICC_CLEARIN( pcb );
	}

	/* Try to shift on error resync */
	pcb->sym = @@error;

	while( pcb->tos >= pcb->stack )
	{
#if UNICC_DEBUG
		fprintf( @@prefix_dbg, "%s: error recovery: in state %d, trying "
				"to shift error resync token...\n",
					UNICC_PARSER, pcb->tos->state );
#if UNICC_STACKDEBUG
		@@prefix_dbg_stack( @@prefix_dbg, pcb->stack, pcb->tos );
#endif
#endif

        /* Default production */
        if( ( pcb->idx = @@prefix_def_prod[ pcb->tos->state ] ) > -1 )
            pcb->act = UNICC_REDUCE;
        else
            pcb->act = UNICC_ERROR;

        /* Get action table entry */
		switch( pcb->tos->state )
		{
@@action-table
		}

		if( pcb->act ) /* !UNICC_ERROR */
		{
			/* Shift */
			if( pcb->act & UNICC_SHIFT )
			{
#if UNICC_DEBUG
				fprintf( @@prefix_dbg, "%s: error recovery: "
							"error resync shifted\n", UNICC_PARSER );
#endif
				break;
			}
		}
		
#if UNICC_DEBUG
		fprintf( @@prefix_dbg, "%s: error recovery: failed, "
					"discarding token '%s'\n", UNICC_PARSER,
						pcb->tos->symbol ?
							pcb->tos->symbol->name : "NULL" );
#endif

		/* Discard one token from stack */
		/* TODO: Discarded token memory (semantic action) */
		pcb->tos--;
	}

	if( pcb->tos <= pcb->stack )
	{
#if UNICC_DEBUG
	fprintf( @@prefix_dbg, "%s: error recovery: "
				"Can't recover this issue, stack is empty.\n",
					UNICC_PARSER );	
#endif
		UNICC_PARSE_ERROR( pcb );
		pcb->error_count++;

		return 1;
	}
	
#if UNICC_DEBUG
	fprintf( @@prefix_dbg, "%s: error recovery: "
				"trying to continue with modified parser state\n",
					UNICC_PARSER );	
#endif

	pcb->error_delay = UNICC_ERROR_DELAY + 1;

	return 0;

%%%ifgen STDTPL
#endif /* @@error >= 0 */
%%%ifgen UNICC4C
%%%code }
%%%end
}
