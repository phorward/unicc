@@goal-type @@prefix_parser::parse( void )
{
	@@goal-type			ret;
	@@prefix_ast*		node;
	@@prefix_ast*		lnode;

#if UNICC_DEBUG
	@@prefix_vtype*		vptr;
	FILE* 				@@prefix_dbg = stderr;
#endif

	// Initialize parser
	this->stacksize = 0;
	this->alloc_stack();

	memset( this->tos, 0, sizeof( @@prefix_tok ) );

	this->act = UNICC_SHIFT;
	this->is_eof = false;
	this->sym = this->old_sym = -1;
	this->line = this->column = 1;

	memset( &this->test, 0, sizeof( @@prefix_vtype ) );

	// Begin of main parser loop
	while( true )
	{
		// Reduce
		while( this->act & UNICC_REDUCE )
		{
#if UNICC_DEBUG
			fprintf( @@prefix_dbg, "%s: << "
					"reducing by production %d (%s)\n",
						UNICC_PARSER, this->idx,
							this->productions[ this->idx ].definition );
#endif
			// Set default left-hand side
			this->lhs = this->productions[ this->idx ].lhs;

			// Run reduction code
			memset( &( this->ret ), 0, sizeof( @@prefix_vtype ) );

			switch( this->idx )
			{
@@actions
			}

			// Drop right-hand side
			node = NULL;

			for( int i = 0; i < this->productions[ this->idx ].length; i++ )
			{
				if( this->tos->node )
				{
					if( node )
					{
						while( node->prev )
							node = node->prev;

						node->prev = this->tos->node;
						this->tos->node->next = node;
					}

					node = this->tos->node;
					this->tos->node = NULL;
				}

				this->tos--;
			}

			if( node )
			{
				if( lnode = this->tos->node )
				{
					while( lnode->next )
						lnode = lnode->next;

					lnode->next = node;
					node->prev = lnode;
				}
				else
					this->tos->node = node;
			}

			if( *this->productions[ this->idx ].emit )
			{
				node = this->ast_create(
							this->productions[ this->idx ].emit, NULL);

				node->child = this->tos->node;
				this->tos->node = node;
			}

			// Enforced error in semantic actions?
			if( this->act == UNICC_ERROR )
				break;

			// Goal symbol reduced, and stack is empty?
			if( this->lhs == @@goal && this->tos == this->stack )
			{
				memcpy( &( this->tos->value ), &( this->ret ),
							sizeof( @@prefix_vtype ) );
				this->ast = this->tos->node;

				UNICC_CLEARIN( this );

				this->act = UNICC_SUCCESS;

				#if UNICC_DEBUG
				fprintf( stderr, "%s: goal symbol reduced, exiting parser\n",
						UNICC_PARSER );
				#endif
				break;
			}

			#if UNICC_DEBUG
			fprintf( @@prefix_dbg, "%s: after reduction, "
						"shifting nonterminal %d (%s)\n",
							UNICC_PARSER, this->lhs,
								this->symbols[ this->lhs ].name );
			#endif

			this->get_go();

			this->tos++;
			this->tos->node = NULL;

			memcpy( &( this->tos->value ), &( this->ret ),
						sizeof( @@prefix_vtype ) );
			this->tos->symbol = &( this->symbols[ this->lhs ] );
			this->tos->state = ( this->act & UNICC_REDUCE ) ? -1 : this->idx;
			this->tos->line = this->line;
			this->tos->column = this->column;
		}

		if( this->act == UNICC_SUCCESS || this->act == UNICC_ERROR )
			break;

		/* If in error recovery, replace old-symbol */
		if( this->error_delay == UNICC_ERROR_DELAY
				&& ( this->sym = this->old_sym ) < 0 )
		{
			/* If symbol is invalid, try to find new token */
			#if UNICC_DEBUG
			fprintf( @@prefix_dbg, "%s: error recovery: "
				"old token invalid, requesting new token\n",
						UNICC_PARSER );
			#endif

			while( !this->get_sym() )
			{
				/* Skip one character */
				this->len = 1;

				UNICC_CLEARIN( this );
			}

			#if UNICC_DEBUG
			fprintf( @@prefix_dbg, "%s: error recovery: "
				"new token %d (%s)\n", UNICC_PARSER, this->sym,
					this->symbols[ this->sym ].name );
			#endif
		}
		else
			this->get_sym();

#if UNICC_DEBUG
		fprintf( @@prefix_dbg, "%s: current token %d (%s)\n",
					UNICC_PARSER, this->sym,
						( this->sym < 0 ) ? "(null)" :
							this->symbols[ this->sym ].name );
#endif

		/* Get action table entry */
		if( !this->get_act() )
		{
			/* Error state, try to recover */
			if( this->handle_error(
#if UNICC_DEBUG
					@@prefix_dbg
#else
					NULL
#endif
					) )
				break;
		}

#if UNICC_DEBUG
		fprintf( @@prefix_dbg,
			"%s: sym = %d (%s) [len = %d] tos->state = %d act = %s idx = %d\n",
				UNICC_PARSER, this->sym,
					( ( this->sym >= 0 ) ?
						this->symbols[ this->sym ].name :
							"(invalid symbol id)" ),
					this->len, this->tos->state,
						( ( this->act == UNICC_SHIFT & UNICC_REDUCE ) ?
								"shift/reduce" :
							( this->act & UNICC_SHIFT ) ?
									"shift" : "reduce" ), this->idx );
#if UNICC_STACKDEBUG
		this->dbg_stack( @@prefix_dbg, this->stack, this->tos );
#endif
#endif

		/* Shift */
		if( this->act & UNICC_SHIFT )
		{
			this->next = this->buf[ this->len ];
			this->buf[ this->len ] = '\0';

#if UNICC_DEBUG
			fprintf( @@prefix_dbg, "%s: >> shifting terminal %d (%s)\n",
			UNICC_PARSER, this->sym, this->symbols[ this->sym ].name );
#endif

			this->alloc_stack();
			this->tos++;
			this->tos->node = NULL;

			/*
				Execute scanner actions, if existing.
				Here, UNICC_ON_SHIFT is set to 1, so that shifting-
				related operations will be performed.
			*/
#define UNICC_ON_SHIFT	1
			switch( this->sym )
			{
@@scan_actions

				default:
					@@top-value = this->get_input( 0 );
					break;
			}
#undef UNICC_ON_SHIFT

			this->tos->state = ( this->act & UNICC_REDUCE ) ? -1 : this->idx;
			this->tos->symbol = &( this->symbols[ this->sym ] );
			this->tos->line = this->line;
			this->tos->column = this->column;

			if( *this->tos->symbol->emit )
				this->tos->node = this->ast_create(
									this->tos->symbol->emit,
										this->get_lexem() );
			else
				this->tos->node = NULL;

			this->buf[ this->len ] = this->next;

			/* Perform the shift on input */
			if( this->sym != @@eof && this->sym != @@error )
			{
				UNICC_CLEARIN( this );
				this->old_sym = -1;
			}

			if( this->error_delay )
				this->error_delay--;
		}
	}

	#if UNICC_DEBUG
	fprintf( @@prefix_dbg, "%s: parse completed with %d errors\n",
		UNICC_PARSER, this->error_count );
	#endif

	// Save return value
	ret = @@goal-value;

	// Clean up parser control block
	UNICC_CLEARIN( this );

	if( this->stack )
	{
		free( this->stack );
		this->stack = NULL;
	}

#if UNICC_UTF8
	if( this->lexem )
	{
		free( this->lexem );
		this->lexem = NULL;
	}
#endif

	return ret;
}
