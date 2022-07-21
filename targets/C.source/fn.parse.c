@@goal-type @@prefix_parse( @@prefix_pcb* pcb )
{
	@@goal-type			ret;
	int					i;

	@@prefix_ast*		node;
	@@prefix_ast*		lnode;

#if UNICC_DEBUG
	@@prefix_vtype*		vptr;
	FILE* 				@@prefix_dbg;

	@@prefix_dbg = stderr;
#endif

	/* If there is no Parser Control Block given, allocate your own one! */
	if( !pcb )
	{
		if( !( pcb = (@@prefix_pcb*)malloc( sizeof( @@prefix_pcb ) ) ) )
		{
			/* Can't allocate memory */
			UNICC_OUTOFMEM( pcb );
			return (@@goal-type)0;
		}

		memset( pcb, 0, sizeof( @@prefix_pcb ) );
		pcb->is_internal = 1;
	}
	else
		pcb->is_internal = 0;

	/* Initialize Parser Control Block */
	pcb->stacksize = 0;
	if( @@prefix_alloc_stack( pcb ) < 0 )
		return (@@goal-type)0;

	memset( pcb->tos, 0, sizeof( @@prefix_tok ) );

	pcb->act = UNICC_SHIFT;
	pcb->sym = -1;
	pcb->old_sym = -1;
	pcb->line = 1;
	pcb->column = 1;

	memset( &pcb->test, 0, sizeof( @@prefix_vtype ) );

	/* Begin of main parser loop */
	while( 1 )
	{
		/* Reduce */
		while( pcb->act & UNICC_REDUCE )
		{
#if UNICC_DEBUG
			fprintf( @@prefix_dbg, "%s: << "
					"reducing by production %d (%s)\n",
						UNICC_PARSER, pcb->idx,
							@@prefix_productions[ pcb->idx ].definition );
#endif
			/* Set default left-hand side */
			pcb->lhs = @@prefix_productions[ pcb->idx ].lhs;

			/* Run reduction code */
			memset( &( pcb->ret ), 0, sizeof( @@prefix_vtype ) );

			switch( pcb->idx )
			{
@@actions
			}

			/* Drop right-hand side */
			/* TODO: Destructor callbacks? */
			for( i = 0, node = (@@prefix_ast*)NULL;
					i < @@prefix_productions[ pcb->idx ].length;
						i++ )
			{
				if( pcb->tos->node )
				{
					if( node )
					{
						while( node->prev )
							node = node->prev;

						node->prev = pcb->tos->node;
						pcb->tos->node->next = node;
					}

					node = pcb->tos->node;
					pcb->tos->node = (@@prefix_ast*)NULL;
				}

				pcb->tos--;
			}

			/* This could be done if no AST construction would be done: */
			/* pcb->tos -= @@prefix_productions[ pcb->idx ].length; */

			if( node )
			{
				if( ( lnode = pcb->tos->node ) )
				{
					while( lnode->next )
						lnode = lnode->next;

					lnode->next = node;
					node->prev = lnode;
				}
				else
					pcb->tos->node = node;
			}

			if( *@@prefix_productions[ pcb->idx ].emit )
			{
				if( ( node = @@prefix_ast_create( pcb,
								@@prefix_productions[ pcb->idx ].emit,
									(UNICC_SCHAR*)NULL ) ) )
				{
					node->child = pcb->tos->node;
					pcb->tos->node = node;
				}
			}

			/* Enforced error in semantic actions? */
			if( pcb->act == UNICC_ERROR )
				break;

			/* Goal symbol reduced, and stack is empty? */
			if( pcb->lhs == @@goal && pcb->tos == pcb->stack )
			{
				memcpy( &( pcb->tos->value ), &( pcb->ret ),
							sizeof( @@prefix_vtype ) );
				pcb->ast = pcb->tos->node;

				UNICC_CLEARIN( pcb );

				pcb->act = UNICC_SUCCESS;

				#if UNICC_DEBUG
				fprintf( stderr, "%s: goal symbol reduced, exiting parser\n",
						UNICC_PARSER );
				#endif
				break;
			}

			#if UNICC_DEBUG
			fprintf( @@prefix_dbg, "%s: after reduction, "
						"shifting nonterminal %d (%s)\n",
							UNICC_PARSER, pcb->lhs,
								@@prefix_symbols[ pcb->lhs ].name );
			#endif

			@@prefix_get_go( pcb );

			pcb->tos++;
			pcb->tos->node = (@@prefix_ast*)NULL;

			memcpy( &( pcb->tos->value ), &( pcb->ret ),
						sizeof( @@prefix_vtype ) );
			pcb->tos->symbol = &( @@prefix_symbols[ pcb->lhs ] );
			pcb->tos->state = ( pcb->act & UNICC_REDUCE ) ? -1 : pcb->idx;
			pcb->tos->line = pcb->line;
			pcb->tos->column = pcb->column;
		}

		if( pcb->act == UNICC_SUCCESS || pcb->act == UNICC_ERROR )
			break;

		/* If in error recovery, replace old-symbol */
		if( pcb->error_delay == UNICC_ERROR_DELAY
				&& ( pcb->sym = pcb->old_sym ) < 0 )
		{
			/* If symbol is invalid, try to find new token */
			#if UNICC_DEBUG
			fprintf( @@prefix_dbg, "%s: error recovery: "
				"old token invalid, requesting new token\n",
						UNICC_PARSER );
			#endif

%%%ifgen STDTPL
			while( !@@prefix_get_sym( pcb ) )
%%%ifgen UNICC4C
			while( !@@prefix_lex( pcb ) )
%%%end
			{
				/* Skip one character */
				pcb->len = 1;

				UNICC_CLEARIN( pcb );
			}

			#if UNICC_DEBUG
			fprintf( @@prefix_dbg, "%s: error recovery: "
				"new token %d (%s)\n", UNICC_PARSER, pcb->sym,
					@@prefix_symbols[ pcb->sym ].name );
			#endif
		}
		else
		{
%%%ifgen STDTPL
			@@prefix_get_sym( pcb );
%%%ifgen UNICC4C
			@@prefix_lex( pcb );
%%%end
		}

#if UNICC_DEBUG
		fprintf( @@prefix_dbg, "%s: current token %d (%s)\n",
					UNICC_PARSER, pcb->sym,
						( pcb->sym < 0 ) ? "(null)" :
							@@prefix_symbols[ pcb->sym ].name );
#endif

		/* Get action table entry */
		if( !@@prefix_get_act( pcb ) )
		{
			/* Error state, try to recover */
			if( @@prefix_handle_error( pcb,
#if UNICC_DEBUG
					@@prefix_dbg
#else
					(FILE*)NULL
#endif
					) )
				break;
		}

#if UNICC_DEBUG
		fprintf( @@prefix_dbg,
			"%s: sym = %d (%s) [len = %d] tos->state = %d act = %s idx = %d\n",
				UNICC_PARSER, pcb->sym,
					( ( pcb->sym >= 0 ) ?
						@@prefix_symbols[ pcb->sym ].name :
							"(invalid symbol id)" ),
					pcb->len, pcb->tos->state,
						( ( pcb->act == UNICC_SHIFT & UNICC_REDUCE ) ?
								"shift/reduce" :
							( pcb->act & UNICC_SHIFT ) ?
									"shift" : "reduce" ), pcb->idx );
#if UNICC_STACKDEBUG
		@@prefix_dbg_stack( @@prefix_dbg, pcb->stack, pcb->tos );
#endif
#endif

		/* Shift */
		if( pcb->act & UNICC_SHIFT )
		{
			pcb->next = pcb->buf[ pcb->len ];
			pcb->buf[ pcb->len ] = '\0';

#if UNICC_DEBUG
			fprintf( @@prefix_dbg, "%s: >> shifting terminal %d (%s)\n",
			UNICC_PARSER, pcb->sym, @@prefix_symbols[ pcb->sym ].name );
#endif

			if( @@prefix_alloc_stack( pcb ) < 0 )
				return (@@goal-type)0;

			pcb->tos++;
			pcb->tos->node = (@@prefix_ast*)NULL;

			/*
				Execute scanner actions, if existing.
				Here, UNICC_ON_SHIFT is set to 1, so that shifting-
				related operations will be performed.
			*/
#define UNICC_ON_SHIFT	1
			switch( pcb->sym )
			{
@@scan_actions

				default:
					@@top-value = @@prefix_get_input( pcb, 0 );
					break;
			}
#undef UNICC_ON_SHIFT

			pcb->tos->state = ( pcb->act & UNICC_REDUCE ) ? -1 : pcb->idx;
			pcb->tos->symbol = &( @@prefix_symbols[ pcb->sym ] );
			pcb->tos->line = pcb->line;
			pcb->tos->column = pcb->column;

			if( *pcb->tos->symbol->emit )
				pcb->tos->node = @@prefix_ast_create( pcb,
									pcb->tos->symbol->emit,
										@@prefix_lexem( pcb ) );
			else
				pcb->tos->node = (@@prefix_ast*)NULL;

			pcb->buf[ pcb->len ] = pcb->next;

			/* Perform the shift on input */
			if( pcb->sym != @@eof && pcb->sym != @@error )
			{
				UNICC_CLEARIN( pcb );
				pcb->old_sym = -1;
			}

			if( pcb->error_delay )
				pcb->error_delay--;
		}
	}

	#if UNICC_DEBUG
	fprintf( @@prefix_dbg, "%s: parse completed with %d errors\n",
		UNICC_PARSER, pcb->error_count );
	#endif

	/* Save return value */
	ret = @@goal-value;

	/* Clean up parser control block */
	free( pcb->buf );
	free( pcb->stack );
#if UNICC_UTF8
	free( pcb->lexem );
#endif

	/* Clean memory of self-allocated parser control block */
	if( pcb->is_internal )
		free( pcb );

	return ret;
}
