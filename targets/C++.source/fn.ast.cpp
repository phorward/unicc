@@prefix_ast* @@prefix_parser::ast_free( @@prefix_ast* node )
{
	if( !node )
		return NULL;

	this->ast_free( node->child );
	this->ast_free( node->next );

	if( node->token )
		free( node->token );

	free( node );
	return NULL;
}

@@prefix_ast* @@prefix_parser::ast_create( const char* emit, UNICC_SCHAR* token )
{
	@@prefix_ast*	node;

	if( !( node = (@@prefix_ast*)malloc( sizeof( @@prefix_ast ) ) ) )
		UNICC_OUTOFMEM;

	memset( node, 0, sizeof( @@prefix_ast ) );

	node->emit = emit;

	if( token )
	{
		#if !UNICC_WCHAR
		if( !( node->token = strdup( token ) ) )
			UNICC_OUTOFMEM;
		#else
		if( !( node->token = wcsdup( token ) ) )
			UNICC_OUTOFMEM;
		#endif
	}

	return node;
}

void @@prefix_parser::ast_print( FILE* stream, @@prefix_ast* node )
{
	int 		i;
	static int 	rec;

	if( !node )
		return;

	if( !stream )
		stream = stderr;

	while( node )
	{
		for( i = 0; i < rec; i++ )
			fprintf( stream,  " " );

		fprintf( stream, "%s", node->emit );

		if( node->token && strcmp( node->emit, node->token ) != 0 )
			fprintf( stream, " (%s)", node->token );

		fprintf( stream, "\n" );

		rec++;
		this->ast_print( stream, node->child );
		rec--;

		node = node->next;
	}
}
