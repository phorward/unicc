bool @@prefix_parser::get_sym( void )
{
	this->sym = -1;
	this->len = 0;

#if @@mode
	do
	{
#endif
#if !@@mode

#if UNICC_DEBUG > 2
		fprintf( stderr, "%s: get sym: state = %d dfa_select = %d\n",
					UNICC_PARSER, this->tos->state,
						@@prefix_dfa_select[ this->tos->state ] );
#endif

		if( this->dfa_select[ this->tos->state ] > -1 )
			this->lex();
		/*
		 * If there is no DFA state machine,
		 * try to identify the end-of-file symbol.
		 * If this also fails, a parse error will
		 * raise.
		 */
		else if( this->get_input( 0 ) == this->eof )
			this->sym = @@eof;
#else
		this->lex();
#endif /* !@@mode */

#if @@mode

		if( this->sym > -1 && this->symbols[ this->sym ].whitespace )
		{
			UNICC_CLEARIN( this );
			continue;
		}

		break;
	}
	while( 1 );
#endif /* @@mode */

	return this->sym > -1;
}
