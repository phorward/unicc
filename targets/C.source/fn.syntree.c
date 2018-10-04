#if UNICC_SYNTAXTREE
UNICC_STATIC @@prefix_syntree* @@prefix_syntree_free( @@prefix_syntree* node )
{
	if( !node )
		return (@@prefix_syntree*)NULL;

	@@prefix_syntree_free( node->child );
	@@prefix_syntree_free( node->next );

	if( node->token )
		free( node->token );

	free( node );
	return (@@prefix_syntree*)NULL;
}

UNICC_STATIC @@prefix_syntree* @@prefix_syntree_append(
	@@prefix_pcb* pcb, UNICC_SCHAR* token )
{
	@@prefix_syntree*	node;
	@@prefix_syntree*	last;

	if( !( node = (@@prefix_syntree*)malloc(
			sizeof( @@prefix_syntree ) ) ) )
		UNICC_OUTOFMEM( pcb );

	memset( node, 0, sizeof( @@prefix_syntree ) );
	memcpy( &( node->symbol ), pcb->tos, sizeof( @@prefix_tok ) );

	if( token )
	{
		#if !UNICC_WCHAR
		if( !( node->token = strdup( token ) ) )
			UNICC_OUTOFMEM( pcb );
		#else
		if( !( node->token = wcsdup( token ) ) )
			UNICC_OUTOFMEM( pcb );
		#endif
	}

	if( ( last = pcb->syntax_tree ) )
	{
		while( last->next )
			last = last->next;

		last->next = node;
		node->prev = last;
	}
	else
		pcb->syntax_tree = node;

	return node;
}
#endif
