UNICC_STATIC int @@prefix_get_go( @@prefix_pcb* pcb, int sym )
{
	int i;

	for( i = 1; i < @@prefix_go[ pcb->tos->state ][0] * 3; i += 3 )
	{
		if( @@prefix_go[ pcb->tos->state ][i] == sym )
		{
			pcb->act = @@prefix_go[ pcb->tos->state ][ i + 1 ];
			pcb->idx = @@prefix_go[ pcb->tos->state ][ i + 2 ];
			return 1;
		}
	}

	return 0;
}
