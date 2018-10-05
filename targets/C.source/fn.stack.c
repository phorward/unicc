UNICC_STATIC int @@prefix_alloc_stack( @@prefix_pcb* pcb )
{
	unsigned int	size;

	if( !pcb->stacksize )
	{
		if( !( pcb->tos = pcb->stack = (@@prefix_tok*)malloc(
				UNICC_MALLOCSTEP * sizeof( @@prefix_tok ) ) ) )
		{
			UNICC_OUTOFMEM( pcb );
			return -1;
		}

		pcb->stacksize = UNICC_MALLOCSTEP;
	}
	else if( ( size = (unsigned int)( pcb->tos - pcb->stack ) )
				== pcb->stacksize )
	{
		@@prefix_tok*	ptr;

		if( !( ptr = (@@prefix_tok*)realloc( pcb->stack,
				( pcb->stacksize + UNICC_MALLOCSTEP )
					* sizeof( @@prefix_tok ) ) ) )
		{
			UNICC_OUTOFMEM( pcb );

			if( pcb->stack )
				free( pcb->stack );

			return -1;
		}

		pcb->tos = pcb->stack = ptr;

		pcb->stacksize += UNICC_MALLOCSTEP;
		pcb->tos += size;
	}

	return 0;
}
