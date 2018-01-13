bool @@prefix_parser::alloc_stack( void )
{
	if( !this->stacksize )
	{
		if( !( this->tos = this->stack = (@@prefix_tok*)malloc(
				UNICC_MALLOCSTEP * sizeof( @@prefix_tok ) ) ) )
		{
			UNICC_OUTOFMEM;
			return false;
		}

		this->stacksize = UNICC_MALLOCSTEP;
	}
	else if( ( this->tos - this->stack ) == this->stacksize )
	{
		size_t size = ( this->tos - this->stack );

		if( !( this->tos = this->stack = (@@prefix_tok*)realloc( this->tos,
				( this->stacksize + UNICC_MALLOCSTEP )
					* sizeof( @@prefix_tok ) ) ) )
		{
			UNICC_OUTOFMEM;
			return false;
		}

		this->stacksize += UNICC_MALLOCSTEP;
		this->tos += size;
	}

	return true;
}
