bool @@prefix_parser::handle_error( FILE* @@prefix_dbg )
{
	if( !this->error_delay )
	{
#if UNICC_DEBUG
		fprintf( @@prefix_dbg, "%s: !!!PARSE ERROR!!!\n"
				"%s: error recovery: current token %d (%s)\n",
					UNICC_PARSER, UNICC_PARSER, this->sym,
						( ( this->sym >= 0 ) ?
							this->symbols[ this->sym ].name :
								"(null)" ) );

		fprintf( @@prefix_dbg,
				"%s: error recovery: expecting ", UNICC_PARSER );

		for( int i = 1; i < this->actions[ this->tos->state ][0] * 3; i += 3 )
		{
			fprintf( @@prefix_dbg, "%d (%s)%s",
				this->actions[ this->tos->state ][i],
				this->symbols[ this->actions[ this->tos->state ][i] ].name,
				( i == this->actions[ this->tos->state ][0] * 3 - 3 ) ?
						"\n" : ", " );
		}

		fprintf( @@prefix_dbg, "\n%s: error recovery: error_delay is %d, %s\n",
					UNICC_PARSER, this->error_delay,
					( this->error_delay ? "error recovery runs silently" :
						"error is reported before its recover!" ) );
#endif
	}

#if @@error < 0
	/* No error token defined? Then exit here... */

#if UNICC_DEBUG
	fprintf( @@prefix_dbg,
		"%s: error recovery: No error resync token used by grammar, "
			"exiting parser.\n", UNICC_PARSER );
#endif

	UNICC_PARSE_ERROR( this );
	this->error_count++;

	return 1;
#else

#if UNICC_DEBUG
	fprintf( @@prefix_dbg, "%s: error recovery: "
		"trying to recover...\n", UNICC_PARSER );
#if UNICC_STACKDEBUG
	@@prefix_dbg_stack( @@prefix_dbg, this->stack, this->tos );
#endif
#endif

	/* Remember previous symbol, or discard it */
	if( this->error_delay != UNICC_ERROR_DELAY )
		this->old_sym = this->sym;
	else
	{
		this->old_sym = -1;
		this->len = 1;
		UNICC_CLEARIN( this );
	}

	/* Try to shift on error resync */
	this->sym = @@error;

	while( this->tos >= this->stack )
	{
#if UNICC_DEBUG
		fprintf( @@prefix_dbg, "%s: error recovery: in state %d, trying "
				"to shift error resync token...\n",
					UNICC_PARSER, this->tos->state );
#if UNICC_STACKDEBUG
		this->dbg_stack( @@prefix_dbg, this->stack, this->tos );
#endif
#endif
		if( @@prefix_get_act( this ) )
		{
			/* Shift */
			if( this->act & UNICC_SHIFT )
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
						this->tos->symbol ?
							this->tos->symbol->name : "NULL" );
#endif

		/* Discard one token from stack */
		/* TODO: Discarded token memory (semantic action) */
		this->tos--;
	}

	if( this->tos <= this->stack )
	{
#if UNICC_DEBUG
	fprintf( @@prefix_dbg, "%s: error recovery: "
				"Can't recover this issue, stack is empty.\n",
					UNICC_PARSER );
#endif
		UNICC_PARSE_ERROR( this );
		this->error_count++;

		return true;
	}

#if UNICC_DEBUG
	fprintf( @@prefix_dbg, "%s: error recovery: "
				"trying to continue with modified parser state\n",
					UNICC_PARSER );
#endif

	this->error_delay = UNICC_ERROR_DELAY + 1;

	return false;

#endif /* @@error >= 0 */
}
