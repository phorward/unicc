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
		if( !( pcb->tos = pcb->stack = (@@prefix_tok*)realloc( pcb->tos,
				( pcb->stacksize + UNICC_MALLOCSTEP )
					* sizeof( @@prefix_tok ) ) ) )
		{
			UNICC_OUTOFMEM( pcb );
			return -1;
		}

		pcb->stacksize += UNICC_MALLOCSTEP;
		pcb->tos += size;
	}

	return 0;
}
